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
#include <innate.h>
#include <arena.h>
#include <race.h>

// external vars

extern struct index_data *mob_index;
extern CWorld world;
extern struct descriptor_data *descriptor_list;
extern bool MOBtrigger;
extern struct mprog_throw_type *g_mprog_throw_list;

extern CHAR_DATA *activeActor;
extern CHAR_DATA *activeRndm;
extern CHAR_DATA *activeTarget;
extern OBJ_DATA *activeObj;
extern void *activeVo;

extern struct zone_data *zone_table;
extern int top_of_world;     
extern room_data ** world_array;

struct obj_data *get_object_in_equip_vis(struct char_data *ch, char *arg, struct obj_data *equipment[], int *j, bool blindfighting);

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
    case LOAD_PROG:		return "load_prog";
    case CAN_SEE_PROG:		return "can_see_prog";
    case DAMAGE_PROG:		return "damage_prog";
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


int do_mphit( CHAR_DATA *ch, char *argument, int cmd )
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
	logf( IMMORTAL, LOG_WORLD, "MpHit - no argument: vnum %d.",
		mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( ( victim = get_char_room_vis( ch, arg ) ) == NULL )
    {
	logf( IMMORTAL, LOG_WORLD, "MpHit - Victim not in room: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( victim == ch )
    {
	logf( IMMORTAL, LOG_WORLD, "MpHit - Bad victim to attack: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
    {
	logf( IMMORTAL, LOG_WORLD, "MpHit - Charmed mob attacking master: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    return one_hit( ch, victim, TYPE_UNDEFINED, FIRST );
}

int do_mpaddlag( CHAR_DATA *ch, char *argument, int cmd )
{
    char      arg[ MAX_INPUT_LENGTH ];
    char      arg1[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg1 );

    if ( arg[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "MpAddlag - no argument: vnum %d.",
		mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    if ( ( victim = get_char_room_vis( ch, arg ) ) == NULL )
    {
	logf( IMMORTAL, LOG_WORLD, "MpAddlag - Victim not in room: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }
    if (!arg1[0] || !is_number(arg1))
    {
	logf( IMMORTAL, LOG_WORLD, "MpAddlag - Invalid duration: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }
    WAIT_STATE(victim, atoi(arg1));
    return eSUCCESS;
}


/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy 
   just plain everything  */

int do_mpjunk( CHAR_DATA *ch, char *argument, int cmd )
{
    char      arg[ MAX_INPUT_LENGTH ];
    OBJ_DATA *obj;
    int location;
    bool dot = FALSE;
    char dotbuf[MAX_INPUT_LENGTH];
    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }
    dotbuf[0] = '\0';
    one_argument( argument, arg );

    if ( arg[0] == '\0')
    {
        logf( IMMORTAL, LOG_WORLD, "Mpjunk - No argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }
    

    if ( str_cmp( arg, "all" ) && !sscanf(arg, "all.%s", dotbuf))
    {
      if ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &location, FALSE )))
      {
	extract_obj( unequip_char( ch, location ) );
	return eSUCCESS;
      }
      if ((obj = get_obj_in_list(arg, ch->carrying)))
      {
        extract_obj( obj );
        return eSUCCESS;
      }
    }
    else
    {
	if (dotbuf[0] != '\0')
           dot = TRUE;

        for(int l = 0; l < MAX_WEAR; l++ )
          if ( ch->equipment[l] )
	   if (!dot || isname(dotbuf, ch->equipment[l]->name))
            extract_obj(unequip_char(ch,l));

  	OBJ_DATA *x,*v;
        for (x = ch->carrying; x; x =v)
	{
	   v = x->next_content;
	   if (!dot || isname(dotbuf, x->name))
             extract_obj(x);

	}
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

    if ( !( victim=get_char_room( arg,ch->in_room, TRUE ) ) )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpechoaround - victim does not exist: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }
     if (CAN_SEE(ch,victim))
    act( argument+1, ch, NULL, victim, TO_ROOM, NOTVICT );
    return eSUCCESS;
}

int do_mpechoaroundnotbad( CHAR_DATA *ch, char *argument, int cmd )
{
  char       arg[ MAX_INPUT_LENGTH ], arg1[MAX_INPUT_LENGTH];
  CHAR_DATA *victim, *victim2;

    if ( !IS_NPC( ch ) )
    {
       send_to_char( "Huh?\n\r", ch );
       return eSUCCESS;
    }

    argument = one_argument( argument, arg );
    argument = one_argument(argument,arg1);
    if ( arg[0] == '\0' || arg1[0] == '\0')
    {
       logf( IMMORTAL, LOG_WORLD, "Mpechoaroundnotbad - No argument:  vnum %d.", mob_index[ch->mobdata->nr].virt );
       return eFAILURE|eINTERNAL_ERROR;
    }
  
    if ( !( victim=get_char_room( arg,ch->in_room ) ) )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpechoaroundnotbad - victim does not exist: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }
    if ( !( victim2=get_char_room( arg1,ch->in_room ) ) )
    {
        logf( IMMORTAL, LOG_WORLD, "Mpechoaroundnotbad - victim does not exist: vnum %d.",
	    mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

//     if (CAN_SEE(ch,victim))
    act( argument+1, victim, NULL, victim2, TO_ROOM, NOTVICT );
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

    if ( !( victim = get_char_room( arg, ch->in_room, TRUE ) ) )
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
    victim->hometown = world[ch->in_room].number;
    char_to_room( victim, ch->in_room );
    return eSUCCESS;
}

int do_mpoload( CHAR_DATA *ch, char *argument, int cmd )
{
    char arg1[ MAX_INPUT_LENGTH ];
    OBJ_DATA       *obj;
    int             realnum;
    extern struct index_data *obj_index;

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

    if (obj_index[obj->item_number].virt == 393 && IS_SET(world[ch->in_room].room_flags, ARENA) && 
        arena.type == POTATO && ArenaIsOpen()) {
	return eFAILURE;
    }

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

    if ( !( victim = get_char_room( arg , ch->in_room)))
    {
	if ( ( obj = get_obj_in_list( arg, world[ch->in_room].contents ) ) ||
             ( obj = get_obj_in_list( arg, ch->carrying ) ) )
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
    if (ch == victim)
	{ extern bool selfpurge; selfpurge = TRUE; }
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

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpgoto - No argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

// TODO - make this work with strings (goto chode) too

   CHAR_DATA *vict;
   if (!str_cmp(arg, "mob"))
   {
      one_argument(argument, arg);
      if (arg[0] == '\0' || !is_number(arg))
      {
	logf( IMMORTAL, LOG_WORLD, "Mpgoto - Missing vnum after 'mob' argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
      }
      vict = get_mob_vnum(atoi(arg));
      if (!vict)
	location = -1;
      else location = vict->in_room;
   } else if (!str_cmp(arg, "pc")) {
      one_argument(argument, arg);
      if (arg[0] == '\0')
      {
	logf( IMMORTAL, LOG_WORLD, "Mpgoto - Missing arg after 'pc' argument: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
      }
      extern CHAR_DATA *get_pc(char *name);
      if (!(vict = get_pc(arg)))
	location = -1;
      else location = vict->in_room;
   } else {
     OBJ_DATA *obj;
     if ((vict = get_char_vis(ch, arg)))
     {
        location = vict->in_room; 
     }
     else if ((obj = get_obj_vis(ch, arg)) && obj->in_room >= 0) {
   	location = obj->in_room;
     } else {
       location = atoi(arg);
     }
    }
    if ( location < 0 )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpgoto - No such location: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }
    if (location == ch->in_room)
      return eFAILURE; // zz
    extern room_data ** world_array;
    extern int top_of_world;
    if (location > top_of_world || !world_array[location])
	location = 0;

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
   CHAR_DATA *vict;
   if (!(vict = get_char_vis(ch, arg)))
   {
       location = atoi(arg);
   }
   else {
        location = vict->in_room; 
   }

    if ( location < 0 )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpat - No such location: vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }
    extern room_data ** world_array;
    extern int top_of_world;
    if (location > top_of_world || !world_array[location])
	location = 0;
    original = ch->in_room;
    char_from_room( ch, false ); // Don't stop all fighting
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
      
    csendf(vict, "You receive %d exps.\r\n", reward);
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
	    &&   d->character->in_room == ch->in_room
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
            else csendf(victim, "Mob %d just tried to MOBProg force you to '%s'.\r\n", 
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
  char second[MAX_INPUT_LENGTH];
  char third[MAX_INPUT_LENGTH];
  char fourth[MAX_INPUT_LENGTH];	
  char fifth[MAX_INPUT_LENGTH];
  // locate and validate argument to find target
  argument = one_argument(argument, first);
  argument = one_argument(argument, second);
  argument = one_argument(argument, third);
  argument = one_argument(argument, fourth);
  argument = one_argument(argument, fifth);

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

  if(!check_range_valid_and_convert(delay, third, 0, 500000)) {
    logf( IMMORTAL, LOG_WORLD, "Mpthrow - Invalid delay: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
    return eFAILURE;
  }

  int opt = 0;
  if(!check_range_valid_and_convert(opt, fifth, 0, 50000))
     opt = 0;

  // create struct
  throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct mprog_throw_type));
  throwitem->target_mob_num = mob_num;
  strcpy(throwitem->target_mob_name, first);
  throwitem->data_num = catch_num;
  throwitem->delay = delay;
  throwitem->mob = TRUE; // This is, suprisingly, a mob
  throwitem->actor = activeActor;
  throwitem->obj = activeObj;
  throwitem->vo = activeVo;
  
  if (fourth[0] !='\0')
   throwitem->var = str_dup(fourth);
  else
   throwitem->var = NULL;
  throwitem->opt = opt;
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
   
    extern void prepare_character_for_sixty(CHAR_DATA *ch);
    prepare_character_for_sixty(ch);

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

char *getTemp(CHAR_DATA *ch, char *name)
{
  struct tempvariable *eh;
  for (eh = ch->tempVariable; eh; eh = eh->next)
    if (!str_cmp(eh->name, name)) 
	return eh->data;
  return NULL;
}

int do_mpsettemp(CHAR_DATA *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  char temp[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  if (!IS_NPC(ch)) {
    send_to_char("Huh?\r\n",ch);
    return eFAILURE;
  }
  argument = one_argument(argument, arg);
  argument = one_argument(argument, temp);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  if (arg[0] == '\0' || temp[0] == '\0' || arg2[0] == '\0')
  {
    int num = mob_index[ch->mobdata->nr].virt;
    sprintf(arg, "Mob %d lacking argument for mpsettemp.", num);
    log(arg, 104, LOG_BUG);
  }
  victim = get_char_room( arg, ch->in_room );
  if (!victim) return eFAILURE;
  struct tempvariable *eh;
  for (eh = victim->tempVariable; eh; eh = eh->next)
  {
    if (!str_cmp(eh->name, temp))
      break;
  }
  if (eh)
  {
    dc_free(eh->data);
    eh->data = str_dup(arg2);
  } else {
#ifdef LEAK_CHECK
        eh = (struct tempvariable *)
                        calloc(1, sizeof(struct tempvariable));
#else
       eh = (struct tempvariable *)
                        dc_alloc(1, sizeof(struct tempvariable));
#endif

     eh->data = str_dup(arg2);
     eh->name = str_dup(temp);
     eh->next = victim->tempVariable;
     victim->tempVariable = eh;
     if (arg3[0] != '\0' && !str_cmp(arg3, "save"))
	eh->save = 1;
     else
	eh->save = 0;
  }
  return eSUCCESS;
}

int do_mpsetalign(CHAR_DATA *ch, char *argument, int cmd)
{
  CHAR_DATA *victim;
  char arg[MAX_INPUT_LENGTH], align[MAX_INPUT_LENGTH];
  if (!IS_NPC(ch))
  {
    send_to_char("Huh?\r\n",ch);
    return eFAILURE;
  }
  if (!*argument || !ch->in_room)
    return eFAILURE;
  half_chop(argument, arg, align);
  victim = get_char_room(arg, ch->in_room);
  if (!victim || (!is_number(align) && (!is_number(align+1) || *align != '-'))) return eFAILURE;
  if (atoi(align) > 1000 || atoi(align) < -1000)
    return eFAILURE;
  victim->alignment = atoi(align);
  return eSUCCESS;
}


// List of people who have been recently damaged by mpdamage
// Used for connecting mpdamage with messaging
struct damage_list
{
  struct damage_list *next;
  char name[512];
  int damage;               // Damage #
};

struct damage_list *dmg_list = NULL;

void free_dmg_list()
{
  struct damage_list *c,*n;
  for (c = dmg_list;c;c = n)
  {
     n = c->next;
     dc_free(c);
  }
  dmg_list = NULL;
}

void add_dmg(CHAR_DATA *ch, int dmg)
{
  struct damage_list *c;

  for (c = dmg_list;c;c = c->next)
  {
    if ((IS_NPC(ch) && !str_cmp(c->name, GET_SHORT(ch))) ||
	(!IS_NPC(ch) && !str_cmp(c->name, GET_NAME(ch))))
    {
       c->damage += dmg;
       return;
    }
  }


#ifdef LEAK_CHECK
  c = (struct damage_list *)calloc(1, sizeof(struct damage_list));
#else
  c = (struct damage_list *)dc_alloc(1, sizeof(struct damage_list));
#endif
  if (IS_NPC(ch))
    strcpy(c->name, GET_SHORT(ch));
  else
    strcpy(c->name, GET_NAME(ch));
  c->damage = dmg;
  c->next = dmg_list;
  dmg_list = c;
}

int do_mpdamage( CHAR_DATA *ch, char *argument, int cmd )
{
    char arg[ MAX_INPUT_LENGTH ];
    char temp[ MAX_INPUT_LENGTH ];
    char damroll[ MAX_INPUT_LENGTH ];
    char attacktype[ MAX_INPUT_LENGTH ];

    free_dmg_list();
    if ( !IS_NPC( ch ) )
    {
	send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }

    argument = one_argument(argument,arg);


    if ( arg[0] == '\0' )
    {
	logf( IMMORTAL, LOG_WORLD, "Mpdamage - Bad syntax: Missing victim - vnum %d.", mob_index[ch->mobdata->nr].virt );
	return eFAILURE|eINTERNAL_ERROR;
    }

    CHAR_DATA *victim = NULL;

    // if it's 'all' leave victim NULL and skip
    if(strcmp(arg, "all") && strcmp(arg, "allpc"))
    {
       victim = get_char_room( arg, ch->in_room );
       if(victim && ( GET_LEVEL(victim) > MORTAL || IS_MOB(victim) ) ) // don't target immortals
           victim = NULL;

       if (!victim)
           return eFAILURE;  // not an error, just couldn't get valid vict
    }

    int retval = eSUCCESS;
    while (1)
    {
      argument = one_argument(argument,damroll);
      argument = one_argument(argument,attacktype);
      argument = one_argument(argument,temp);

      if (damroll[0] == '\0')
	break;
      int numdice, sizedice;
      numdice = sizedice = 0;
      bool perc = TRUE;
      char t,l,o;
      int plus = 0;
      bool plusPerc = TRUE;

      if (sscanf(damroll, "%dd%d%c%c%d%c", &numdice, &sizedice, &t, &l, &plus, &o) != 6 || l != '+' || t != '%' || o != '%')
      {
       plusPerc = FALSE;
       if (sscanf(damroll, "%dd%d%c%c%d", &numdice, &sizedice, &t, &l, &plus) != 5 || l != '+' || t != '%')
        if (sscanf(damroll, "%dd%d%c", &numdice, &sizedice,&t) != 3 || t != '%')
        {
 	  perc = FALSE;
	  plusPerc = TRUE;
	  if (sscanf(damroll, "%dd%d%c%d%c", &numdice, &sizedice, &l, &plus,&o) != 5 || l != '+' || o != '%')
	  {
	    plusPerc = FALSE;
  	    if (sscanf(damroll, "%dd%d%c%d", &numdice, &sizedice, &l, &plus) != 4 || l != '+')
	  	sscanf(damroll,"%dd%d",&numdice, &sizedice);
	  }
        }
      }
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
      int dam = 0;
    // do the damage
    // half of the casts are probably pointless, but whatever

      if(victim) {
	int *data = NULL;
	if (!temp[0] || !str_cmp(temp,"hitpoints"))
	  data = &GET_HIT(victim);
	else if (!str_cmp(temp, "mana"))
	  data = &GET_MANA(victim);
	else if (!str_cmp(temp, "ki"))
	  data = &GET_KI(victim);
	else if (!str_cmp(temp, "move"))
	  data = &GET_MOVE(victim);
	else {
           logf( IMMORTAL, LOG_WORLD, "Mpdamage - Must damage either ki,mana,hitpoints or move: vnum %d", mob_index[ch->mobdata->nr].virt);
           return eFAILURE|eINTERNAL_ERROR;
	}
	if (perc)
         dam = (int)((double)(*data) / 100.0 * (double)dice(numdice, sizedice));
	else
	 dam = dice(numdice, sizedice);
       if (plusPerc)
         dam += (int)((double)(*data) / 100.0 * (double)plus);
       else
         dam += plus;
       add_dmg(victim,dam);
       if (!temp[0] || !str_cmp(temp,"hitpoints")) {
	 retval = damage(ch, victim, dam, damtype, TYPE_UNDEFINED, 0);
	 if (SOMEONE_DIED(retval)) return retval;
       } else {
	 *data -= dam;
	 if (*data < 0) *data = 0;
       }
/*
       else if (!str_cmp(temp, "mana"))
       {
	   GET_MANA(victim) -= dam;
	   if (GET_MANA(victim) < 0) GET_MANA(victim) = 0;
       } else if (!str_cmp(temp, "ki"))
       {
	   GET_KI(victim) -= dam;
	   if (GET_KI(victim) < 0) GET_KI(victim) = 0;
       } else if (!str_cmp(temp, "move"))
       {
   	   GET_MOVE(victim) -= dam;
	   if (GET_MOVE(victim) < 0) GET_MOVE(victim) = 0;
       } else {
           logf( IMMORTAL, LOG_WORLD, "Mpdamage - Must damage either ki,mana,hitpoints or move: vnum %d", mob_index[ch->mobdata->nr].virt);
           return eFAILURE|eINTERNAL_ERROR;
       }*/
	continue;
      }
      
      char_data * next_vict;
      for(victim = world[ch->in_room].people; victim; victim = next_vict)
      {
        next_vict = victim->next_in_room;
        if((!IS_NPC(victim) && GET_LEVEL(victim) > MORTAL) || victim==ch)
           continue;
	if (!strcmp(arg, "allpc") && IS_NPC(victim)) {
	    continue;
	}
        int *data = NULL;
        if (!temp[0] || !str_cmp(temp,"hitpoints"))
          data = &GET_HIT(victim);
        else if (!str_cmp(temp, "mana"))
          data = &GET_MANA(victim);
        else if (!str_cmp(temp, "ki"))
          data = &GET_KI(victim);
        else if (!str_cmp(temp, "move"))
          data = &GET_MOVE(victim);
        else {
           logf( IMMORTAL, LOG_WORLD, "Mpdamage - Must damage either ki,mana,hitpoints or move: vnum %d", mob_index[ch->mobdata->nr].virt);
           return eFAILURE|eINTERNAL_ERROR;
        }
        if (perc)
         dam = (int)((double)(*data) / 100.0 * (double)dice(numdice, sizedice));
        else
         dam = dice(numdice, sizedice);
       if (plusPerc)
         dam += (int)((double)(*data) / 100.0 * (double)plus);
       else
         dam += plus;
       add_dmg(victim,dam);
       if (!temp[0] || !str_cmp(temp,"hitpoints")) {
         retval = damage(ch, victim, dam, damtype, TYPE_UNDEFINED, 0);
         if (IS_SET(retval, eCH_DIED)) return retval;
       } else {
         *data -= dam;
         if (*data < 0) *data = 0;
       }
/*
        if (perc)
         dam = (int)((double)GET_HIT(victim) / 100.0 * (double)dice(numdice, sizedice));
        else
         dam = dice(numdice, sizedice);
       if (plusPerc)
         dam += (int)((double)GET_HIT(victim) / 100.0 * (double)plus);
       else
         dam += plus;
	 add_dmg(victim,dam);
        if (!temp[0] || !str_cmp(temp,"hitpoints"))
	{
            retval = damage(ch, victim, dam, damtype, TYPE_UNDEFINED, 0);
  	    if (IS_SET(retval, eCH_DIED)) return retval;

	}
        else if (!str_cmp(temp, "mana"))
        {
           GET_MANA(victim) -= dam;
           if (GET_MANA(victim) < 0) GET_MANA(victim) = 0;
        } else if (!str_cmp(temp, "ki"))
        {
           GET_KI(victim) -= dam;
           if (GET_KI(victim) < 0) GET_KI(victim) = 0;
        } else if (!str_cmp(temp, "move"))
        {
           GET_MOVE(victim) -= dam;
           if (GET_MOVE(victim) < 0) GET_MOVE(victim) = 0;
        } else {
           logf( IMMORTAL, LOG_WORLD, "Mpdamage - Must damage either ki,mana,hitpoints or move: vnum %d", mob_index[ch->mobdata->nr].virt);
           return eFAILURE|eINTERNAL_ERROR;
        }*/
        if(IS_SET(retval, eCH_DIED))
           return retval;
      }
    }
    return eSUCCESS;
}


int do_mpothrow( CHAR_DATA *ch, char *argument, int cmd )
{
  struct mprog_throw_type * throwitem = NULL;
  int mob_num;
  int catch_num;
  int delay;

  char first[MAX_INPUT_LENGTH];
  char second[MAX_INPUT_LENGTH];
  char third[MAX_INPUT_LENGTH];
  char fourth[MAX_INPUT_LENGTH];
  // locate and validate argument to find target
  argument = one_argument(argument, first);
  argument = one_argument(argument, second);
  argument = one_argument(argument, third);
  argument = one_argument(argument, fourth);


  if(isdigit(*first)) {
    if(!check_valid_and_convert(mob_num, first) ||
(real_object(mob_num) < 0)) {
      logf( IMMORTAL, LOG_WORLD, "Mpothrow - Invalid objnum: vnum %d.",
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

  if(!check_range_valid_and_convert(catch_num, second, MPROG_CATCH_MIN,
MPROG_CATCH_MAX)) {
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
  throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct
mprog_throw_type));
  throwitem->target_mob_num = mob_num;
  strcpy(throwitem->target_mob_name, first);
  throwitem->data_num = catch_num;
  throwitem->delay = delay;
  throwitem->mob = FALSE;
  if (fourth[0] != '\0')
    throwitem->var = str_dup(fourth);
  else throwitem->var = NULL;

  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;

  return eSUCCESS;
}

int skill_aff[] = 
{
  4, 29, 18, 19, 20, 44,
  123, 36, 0, SPELL_EAS, 17, 145, 
  33, 34, 84, 81, 86, 38,
  89, 0, 0, 0, 0, 0, SPELL_SOLIDITY, 72,
  SPELL_CANTQUIT, SPELL_KILLER, 56, 133, 74, 0,
  SPELL_SHADOWSLIP, SPELL_INSOMNIA, SPELL_FREEFLOAT,
  SPELL_FARSIGHT, SPELL_CAMOUFLAGE, SPELL_STABILITY,
  0, 0, SPELL_FOREST_MELD, SKILL_SONG_INSANE_CHANT,
  SKILL_SONG_GLITTER_DUST, SKILL_SONG_STICKY_LULL,
  0, SPELL_PROTECT_FROM_GOOD, SKILL_INNATE_POWERWIELD,
  SKILL_INNATE_REGENERATION, SKILL_INNATE_FOCUS,
  SPELL_KNOW_ALIGNMENT, 0, SPELL_WATER_BREATHING, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int do_mpbestow(CHAR_DATA *ch, char *argument, int cmd)
{
   char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH],
     arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  CHAR_DATA *victim, *owner = NULL;
  if (!IS_NPC(ch)) return eFAILURE;
  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);

  if (arg[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0') return eFAILURE;
    if ( ( victim = get_char_room(arg,ch->in_room,TRUE ) ) == NULL && 
      str_cmp(arg, "all") && str_cmp(arg,"allpc"))
    {
        logf( IMMORTAL, LOG_WORLD, "Mpbestow - No such person: vnum %d.",
            mob_index[ch->mobdata->nr].virt );
        return eFAILURE|eINTERNAL_ERROR;
    }
  int i,o=0;
  if (!check_range_valid_and_convert(i, arg2,0, 100)) return eFAILURE|eINTERNAL_ERROR;
  if (!check_range_valid_and_convert(o, arg3,0, 10000));
  int a = 0;
  if (ch->beacon) owner = (CHAR_DATA*)ch->beacon;
  extern char *affected_bits[];
  if (!victim) victim = world[ch->in_room].people;
  int z=0;
  for (;victim;) {
   for ( ; affected_bits[a][0] != '\n'; a++) 
   {
    if (!str_cmp(affected_bits[a], arg1))
    {
	//debugpoint();
	struct affected_type af;
        af.type = z = skill_aff[a];
	if (affected_by_spell(victim, z+BASE_TIMERS))
        {
//		send_to_char("A s.\r\n",victim);
		return eFAILURE;
        }
	af.duration = i;
	af.bitvector = a+1;
	af.location = 0;
	af.modifier = 987; // Notifies that it's timered. 
	affect_join(victim, &af, TRUE, FALSE);
        if (z && o) // Timer on it
        {
          af.type = BASE_TIMERS + z;
          af.duration = o;
          af.bitvector = -1;
          af.location = 0;
          af.modifier = 0;
          affect_join(victim, &af, TRUE, FALSE);
         }

    }
   } 
   if (!str_cmp(arg, "all")) victim = victim->next_in_room;
   else if (!str_cmp(arg, "allpc")) {
      while ((victim = victim->next_in_room)) {
	  if (!IS_NPC(victim)) break;	
      }
   } else break;
  }
  if (z && o && owner) // Timer on it
  {
        struct affected_type af;
    af.type = BASE_TIMERS + z;
    af.duration = o;
    af.bitvector = -1;
    af.location = 0;
    af.modifier = 0;
    affect_join(owner, &af, TRUE, FALSE);
  }
  return eSUCCESS;
}

// simulate a pause in proc execution
// stops prog, mpthrow a special kinda throw, picks it up again when delay is over
int do_mppause( CHAR_DATA *ch, char *argument, int cmd )
{
  struct mprog_throw_type * throwitem = NULL;
  int delay;

  char first[MAX_INPUT_LENGTH];

  argument = one_argument(argument, first);

  if(!check_range_valid_and_convert(delay, first, 0, 65536)) {
    logf( IMMORTAL, LOG_WORLD, "Mppause - Invalid delay: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
    return eFAILURE;
  }

  // create struct
  throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct mprog_throw_type));
  throwitem->target_mob_num = mob_index[ch->mobdata->nr].virt;
  throwitem->target_mob_name[0] = '\0';
  throwitem->tMob = ch;
  throwitem->data_num = -999;
  throwitem->delay = delay;
  throwitem->mob = TRUE; // This is, suprisingly, a mob

  extern CHAR_DATA *activeActor;
  extern OBJ_DATA *activeObj;
  extern void *activeVo;
  throwitem->actor = activeActor;
  throwitem->obj = activeObj;
  throwitem->vo = activeVo;


  extern char *activeProg;
  extern char *activePos;
  extern char *activeProgTmpBuf;
  throwitem->orig = str_dup(activeProg);

  extern int cIfs[256];
  throwitem->cPos = 0;
  memcpy(&throwitem->ifchecks[0], &cIfs[0], sizeof(int) * 256);

  throwitem->startPos = activePos - activeProgTmpBuf;

  throwitem->var = NULL;
  throwitem->opt = 0;
  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;
  return eSUCCESS|eDELAYED_EXEC;
}

int do_mpteleport(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char person[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    int to_room = 0;
    
    if (IS_PC(ch) && GET_LEVEL(ch) < 110)
	return eFAILURE;

    half_chop(argument, person, type);

    if (!*person) {
	strncpy(person, GET_NAME(ch), MAX_INPUT_LENGTH);
    }

   if (!(victim = get_char_vis(ch, person))) { 
       if (!strcmp(person, "area")) {
	   victim = ch;
       } else {
	   send_to_char("No-one by that name around.\n\r", ch);
	   return eFAILURE;
       }
    }

  if(IS_SET(world[victim->in_room].room_flags, TELEPORT_BLOCK) ||
     IS_AFFECTED(victim, AFF_SOLIDITY)) {
      send_to_char("You find yourself unable to.\n\r", ch);
      if(ch != victim) {
          snprintf(buf, MAX_INPUT_LENGTH, "%s just tried to teleport you.\n\r", GET_SHORT(ch));
          send_to_char(buf, victim);
      }
      return eFAILURE;
  }

  int i = world[ch->in_room].zone,
      low = zone_table[i].bottom_rnum,
      high = zone_table[i].top_rnum,
      attempts = 0;

  do {
      if ((*type && !strcmp(type, "area")) || 
	  (victim == ch && *person && !strcmp(person, "area"))) {
	  to_room = number(low, high);

	  // Check to see if we're in an endless loop
	  if (attempts++ > high-low) {
	      return eFAILURE;
	  }
      } else {
	  to_room = number(0, top_of_world);
      }
  } while (!world_array[to_room] ||
	   IS_SET(world[to_room].room_flags, PRIVATE) ||
	   IS_SET(world[to_room].room_flags, IMP_ONLY) ||
	   IS_SET(world[to_room].room_flags, NO_TELEPORT) ||
	   IS_SET(world[to_room].room_flags, ARENA) ||
	   (world[to_room].sector_type == SECT_UNDERWATER && GET_RACE(victim) != RACE_FISH) ||
	   IS_SET(zone_table[world[to_room].zone].zone_flags, ZONE_NO_TELEPORT) ||
	   ( (IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN)) ? 
	     (IS_SET(zone_table[world[to_room].zone].zone_flags, ZONE_IS_TOWN)) : FALSE ) ||
	   ( IS_AFFECTED(victim, AFF_CHAMPION) && (IS_SET(world[to_room].room_flags, CLAN_ROOM) ||
						   to_room >= 1900 && to_room <= 1999) ) );

  act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
  move_char(victim, to_room);
  act("$n slowly fades into existence.", victim, 0, 0, TO_ROOM, 0);

  do_look(victim, "", 0);
  return eSUCCESS;
}

int do_mppeace( struct char_data *ch, char *argument, int cmd )
{
    if ( !IS_NPC( ch ) )
    {
        send_to_char( "Huh?\n\r", ch );
	return eSUCCESS;
    }
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);

    struct char_data *rch,*vict = NULL;

    if (arg[0]) {
      vict = get_char_room(arg,ch->in_room);
      if (!vict) {
	    logf( IMMORTAL, LOG_WORLD, "Mppeace - Vict not found: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
	    return eFAILURE;
      }
      if ( IS_MOB(vict) && vict->mobdata->hatred != NULL )
        remove_memory(vict, 'h');
      if ( vict->fighting != NULL )
        stop_fighting(vict);
     return eSUCCESS;
    }
    for (rch = world[ch->in_room].people; rch!=NULL; rch = rch->next_in_room) {
        if ( IS_MOB(rch) && rch->mobdata->hatred != NULL )
            remove_memory(rch, 'h');
        if ( rch->fighting != NULL )
            stop_fighting(rch);
    }
    return eSUCCESS;
}


int do_mpretval( struct char_data *ch, char *argument, int cmd )
{
  char *retvals[] =
  {
     "true",
     "false",
     "\n"
  };
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument,arg);
  int retval = eSUCCESS;

  for (int i = 0; retvals[i][0] != '\n'; i++)
   if (!str_cmp(arg,retvals[i]))
     SET_BIT(retval, 1<<(5+i));

  return retval;
}




// Yes, I'm sure there's a better way to do this. Stop whining.
int process_math(char_data *ch, char *string)
{
  int result = 0,curr = 0; 
  char lastsign = '\0';
  bool numproc = FALSE;
  if (!string) return -9839;

  while (1)
  {
    if (*string == ' ') { string++; continue; }
    if (isdigit(*string))
    {
      numproc = TRUE;
      curr *= 10;
      curr += (*string - '0');
      string++;
      continue;
    } else if (numproc) {
      if (lastsign == '\0')
	result = curr;
      else
      {
	switch (lastsign)
	{
	  case '+':		result += curr;break;
	  case '-':		result -= curr;break;
	  case '/':		result /= curr;break;
	  case '*':		result *= curr;break;
	}
      }
      curr = 0;
    }
    if (*string == '\0') break; 
    switch (*string)
    {
	case '+': 
	case '-': 
        case '/': 
	case '*': 
	  curr = 0;
	  lastsign = *string;
	  break;
	default:
	    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - Unknown symbol: %c: vnum %d.",
	  	*string, mob_index[ch->mobdata->nr].virt );
	    return result;
	  break;
    };
    string++;
  }
  return result;
}


// Unreadable code > you
char *expand_data(char_data *ch, char *orig)
{
  static char buf[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH];
  while (*orig == ' ') orig++;
  strcpy(buf1,orig);
  orig = &buf1[0];
  buf[0] = '\0';
  char *ptr;
  int i = 0,l,r,z = 0,o;
  char c;
  while (1)
  {
    char left[MAX_INPUT_LENGTH];
    char right[MAX_INPUT_LENGTH];
    ptr = strchr(orig, '.');
    if (ptr == orig) break;
    if (!ptr) break;
    *ptr = '#';
    for (l = 1; ; l++)
    {
       if ((ptr-l) == orig)
        break;
       if (!isalpha(*(ptr-l)) && (*(ptr-l) != ',') &&
	(*(ptr-l) != '_')) 
  	{
	// Failsafe to ensure no 'reserved' characters are being used, ie. # ~
	  l--;
  	  break;
	}
    }
    *(ptr) = '\0';
    strcpy(left, (ptr-l));
    *(ptr) = '#';
    *(ptr-l) = '~';
    for (r = 1; ; r++)
    {
       if ((ptr+r) == '\0')
        break;
       if (!isalpha(*(ptr+r)) && (*(ptr+r) != ',') && (*(ptr-l) != '_'))
	break;
 
    }
    c = *(ptr+r);
    *(ptr+r) = '\0';    
    strcpy(right, ptr+1);
    *(ptr+r) = c;
    r--;
    if (!c == '\0')
      *(ptr+r) = '~';
    
    int16 *lvali = 0;
    uint32 *lvalui = 0;
    char **lvalstr = 0;
    int64 *lvali64 = 0;
    sbyte *lvalb = 0;
    translate_value(left,right,&lvali,&lvalui, &lvalstr,&lvali64, &lvalb,ch,activeActor, activeObj, activeVo, activeRndm);

    if (!lvali && !lvalui && !lvalb)
    {
	    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - Expand_data - Accessing unknown data: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
	    return NULL;
    }

    for (o = z; ; o++)
    {
      if (*(orig+o) == '~' || *(orig+o) == '\0')
       break;
      buf[i++] = *(orig+o);
    }
    buf[i] = '\0';
    char tmp[MAX_INPUT_LENGTH];
    if (lvali)
      sprintf(tmp, "%d",*lvali);
    if (lvalui)
      sprintf(tmp, "%u",*lvalui);
    if (lvalb)
      sprintf(tmp, "%d",*lvalb);
    strcat(buf,tmp);
    i += strlen(tmp);
    z = (ptr-orig)+r+1;
  }


  for (o = z; ; o++)
  {
    if (*(orig+o) == '~' || *(orig+o) == '\0')
       break;
    buf[i++] = *(orig+o);
  }
  buf[i] = '\0';
  return &buf[0];
}


char *allowedData[] = {
 "hitpoints", "mana", "move", NULL
};

int do_mpsetmath(char_data *ch, char *arg, int cmd)
{
  char_data *vict;
//  if (activeActor) csendf(activeActor, "{%s}\r\n", arg);
//  vict = get_pc("Urizen");
//  if (vict) csendf(vict, "{%s}\r\n", arg);

  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  arg = one_argument(arg, arg1);
  char *r = NULL;

  if ((r = strchr(arg1, '.')) != NULL)
  {
    *r = '\0';
    r++;
    one_argument(r, arg2);
  }
  if (!r || !*r || !*arg1)
  {
	    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - Invalid primary data: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
	     return eFAILURE;
  }
  bool allowed = FALSE;

  int16 *lvali = 0;
  uint32 *lvalui = 0;
  char **lvalstr = 0;
  int64 *lvali64 = 0;
  sbyte *lvalb = 0;
  translate_value(arg1,r,&lvali,&lvalui, &lvalstr,&lvali64, &lvalb,ch,activeActor, activeObj, activeVo, activeRndm);

  vict = activeTarget;
  if (!vict)
  {
	    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - No target: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
	    return eFAILURE;
  }
  if (IS_NPC(vict)) allowed = TRUE;
  else
    for (int i = 0; allowedData[i]; i++)
      if (!str_cmp(allowedData[i],r))
	allowed = TRUE;
  if (!allowed)
  {
	    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - Accessing unallowed data: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
	    return eFAILURE;
  }

  if (lvalstr)
  {
    char nw[MAX_INPUT_LENGTH];
    arg = one_argument_long(arg,nw);
    if (!nw[0])
    {
	    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - Invalid string: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
	    return eFAILURE;
    }
    *lvalstr = str_dup(nw);    
    return eSUCCESS;
  }
  if (!lvalui && !lvali && !lvalb)
  {
	    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - Accessing unknown data: vnum %d.",
	  	mob_index[ch->mobdata->nr].virt );
	    return eFAILURE;
  }

  char *fixed = expand_data(ch, arg);

  int i = process_math(ch, fixed);

  if (i == -9839)
  {
    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - Invalid math-string: vnum %d.",
  	mob_index[ch->mobdata->nr].virt );
    return eFAILURE;
  }
  if (lvali)
  {
    *lvali = i;
    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - %s set to %d: vnum %d.",
  	r, i, mob_index[ch->mobdata->nr].virt );
  }
  if (lvalb)
  {
    *lvalb = (sbyte)i;
    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - %s set to %d: vnum %d.",
  	r, i, mob_index[ch->mobdata->nr].virt );
  }
  if (lvalui)
  {
    *lvalui = (unsigned int)i;
    logf( IMMORTAL, LOG_WORLD, "Mpsetmath - %s set to %d: vnum %d.",
  	r, i, mob_index[ch->mobdata->nr].virt );
  }


/*  csendf(vict, "%d\r\n%d\r\n%d\r\n%d\r\n",
		process_math(ch, "1+1-3+5*10"),
		process_math(ch, "1-3+5*10/2"),
		process_math(ch, "1+101-34+5*10*2/8"),
		process_math(ch, "1+101-34+5*10*2/8")
		);
  */return eSUCCESS;
}
