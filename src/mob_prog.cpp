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
 *  such installation can be found in INSTALL.  Enjoy...         N'Atas-Ha *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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

// Extern variables

extern CWorld world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
CHAR_DATA *rndm2;

// Global defined here

bool  MOBtrigger;
struct mprog_throw_type *g_mprog_throw_list = 0;   // holds all pending mprog throws

/*
 * Local function prototypes
 */

char *	mprog_next_command	( char* clist );
int	mprog_seval		( char* lhs, char* opr, char* rhs );
int	mprog_veval		( int lhs, char* opr, int rhs );
int	mprog_do_ifchck		( char* ifchck, CHAR_DATA* mob,
				       CHAR_DATA* actor, OBJ_DATA* obj,
				       void* vo, CHAR_DATA* rndm );
char *	mprog_process_if	( char* ifchck, char* com_list, 
				       CHAR_DATA* mob, CHAR_DATA* actor,
				       OBJ_DATA* obj, void* vo,
				       CHAR_DATA* rndm );
void	mprog_translate		( char ch, char* t, CHAR_DATA* mob,
				       CHAR_DATA* actor, OBJ_DATA* obj,
				       void* vo, CHAR_DATA* rndm );
int	mprog_process_cmnd	( char* cmnd, CHAR_DATA* mob, 
				       CHAR_DATA* actor, OBJ_DATA* obj,
				       void* vo, CHAR_DATA* rndm );
void	mprog_driver		( char* com_list, CHAR_DATA* mob,
				       CHAR_DATA* actor, OBJ_DATA* obj,
				       void* vo );

/***************************************************************************
 * Local function code and brief comments.
 */

/* Used to get sequential lines of a multi line string (separated by "\n\r")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line string argument, and thus clist must not be shared.
 */
char *mprog_next_command( char *clist )
{

  char *pointer = clist;

  while ( *pointer != '\n' && *pointer != '\0' )
    pointer++;
  while (*pointer == '\n' || *pointer == '\r')
     *pointer++ = '\0';
/*  if ( *pointer == '\n' )
    *pointer++ = '\0';
  if ( *pointer == '\r' )
    *pointer++ = '\0';*/

  return ( pointer );

}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
	logf( IMMORTAL, LOG_WORLD, "Str_prefix: null astr.", 0 );
	return TRUE;
    }

    if ( bstr == NULL )
    {
	logf( IMMORTAL, LOG_WORLD, "Str_prefix: null bstr.", 0 );
	return TRUE;
    }

    for ( ; *astr; astr++, bstr++ )
    {
	if ( LOWER(*astr) != LOWER(*bstr) )
	    return TRUE;
    }

    return FALSE;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */

bool str_infix( const char *astr, const char *bstr 
)
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ( ( c0 = LOWER(astr[0]) ) == '\0' )
	return FALSE;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
    {
	if ( c0 == LOWER(bstr[ichar]) && !str_prefix( astr, bstr + ichar ) )
	    return FALSE;
    }

    return TRUE;
}

/* These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the string operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 */
int mprog_seval( char *lhs, char *opr, char *rhs )
{
 if (!lhs || !rhs)
     return FALSE;
 if ( !str_cmp( opr, "==" ) )
    return ( !str_cmp( lhs, rhs ) );
  if ( !str_cmp( opr, "!=" ) )
    return ( str_cmp( lhs, rhs ) );
  if ( !str_cmp( opr, "/" ) )
    return ( !str_infix( rhs, lhs ) );
  if ( !str_cmp( opr, "!/" ) )
    return ( str_infix( rhs, lhs ) );

  logf( IMMORTAL, LOG_WORLD,  "Improper MOBprog operator\n\r", 0 );
  return 0;

}

int mprog_veval( int lhs, char *opr, int rhs )
{

  if ( !str_cmp( opr, "==" ) )
    return ( lhs == rhs );
  if ( !str_cmp( opr, "!=" ) )
    return ( lhs != rhs );
  if ( !str_cmp( opr, ">" ) )
    return ( lhs > rhs );
  if ( !str_cmp( opr, "<" ) )
    return ( lhs < rhs );
  if ( !str_cmp( opr, "<=" ) )
    return ( lhs <= rhs );
  if ( !str_cmp( opr, ">=" ) )
    return ( lhs >= rhs );
  if ( !str_cmp( opr, "&" ) )
    return ( lhs & rhs );
  if ( !str_cmp( opr, "|" ) )
    return ( lhs | rhs );

  logf( IMMORTAL, LOG_WORLD,  "Improper MOBprog operator\n\r", 0 );
  return 0;

}


void debugpoint(){}
/* This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifchck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return -1 otherwise return boolean 1,0
 */

// Azrack -- this was originally returning a bool, but its returning all sorts of values,
// switched it to int 
int mprog_do_ifchck( char *ifchck, CHAR_DATA *mob, CHAR_DATA *actor,
		     OBJ_DATA *obj, void *vo, CHAR_DATA *rndm)
{

  char buf[ MAX_INPUT_LENGTH ];
  char arg[ MAX_INPUT_LENGTH ];
  char opr[ MAX_INPUT_LENGTH ];
  char val[ MAX_INPUT_LENGTH ];
  CHAR_DATA *vict = (CHAR_DATA *) vo;
  OBJ_DATA *v_obj = (OBJ_DATA  *) vo;
  char     *bufpt = buf;
  char     *argpt = arg;
  char     *oprpt = opr;
  char     *valpt = val;
  char     *point = ifchck;
  int       lhsvl;
  int       rhsvl;

  if ( *point == '\0' ) 
    {
      logf( IMMORTAL, LOG_WORLD,  "Mob: %d null ifchck", mob_index[mob->mobdata->nr].virt ); 
      return -1;
    }   
  /* skip leading spaces */
  while ( *point == ' ' )
    point++;
  if (mob_index[mob->mobdata->nr].virt == 17201)
   debugpoint();

  /* get whatever comes before the left paren.. ignore spaces */
  while ( *point != '(' ) 
    if ( *point == '\0' ) 
      {
	logf( IMMORTAL, LOG_WORLD,  "Mob: %d ifchck syntax error", mob_index[mob->mobdata->nr].virt ); 
	return -1;
      }   
    else
      if ( *point == ' ' )
	point++;
      else 
	*bufpt++ = *point++; 

  *bufpt = '\0';
  point++;

  /* get whatever is in between the parens.. ignore spaces */
  while ( *point != ')' ) 
    if ( *point == '\0' ) 
      {
	logf( IMMORTAL, LOG_WORLD,  "Mob: %d ifchck syntax error", mob_index[mob->mobdata->nr].virt ); 
	return -1;
      }   
    else
      if ( *point == ' ' )
	point++;
      else 
	*argpt++ = *point++; 

  *argpt = '\0';
  point++;

  /* check to see if there is an operator */
  while ( *point == ' ' || *point == '\r')
    point++;
  if ( *point == '\0' ) 
    {
      *opr = '\0';
      *val = '\0';
    }   
  else /* there should be an operator and value, so get them */
    {
      while ( ( *point != ' ' ) && ( !isalnum( *point ) ) ) 
	if ( *point == '\0' ) 
	  {
	    logf( IMMORTAL, LOG_WORLD,  "Mob: %d ifchck operator without value",
		 mob_index[mob->mobdata->nr].virt ); 
	    return -1;
	  }   
	else
	  *oprpt++ = *point++; 

      *oprpt = '\0';
 
      /* finished with operator, skip spaces and then get the value */
      while ( *point == ' ' )
	point++;
      for( ; ; )
	{
	  if ( ( *point != ' ' ) && ( *point == '\0' ) )
	    break;
	  else
	    *valpt++ = *point++; 
	}

      *valpt = '\0';
    }
  bufpt = buf;
  argpt = arg;
  oprpt = opr;
  valpt = val;

  /* Ok... now buf contains the ifchck, arg contains the inside of the
   *  parentheses, opr contains an operator if one is present, and val
   *  has the value if an operator was present.
   *  So.. basically use if statements and run over all known ifchecks
   *  Once inside, use the argument and expand the lhs. Then if need be
   *  send the lhs,opr,rhs off to be evaluated.
   */

  if ( !str_cmp( buf, "rand" ) )
    {
      return ( number(1, 100) <= atoi(arg) );
    }
  if ( !str_cmp(buf, "numpcs"))
  {
     struct char_data *p;
      int i =0;
     for (p = world[mob->in_room].people;p;p=p->next_in_room)
       if (!IS_NPC(p))
	i++;

           return mprog_veval( i, opr, atoi(val) );
     
  }
  if ( !str_cmp( buf, "ispc" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return 0;
	case 'z': if (mob->beacon)
             return ( IS_NPC((CHAR_DATA*)mob->beacon));
           else return -1;
	case 'n': if ( actor )
 	             return ( !IS_NPC( actor ) );
	          else return 0;
	case 't': if ( vict )
                     return ( !IS_NPC( vict ) );
	          else return 0;
	case 'r': if ( rndm )
                     return ( !IS_NPC( rndm ) );
	          else return 0;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'ispc'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "iswielding" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
        {
        case 'i': return (mob->equipment[WIELD] )?1:0;
	case 'z': if (mob->beacon)
             return ((CHAR_DATA*)mob->beacon)->equipment[WIELD]?1:0;
           else return -1;

        case 'n': if ( actor )
                     return ( actor->equipment[WIELD] ) ? 1:0;
                  else return 0;
        case 't': if ( vict )
                     return ( vict->equipment[WIELD] ) ?1:0;
                  else return 0; 
        case 'r': if ( rndm )
                     return ( rndm->equipment[WIELD] )?1:0;
                  else return 0;
        default:
          logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'ispc'", mob_index[mob->mobdata->nr].virt);
          return -1;
      }
  }
  if ( !str_cmp( buf, "isnpc" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return 1;
	case 'z': if (mob->beacon)
             return IS_NPC(((CHAR_DATA*)mob->beacon));
           else return -1;

	case 'n': if ( actor )
	             return IS_NPC( actor );
	          else return -1;
	case 't': if ( vict )
                     return IS_NPC( vict );
	          else return -1;
	case 'r': if ( rndm )
	             return IS_NPC( rndm );
	          else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD, "Mob: %d bad argument to 'isnpc'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isgood" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return IS_GOOD( mob );
	case 'z': if (mob->beacon)
             return IS_GOOD(((CHAR_DATA*)mob->beacon));
           else return -1;
	case 'n': if ( actor )
	             return IS_GOOD( actor );
	          else return -1;
	case 't': if ( vict )
	             return IS_GOOD( vict );
	          else return -1;
	case 'r': if ( rndm )
	             return IS_GOOD( rndm );
	          else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'isgood'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isworn" ) )
    {
	debug_point();
        OBJ_DATA *o;
	if (mob->mobdata->last_room > 50000) // an object
	 o = (OBJ_DATA*) mob->mobdata->last_room;
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': if (mob->beacon)
             return is_wearing(((CHAR_DATA*)mob->beacon), o);
           else return -1;
	case 'i': return -1;
	case 'n': if ( actor )
	             return is_wearing(actor,o);
	          else return -1;
	case 't': if ( vict )
	             return is_wearing(actor,o);
	          else return -1;
	case 'r': if ( rndm )
	             return is_wearing(rndm, o);
	          else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'isworn'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }
  if ( !str_cmp( buf, "isfight" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': if (mob->beacon)
             return ((CHAR_DATA*)mob->beacon)->fighting ? 1:0;
           else return -1;

	case 'i': return ( mob->fighting ) ? 1 : 0;
	case 'n': if ( actor )
	             return ( actor->fighting ) ? 1 : 0;
	          else return -1;
	case 't': if ( vict )
	             return ( vict->fighting ) ? 1 : 0;
	          else return -1;
	case 'r': if ( rndm )
	             return ( rndm->fighting ) ? 1 : 0;
	          else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'isfight'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isimmort" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return ( GET_LEVEL( mob ) > IMMORTAL );
	case 'z': if (mob->beacon)
             return ( GET_LEVEL(((CHAR_DATA*)mob->beacon)) > IMMORTAL);
           else return -1;

	case 'n': if ( actor )
	             return ( GET_LEVEL( actor ) > IMMORTAL );
  	          else return -1;
	case 't': if ( vict )
	             return ( GET_LEVEL( vict ) > IMMORTAL );
                  else return -1;
	case 'r': if ( rndm )
	             return ( GET_LEVEL( rndm ) > IMMORTAL );
                  else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'isimmort'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "ischarmed" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return IS_AFFECTED( mob, AFF_CHARM );
	case 'z': if (mob->beacon)
             return IS_AFFECTED(((CHAR_DATA*)mob->beacon), AFF_CHARM);
           else return -1;

	case 'n': if ( actor )
	             return IS_AFFECTED( actor, AFF_CHARM );
	          else return -1;
	case 't': if ( vict )
	             return IS_AFFECTED( vict, AFF_CHARM );
	          else return -1;
	case 'r': if ( rndm )
	             return IS_AFFECTED( rndm, AFF_CHARM );
	          else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'ischarmed'",
	       mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isfollow" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return ( mob->master != NULL
			  && mob->master->in_room == mob->in_room );
	case 'z': if (mob->beacon)
             return ((CHAR_DATA*)mob->beacon)->master && ((CHAR_DATA*)mob->beacon)->master->in_room
			== ((CHAR_DATA*)mob->beacon)->in_room;
           else return -1;
	case 'n': if ( actor )
	             return ( actor->master != NULL
			     && actor->master->in_room == actor->in_room );
	          else return -1;
	case 't': if ( vict )
	             return ( vict->master != NULL
			     && vict->master->in_room == vict->in_room );
	          else return -1;
	case 'r': if ( rndm )
	             return ( rndm->master != NULL
			     && rndm->master->in_room == rndm->in_room );
	          else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'isfollow'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if (!str_cmp(buf, "isspelled"))
  {
	int find_skill_num(char *name);
     switch (arg[1]) 
     {
        case 'i': // mob
		return (int)(affected_by_spell(mob, find_skill_num(val)));
	case 'z': if (mob->beacon)
             return (int)(affected_by_spell(((CHAR_DATA*)mob->beacon), find_skill_num(val)));
           else return -1;

	case 'n': // actor
	 	if (actor)
                return (int)(affected_by_spell(actor, find_skill_num(val)));
		else return -1;
	case 't': // vict
		if (vict)
                return (int)(affected_by_spell(vict, find_skill_num(val)));
		else return -1;
	case 'r': //rand
		if (rndm)
                return (int)(affected_by_spell(rndm, find_skill_num(val)));
		return -1;
     }
  }
  if ( !str_cmp( buf, "isaffected" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return ( ISSET(mob->affected_by, atoi( val )) );
	case 'z': if (mob->beacon)
             return ( ISSET(((CHAR_DATA*)mob->beacon)->affected_by, atoi(val)) );
             else return -1;

	case 'n': if ( actor )
	             return ( ISSET(actor->affected_by, atoi( val )) );
	          else return -1;
	case 't': if ( vict )
	             return ( ISSET(vict->affected_by, atoi( val )) );
	          else return -1;
	case 'r': if ( rndm )
	             return ( ISSET(rndm->affected_by, atoi( val )) );
	          else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'isaffected'",
	       mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "hitprcnt" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': lhsvl = (mob->hit*100)  / mob->max_hit;
	          rhsvl = atoi( val );
         	  return mprog_veval( lhsvl, opr, rhsvl );
	case 'z': if (mob->beacon) {
		lhsvl = (((CHAR_DATA*)mob->beacon)->hit*100)/((CHAR_DATA*)mob->beacon)->max_hit;
		rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);
	  }
           else return -1;

	case 'n': if ( actor )
	          {
		    lhsvl = (actor->hit*100) / actor->max_hit;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = (vict->hit*100) / vict->max_hit;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = (rndm->hit*100) / rndm->max_hit;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'hitprcnt'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "inroom" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': lhsvl = world[mob->in_room].number;
	          rhsvl = atoi(val);
	          return mprog_veval( lhsvl, opr, rhsvl );
	case 'z': if (mob->beacon) {
		lhsvl = world[((CHAR_DATA*)mob->beacon)->in_room].number;
	        rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);;
	}
           else return -1;

	case 'n': if ( actor )
	          {
		    lhsvl = world[actor->in_room].number;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = world[vict->in_room].number;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = world[rndm->in_room].number;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'inroom'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "sex" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': if (mob->beacon) {
		lhsvl = ((CHAR_DATA*)mob->beacon)->sex;
	      rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);
	}
           else return -1;

	case 'i': lhsvl = mob->sex;
	          rhsvl = atoi( val );
	          return mprog_veval( lhsvl, opr, rhsvl );
	case 'n': if ( actor )
	          {
		    lhsvl = actor->sex;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->sex;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->sex;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'sex'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "position" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': lhsvl = mob->position;
	          rhsvl = atoi( val );
	          return mprog_veval( lhsvl, opr, rhsvl );
	case 'z': if (mob->beacon) {
		lhsvl = ((CHAR_DATA*)mob->beacon)->position;
		rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);
	}
           else return -1;

	case 'n': if ( actor )
	          {
		    lhsvl = actor->position;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->position;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->position;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'position'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "level" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': if (mob->beacon) {
		lhsvl = GET_LEVEL(((CHAR_DATA*)mob->beacon));
		rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);
		}
           else return -1;

	case 'i': lhsvl = GET_LEVEL( mob );
	          rhsvl = atoi( val );
	          return mprog_veval( lhsvl, opr, rhsvl );
	case 'n': if ( actor )
	          {
		    lhsvl = GET_LEVEL( actor );
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else 
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = GET_LEVEL( vict );
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = GET_LEVEL( rndm );
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'level'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }
  if (!str_cmp(buf, "race") )
  {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
        {
        case 'i': lhsvl = GET_RACE(mob);
                  rhsvl = atoi( val );
                  return mprog_veval( lhsvl, opr, rhsvl );
	case 'z': if (mob->beacon)
	  {
	      lhsvl = GET_RACE(((CHAR_DATA*)mob->beacon));
	     rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);
	  }
           else return -1;

        case 'n': if ( actor )
                  {
                    lhsvl = GET_RACE(actor);
                    rhsvl = atoi( val );
                    return mprog_veval( lhsvl, opr, rhsvl );
                  }
                  else
                    return -1;
        case 't': if ( vict )
                  {
                    lhsvl = GET_RACE(vict);
                    rhsvl = atoi( val );
                    return mprog_veval( lhsvl, opr, rhsvl );
                  }
                  else
                    return -1;
        case 'r': if ( rndm )
                  {
                    lhsvl = GET_RACE(rndm);
                    rhsvl = atoi( val );
                    return mprog_veval( lhsvl, opr, rhsvl );
                  }
                  else
                    return -1;
        default:
          logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'race'", mob_index[mob->mobdata->nr].virt );
          return -1;
        }
       
  }
  if ( !str_cmp( buf, "class" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': if (mob->beacon)
	  {
	      lhsvl = GET_CLASS(((CHAR_DATA*)mob->beacon));
	     rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);
	  }
           else return -1;

	case 'i': lhsvl = GET_CLASS(mob);
	          rhsvl = atoi( val );
                  return mprog_veval( lhsvl, opr, rhsvl );
	case 'n': if ( actor )
	          {
		    lhsvl = GET_CLASS(actor);
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else 
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = GET_CLASS(vict);
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = GET_CLASS(rndm);
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'class'", 
mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if (!str_cmp(buf, "carries"))
  {
    struct obj_data *obj=0;
    CHAR_DATA *take;
    extern void debug_here();
    debug_here();
    struct obj_data * search_char_for_item(char_data * ch, int16 item_number);
    char bufeh[MAX_STRING_LENGTH];
    char *valu = one_argument(val, bufeh);
    switch (arg[1] )
    {
	case 'z': if (!mob->beacon) return -1;
		obj = search_char_for_item(((CHAR_DATA*)mob->beacon), real_object(atoi(valu)));
	      take = ((CHAR_DATA*)mob->beacon);
       case 'i': // mob
          obj = search_char_for_item(mob, real_object(atoi(valu)));
	  take = mob;
	   break;
       case 'n': // actor
	 if (!actor) return -1;
         obj = search_char_for_item(actor, real_object(atoi(valu)));
	 take = actor;
	     break;
	case 't': // vict
	  if (!vict) return -1;
          obj = search_char_for_item(vict, real_object(atoi(valu)));
	  take = vict;
	  break;
       case 'r': // rndm
	 if (!rndm) return -1;
	  obj = search_char_for_item(rndm, real_object(atoi(valu)));
	  take = rndm;
	  break;
	default:
          logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'carries'", mob_index[mob->mobdata->nr].virt );
	  return -1;
    }
    if (!obj) return 0;
    if (!str_cmp(bufeh, "keep"))
       return 1;
    else if (!str_cmp(bufeh, "take"))
    {
	int i;
       if (obj->carried_by)
	  obj_from_char(obj);
      for (i=0; i < MAX_WEAR; i++)
	 if (obj == take->equipment[i])
	 {
	   obj_from_char(unequip_char(take, i));
	 }
      extract_obj(obj);
      return 1;
    }
    return -1;
   }
  if ( !str_cmp( buf, "goldamt" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': if (mob->beacon)
	  {
	      lhsvl = ((CHAR_DATA*)mob->beacon)->gold;
	     rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);
	  }
           else return -1;

	case 'i': lhsvl = mob->gold;
                  rhsvl = atoi( val );
                  return mprog_veval( lhsvl, opr, rhsvl );
	case 'n': if ( actor )
	          {
		    lhsvl = actor->gold;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->gold;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->gold;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'goldamt'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objtype" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = GET_ITEM_TYPE(obj);
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	         else
		   return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = GET_ITEM_TYPE(obj);
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'objtype'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objval0" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->obj_flags.value[0];
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = v_obj->obj_flags.value[0];
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else 
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'objval0'",mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objval1" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->obj_flags.value[1];
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = v_obj->obj_flags.value[1];
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'objval1'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objval2" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->obj_flags.value[2];
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = v_obj->obj_flags.value[2];
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'objval2'",mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objval3" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->obj_flags.value[3];
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj ) 
	          {
		    lhsvl = v_obj->obj_flags.value[3];
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'objval3'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "number" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': lhsvl = mob_index[mob->mobdata->nr].virt;
	          rhsvl = atoi( val );
	          return mprog_veval( lhsvl, opr, rhsvl );
	case 'z': if (mob->beacon)
	  {
		if (IS_NPC(((CHAR_DATA*)mob->beacon)))
		{
	      lhsvl = mob_index[((CHAR_DATA*)mob->beacon)->mobdata->nr].virt;
	     rhsvl = atoi(val);
             return mprog_veval(lhsvl, opr, rhsvl);
		} else
		return 0;
	  }
           else return 0;

	case 'n': if ( actor )
	          {
		    if IS_NPC( actor )
		    {
		      lhsvl = mob_index[actor->mobdata->nr].virt;
		      rhsvl = atoi( val );
		      return mprog_veval( lhsvl, opr, rhsvl );
		    }
		  }
	          else
		    return 0;
	case 't': if ( vict )
	          {
		    if IS_NPC( actor )
		    {
		      lhsvl = mob_index[vict->mobdata->nr].virt;
		      rhsvl = atoi( val );
		      return mprog_veval( lhsvl, opr, rhsvl );
		    }
		  }
                  else
		    return 0;
	case 'r': if ( rndm )
	          {
		    if IS_NPC( actor )
		    {
		      lhsvl = mob_index[rndm->mobdata->nr].virt;
		      rhsvl = atoi( val );
		      return mprog_veval( lhsvl, opr, rhsvl );
		    }
		  }
	         else return 0;
	case 'o': if ( obj )
	          {
		    lhsvl = obj_index[obj->item_number].virt;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = obj_index[v_obj->item_number].virt;
		    rhsvl = atoi( val );
		    return mprog_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'number'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  if ( !str_cmp( buf, "tempvar" ) )
    {
      char buf4[MAX_STRING_LENGTH], *buf4pt;
      if (arg[2] != '[')
      {
          logf( IMMORTAL, LOG_WORLD,  "Mob: %d badtarget  to 'tempvar'",mob_index[mob->mobdata->nr].virt );
	  return -1;
      }	
      buf4pt = &arg[3];
      strcpy(buf4, buf4pt);
      buf4pt = &buf4[0];
      while (*buf4pt != ']' && *buf4pt != '\0')
	buf4pt++;
      if (*buf4pt == '\0')
      {
          logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad target to 'tempvar'",mob_index[mob->mobdata->nr].virt );
          return -1;
      }
      *buf4pt = '\0';
      if (val[0] == '$' && val[1])
        mprog_translate(val[1], val, mob, actor, obj, vo, rndm);
	char *getTemp(CHAR_DATA *ch, char *name);

      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
        {
	case 'z': if (mob->beacon)
	  {
             return mprog_seval( getTemp(((CHAR_DATA*)mob->beacon), buf4), opr, val);
	  }
           else return -1;
        case 'i': return mprog_seval( getTemp(mob, buf4), opr, val );
        case 'n': if ( actor )
                    return mprog_seval( getTemp(actor, buf4), opr, val );
                  else
                    return -1;
        case 't': if ( vict )
                    return mprog_seval( getTemp(vict, buf4), opr, val );
                  else
                    return -1;
        case 'r': if ( rndm )
                    return mprog_seval( getTemp(rndm, buf4), opr, val );
                  else
                    return -1;
        case 'o': 
                    return -1;
        case 'p': 
                    return -1;
        default:
          logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'tempvar'",mob_index[mob->mobdata->nr].virt );
          return -1;
        }
    }


  if ( !str_cmp( buf, "name" ) )
    {
    if (val[0] == '$' && val[1])
        mprog_translate(val[1], val, mob, actor, obj, vo, rndm);
       switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': if (mob->beacon)
	  {
             return mprog_seval(((CHAR_DATA*)mob->beacon)->name, opr, val);
	  }
           else return -1;

	case 'i': return mprog_seval( mob->name, opr, val );
	case 'n': if ( actor )
	            return mprog_seval( actor->name, opr, val );
	          else
		    return -1;
	case 't': if ( vict )
	            return mprog_seval( vict->name, opr, val );
	          else
		    return -1;
	case 'r': if ( rndm )
	            return mprog_seval( rndm->name, opr, val );
	          else
		    return -1;
	case 'o': if ( obj )
	            return mprog_seval( obj->name, opr, val );
	          else
		    return -1;
	case 'p': if ( v_obj )
	            return mprog_seval( v_obj->name, opr, val );
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'name'",mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  // search a room for a mob with vnum arg
  if ( !str_cmp( buf, "ismobvnuminroom" ) )
    {
      int target = atoi(arg);

      for(char_data * vch = world[mob->in_room].people;
          vch;
          vch = vch->next_in_room)
      {
        if(IS_NPC(vch) && 
           vch != mob && 
           mob_index[vch->mobdata->nr].virt == target)
          return 1;
      }
      return 0;
    }

  // search a room for a obj with vnum arg
  if ( !str_cmp( buf, "isobjvnuminroom" ) )
    {
      int target = atoi(arg);

      for(obj_data * obj = world[mob->in_room].contents;
          obj;
          obj = obj->next_content)
      {
        if(obj_index[obj->item_number].virt == target)
          return 1;
      }
      return 0;
    }

  if ( !str_cmp( buf, "cansee" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': return 1; // can always see holder
	case 'i': return 1;
	case 'n': if ( actor )
	             return CAN_SEE(mob, actor );
	          else return -1;
	case 't': if ( vict )
                     return CAN_SEE(mob, vict );
	          else return -1;
	case 'r': if ( rndm )
	             return CAN_SEE(mob, rndm );
	          else return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD, "Mob: %d bad argument to 'isnpc'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  // had done the quest 
  if ( !str_cmp( buf, "hasdonequest1" ) )
    {
      int target;
      if(!check_range_valid_and_convert(target, arg, 1, (1<<31))) {
        logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'hasdonequest1'", mob_index[mob->mobdata->nr].virt );
        return -1;
      }

      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'z': if (mob->beacon && !IS_NPC(((CHAR_DATA*)mob->beacon)))
		   return IS_SET(((CHAR_DATA*)mob->beacon)->pcdata->quest_bv1, (1<<(target)));
		else return -1;
	case 'n': if ( actor && !IS_NPC(actor) )
	            return IS_SET( actor->pcdata->quest_bv1, (1<<(target)));
	          else
		    return -1;
	case 't': if ( vict && !IS_NPC(vict) )
	            return IS_SET( vict->pcdata->quest_bv1, (1<<(target)));
	          else
		    return -1;
	case 'r': if ( rndm && !IS_NPC(rndm) )
	            return IS_SET( rndm->pcdata->quest_bv1, (1<<(target)));
	          else
		    return -1;
	default:
	  logf( IMMORTAL, LOG_WORLD,  "Mob: %d bad argument to 'hasdonequest1'", mob_index[mob->mobdata->nr].virt ); 
	  return -1;
	}
    }

  /* Ok... all the ifchcks are done, so if we didnt find ours then something
   * odd happened.  So report the bug and abort the MOBprogram (return error)
   */
  logf( IMMORTAL, LOG_WORLD,  "Mob: %d unknown ifchck", mob_index[mob->mobdata->nr].virt ); 
  return -1;

}
/* Quite a long and arduous function, this guy handles the control
 * flow part of MOBprograms.  Basicially once the driver sees an
 * 'if' attention shifts to here.  While many syntax errors are
 * caught, some will still get through due to the handling of break
 * and errors in the same fashion.  The desire to break out of the
 * recursion without catastrophe in the event of a mis-parse was
 * believed to be high. Thus, if an error is found, it is bugged and
 * the parser acts as though a break were issued and just bails out
 * at that point. I havent tested all the possibilites, so I'm speaking
 * in theory, but it is 'guaranteed' to work on syntactically correct
 * MOBprograms, so if the mud crashes here, check the mob carefully!
 */
// Null is kept globally so it doesn't go out of scope when we return in
// -pir 11/18/01
char null[ 1 ];
// Mprog_cur_result holds the current status of the mob that is acting.
// It will hold eCH_DIED if the mob died doing it's proc.  We need to 
// make sure this is not true when returning from an if check or the
// mob's pointer is no longer valid
int  mprog_cur_result;

char *mprog_process_if( char *ifchck, char *com_list, CHAR_DATA *mob,
		       CHAR_DATA *actor, OBJ_DATA *obj, void *vo,
		       CHAR_DATA *rndm )
{

 char buf[ MAX_INPUT_LENGTH ];
 char *morebuf = '\0';
 char    *cmnd = '\0';
 bool loopdone = FALSE;
 bool     flag = FALSE;
 int  legal;

 *null = '\0';

 /* check for trueness of the ifcheck */
 if ( ( legal = mprog_do_ifchck( ifchck, mob, actor, obj, vo, rndm ) ) )
   if ( legal >= 1 )
     flag = TRUE;
   else
     return null;

 while( loopdone == FALSE ) /*scan over any existing or statements */
 {
     cmnd     = com_list;
     com_list = mprog_next_command( com_list );
     while ( *cmnd == ' ' )
       cmnd++;
     if ( *cmnd == '\0' )
     {
	 logf( IMMORTAL, LOG_WORLD,  "Mob: %d no commands after IF/OR", mob_index[mob->mobdata->nr].virt ); 
	 return null;
     }
     morebuf = one_argument( cmnd, buf );
     if ( !str_cmp( buf, "or" ) )
     {
	 if ( ( legal = mprog_do_ifchck( morebuf,mob,actor,obj,vo,rndm ) ) )
	   if ( legal == 1 )
	     flag = TRUE;
	   else
	     return null;
     }
     else
       loopdone = TRUE;
 }
 
 if ( flag )
   for ( ; ; ) /*ifcheck was true, do commands but ignore else to endif*/ 
   {
       if ( !str_cmp( buf, "if" ) )
       { 
	   com_list = mprog_process_if(morebuf,com_list,mob,actor,obj,vo,rndm);
           if(IS_SET(mprog_cur_result, eCH_DIED))
             return null;
	   while ( *cmnd==' ' )
	     cmnd++;
	   if ( *com_list == '\0' )
	     return null;
	   cmnd     = com_list;
	   com_list = mprog_next_command( com_list );
	   morebuf  = one_argument( cmnd,buf );
	   continue;
       }
       if ( !str_cmp( buf, "break" ) )
	 return null;
       if ( !str_cmp( buf, "endif" ) )
	 return com_list; 
       if ( !str_cmp( buf, "else" ) ) 
       {
	   while ( str_cmp( buf, "endif" ) ) 
	   {
	       cmnd     = com_list;
	       com_list = mprog_next_command( com_list );
	       while ( *cmnd == ' ' )
		 cmnd++;
	       if ( *cmnd == '\0' )
	       {
		   logf( IMMORTAL, LOG_WORLD,  "Mob: %d missing endif after else",
			mob_index[mob->mobdata->nr].virt );
		   return null;
	       }
	       morebuf = one_argument( cmnd,buf );
	   }
	   return com_list; 
       }

       mprog_cur_result = mprog_process_cmnd( cmnd, mob, actor, obj, vo, rndm );
       if(IS_SET(mprog_cur_result, eCH_DIED))
         return null;

       cmnd     = com_list;
       com_list = mprog_next_command( com_list );
       while ( *cmnd == ' ' )
	 cmnd++;
       if ( *cmnd == '\0' )
       {
           logf( IMMORTAL, LOG_WORLD,  "Mob: %d missing else or endif", mob_index[mob->mobdata->nr].virt ); 
           return null;
       }
       morebuf = one_argument( cmnd, buf );
   }
 else /*false ifcheck, find else and do existing commands or quit at endif*/
   {
     int nest = 0;
     while ( true )
     { // Fix here 13/4 2004. Nested ifs are now taken into account.
	if ( !str_cmp(buf, "if")) nest++;
        if (!str_cmp(buf,"endif")) { if (nest == 0) break; else nest--; }
        if (!nest&&!str_cmp(buf,"else")) break;

	 cmnd     = com_list;
	 com_list = mprog_next_command( com_list );
	 while ( *cmnd == ' ' )
	   cmnd++;
	 if ( *cmnd == '\0' )
	   {
	     logf( IMMORTAL, LOG_WORLD,  "Mob: %d missing an else or endif",
		  mob_index[mob->mobdata->nr].virt ); 
	     return null;
	   }
	 morebuf = one_argument( cmnd, buf );
       }

     /* found either an else or an endif.. act accordingly */
     if ( !str_cmp( buf, "endif" ) )
       return com_list;
     cmnd     = com_list;
     com_list = mprog_next_command( com_list );
     while ( *cmnd == ' ' )
       cmnd++;
     if ( *cmnd == '\0' )
       { 
	 logf( IMMORTAL, LOG_WORLD,  "Mob: %d missing endif", mob_index[mob->mobdata->nr].virt ); 
	 return null;
       }
     morebuf = one_argument( cmnd, buf );
     
     for ( ; ; ) /*process the post-else commands until an endif is found.*/
       {
	 if ( !str_cmp( buf, "if" ) )
	   { 
	     com_list = mprog_process_if( morebuf, com_list, mob, actor,
					 obj, vo, rndm );
             if(IS_SET(mprog_cur_result, eCH_DIED))
               return null;
	     while ( *cmnd == ' ' )
	       cmnd++;
	     if ( *com_list == '\0' )
	       return null;
	     cmnd     = com_list;
	     com_list = mprog_next_command( com_list );
	     morebuf  = one_argument( cmnd,buf );
	     continue;
	   }
	 if ( !str_cmp( buf, "else" ) ) 
	   {
	     logf( IMMORTAL, LOG_WORLD,  "Mob: %d found else in an else section",
		  mob_index[mob->mobdata->nr].virt ); 
	     return null;
	   }
	 if ( !str_cmp( buf, "break" ) )
	   return null;
	 if ( !str_cmp( buf, "endif" ) )
	   return com_list; 

	 mprog_cur_result = mprog_process_cmnd( cmnd, mob, actor, obj, vo, rndm );
         if(IS_SET(mprog_cur_result, eCH_DIED))
           return null;

	 cmnd     = com_list;
	 com_list = mprog_next_command( com_list );
	 while ( *cmnd == ' ' )
	   cmnd++;
	 if ( *cmnd == '\0' )
	   {
	     logf( IMMORTAL, LOG_WORLD,  "Mob:%d missing endif in else section",
		  mob_index[mob->mobdata->nr].virt ); 
	     return null;
	   }
	 morebuf = one_argument( cmnd, buf );
       }
   }
}

/* This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object 
 * i.e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */
void mprog_translate( char ch, char *t, CHAR_DATA *mob, CHAR_DATA *actor,
                    OBJ_DATA *obj, void *vo, CHAR_DATA *rndm )
{
 static char *he_she        [] = { "it",  "he",  "she" };
 static char *him_her       [] = { "it",  "him", "her" };
 static char *his_her       [] = { "its", "his", "her" };
 CHAR_DATA   *vict             = (CHAR_DATA *) vo;
 OBJ_DATA    *v_obj            = (OBJ_DATA  *) vo;

 *t = '\0';
 switch ( ch ) {
     case 'i':
         one_argument( mob->name, t );
      break;
     case 'z':
	   if (mob->beacon){ one_argument(((CHAR_DATA*)mob->beacon)->name, t);
	break;}
	 strcpy(t,"error");
	break;
     case 'Z':
	  if (mob->beacon) strcpy(t, ((CHAR_DATA*)mob->beacon)->short_desc);
	  else strcpy(t,"error");
	break;	   
     case 'I':
         strcpy( t, mob->short_desc );
      break;
     case 'x':
          if (mob->beacon && ((CHAR_DATA*)mob->beacon)->fighting) one_argument(((CHAR_DATA*)mob->beacon)->fighting->name,t);
	  else
	  strcpy(t,"error");
	*t = UPPER(*t);
	break;
/*     case 'v':
	if (mob->tempVariable)
  	  strcpy(t, mob->tempVariable);
	else
	  strcpy(t,"");
	break;
     case 'V':
	if (actor)
	  if (actor->tempVariable)
	  {  strcpy(t,actor->tempVariable); break; }
	if (t && *t) *t = UPPER(*t);
	strcpy(t,"error");
	break;
     case 'w':
        if (vict)
          if (vict->tempVariable)
          {  strcpy(t, vict->tempVariable); break; }
        strcpy(t,"error");
        break;
     case 'W':
        if (rndm)
          if (rndm->tempVariable)
          {  strcpy(t,rndm->tempVariable); break; }
        strcpy(t,"error");
        break;
*/
     case 'n':
         if ( actor ) {
// Mobs can see them no matter what.  Use "cansee()" if you don't want that
//	   if ( CAN_SEE( mob,actor ) )
	     one_argument( actor->name, t );
           if ( !IS_NPC( actor ) )
             *t = UPPER( *t );
         }
         else logf(IMMORTAL, LOG_WORLD, "Mob %d trying illegal $ n in MOBProg.", mob_index[mob->mobdata->nr].virt);
      break;

     case 'N':
         if ( actor ) 
            if ( CAN_SEE( mob, actor ) )
	       if ( IS_NPC( actor ) )
		 strcpy( t, actor->short_desc );
	       else
	       {
		   strcpy( t, actor->name );
		   strcat( t, " " );
		   strcat( t, actor->title );
	       }
	    else
	      strcpy( t, "someone" );
         else logf(IMMORTAL, LOG_WORLD, "Mob %d trying illegal $ N in MOBProg.", mob_index[mob->mobdata->nr].virt);
	 break;

     case 't':
         if ( vict ) {
	   if ( CAN_SEE( mob, vict ) )
	     one_argument( vict->name, t );
           if ( !IS_NPC( vict ) )
             *t = UPPER( *t );
         }
         else logf(IMMORTAL, LOG_WORLD, "Mob %d trying illegal $ t in MOBProg.", mob_index[mob->mobdata->nr].virt);
	 break;

     case 'T':
         if ( vict ) 
            if ( CAN_SEE( mob, vict ) )
	       if ( IS_NPC( vict ) )
		 strcpy( t, vict->short_desc );
	       else
	       {
		 strcpy( t, vict->name );
		 strcat( t, " " );
		 strcat( t, vict->title );
	       }
	    else
	      strcpy( t, "someone" );
         else logf(IMMORTAL, LOG_WORLD, "Mob %d trying illegal $T in MOBProg.", mob_index[mob->mobdata->nr].virt);
	 break;
     
     case 'r':
         if ( rndm ) {
	   if ( CAN_SEE( mob, rndm ) )
	     one_argument( rndm->name, t );
           if ( !IS_NPC( rndm ) )
             *t = UPPER( *t );
         }
      break;

     case 'R':
         if ( rndm ) 
            if ( CAN_SEE( mob, rndm ) )
	       if ( IS_NPC( rndm ) )
		 strcpy(t,rndm->short_desc);
	       else
	       {
		 strcpy( t, rndm->name );
		 strcat( t, " " );
		 strcat( t, rndm->title );
	       }
	    else
	      strcpy( t, "someone" );
	 break;

     case 'e':
         if ( actor )
	   CAN_SEE( mob, actor ) ? strcpy( t, he_she[ actor->sex ] )
	                         : strcpy( t, "someone" );
	 break;
  
     case 'm':
         if ( actor )
	   CAN_SEE( mob, actor ) ? strcpy( t, him_her[ actor->sex ] )
                                 : strcpy( t, "someone" );
	 break;
  
     case 's':
         if ( actor )
	   CAN_SEE( mob, actor ) ? strcpy( t, his_her[ actor->sex ] )
	                         : strcpy( t, "someone's" );
	 break;
     
     case 'E':
         if ( vict )
	   CAN_SEE( mob, vict ) ? strcpy( t, he_she[ vict->sex ] )
                                : strcpy( t, "someone" );
	 break;
  
     case 'M':
         if ( vict )
	   CAN_SEE( mob, vict ) ? strcpy( t, him_her[ vict->sex ] )
                                : strcpy( t, "someone" );
	 break;
  
     case 'S':
         if ( vict )
	   CAN_SEE( mob, vict ) ? strcpy( t, his_her[ vict->sex ] )
                                : strcpy( t, "someone's" ); 
	 break;
     case 'q':
	char buf[15];
	sprintf(buf, "%d", mob_index[mob->mobdata->nr].virt);
	strcpy(t, buf);
	break;
     case 'j':
	 strcpy( t, he_she[ mob->sex ] );
	 break;
  
     case 'k':
	 strcpy( t, him_her[ mob->sex ] );
	 break;
  
     case 'l':
	 strcpy( t, his_her[ mob->sex ] );
	 break;

     case 'J':
         if ( rndm )
	   CAN_SEE( mob, rndm ) ? strcpy( t, he_she[ rndm->sex ] )
	                        : strcpy( t, "someone" );
	 break;
  
     case 'K':
         if ( rndm )
	   CAN_SEE( mob, rndm ) ? strcpy( t, him_her[ rndm->sex ] )
                                : strcpy( t, "someone" );
	 break;
  
     case 'L':
         if ( rndm )
	   CAN_SEE( mob, rndm ) ? strcpy( t, his_her[ rndm->sex ] )
	                        : strcpy( t, "someone's" );
	 break;

     case 'o':
         if ( obj )
	   CAN_SEE_OBJ( mob, obj ) ? one_argument( obj->name, t )
                                   : strcpy( t, "something" );
	 break;

     case 'O':
         if ( obj )
	   CAN_SEE_OBJ( mob, obj ) ? strcpy( t, obj->short_description )
                                   : strcpy( t, "something" );
	 break;

     case 'p':
         if ( v_obj )
	   CAN_SEE_OBJ( mob, v_obj ) ? one_argument( v_obj->name, t )
                                     : strcpy( t, "something" );
	 break;

     case 'P':
         if ( v_obj )
	   CAN_SEE_OBJ( mob, v_obj ) ? strcpy( t, v_obj->short_description )
                                     : strcpy( t, "something" );
      break;

     case 'a':
         if ( obj ) 
          switch ( *( obj->name ) )
	  {
	    case 'a': case 'e': case 'i':
            case 'o': case 'u': strcpy( t, "an" );
	      break;
            default: strcpy( t, "a" );
          }
	 break;

     case 'A':
         if ( v_obj ) 
          switch ( *( v_obj->name ) )
	  {
            case 'a': case 'e': case 'i':
	    case 'o': case 'u': strcpy( t, "an" );
	      break;
            default: strcpy( t, "a" );
          }
	 break;

     case '$':
         strcpy( t, "$" );
	 break;

     default:
         logf( IMMORTAL, LOG_WORLD, "Mob: %d bad $var", mob_index[mob->mobdata->nr].virt );
	 break;
       }

 return;

}



/* This procedure simply copies the cmnd to a buffer while expanding
 * any variables by calling the translate procedure.  The observant
 * code scrutinizer will notice that this is taken from act()
 */
int mprog_process_cmnd( char *cmnd, CHAR_DATA *mob, CHAR_DATA *actor,
			OBJ_DATA *obj, void *vo, CHAR_DATA *rndm )
{
  char buf[ MAX_INPUT_LENGTH*2 ];
  char tmp[ MAX_INPUT_LENGTH*2 ];
  char *str;
  char *i;
  char *point;

  point   = buf;
  str     = cmnd;

  while ( *str != '\0' )
  {
    if ( *str != '$' )
    {
      *point++ = *str++;
      continue;
    }
    str++;
    if (LOWER(*str) == 'v' || LOWER(*str) == 'w')
    {
      char a = *str;
      str++;
      if (*str == '[') 
      {
	char *tmp1 = &tmp[0];
	str++;
	while (*str != ']' && *str!='\0')
	  *tmp1++ = *str++;
        *tmp1 = '\0';
        CHAR_DATA *who;
	if (a == 'v') who = mob;
	else if (a == 'V') who = actor;
	else if (a == 'w') who = (CHAR_DATA*)vo;
	else if (a == 'W') who = rndm;
        if (who) {
	  struct tempvariable *eh = who->tempVariable;
	  for (; eh; eh = eh->next)
            if (!str_cmp(tmp, eh->name))
  	      break;
	 if (eh) {
	strcpy(tmp, eh->data);
	if (eh->data && *eh->data) *eh->data = UPPER(*eh->data);
	}
	 else continue; // Doesn't have the variable.
        }
      }
    } else
    mprog_translate( *str, tmp, mob, actor, obj, vo, rndm );
    i = tmp;
    ++str;
    while ( ( *point = *i ) != '\0' )
      ++point, ++i;
  }
  *point = '\0';

  if(strlen(buf) > MAX_INPUT_LENGTH-1)
    logf(IMMORTAL, LOG_WORLD, "Warning!  Mob '%s' has MobProg command longer than max input.", GET_NAME(mob));

  return command_interpreter( mob, buf, TRUE );
}

/* The main focus of the MOBprograms.  This routine is called 
 *  whenever a trigger is successful.  It is responsible for parsing
 *  the command list and figuring out what to do. However, like all
 *  complex procedures, everything is farmed out to the other guys.
 */
void mprog_driver ( char *com_list, CHAR_DATA *mob, CHAR_DATA *actor,
		   OBJ_DATA *obj, void *vo)
{

 char tmpcmndlst[ MAX_STRING_LENGTH ];
 char buf       [ MAX_INPUT_LENGTH ];
 char *morebuf;
 char *command_list;
 char *cmnd;
 CHAR_DATA *rndm  = NULL;
 CHAR_DATA *vch   = NULL;
 int        count = 0;
 if (IS_AFFECTED( mob, AFF_CHARM ))
   return;

 mprog_cur_result = eSUCCESS;

 if(!com_list) // this can happen when someone is editing
   return;
// int count2 = 0;
 // count valid random victs in room
 for ( vch = world[mob->in_room].people; vch; vch = vch->next_in_room )
   if (  CAN_SEE( mob, vch ) )
       count++;
  // else if (CAN_SEE(mob,vch))
   //    count2++;
 // count2 += count;
 if(count)
   count = number( 1, count );  // if we have valid victs, choose one
  /*if (count2) count2 = number(1,count2);
 if (count2)
  {
    for (vch = world[mob->in_room].people; vch && count2;)
    {
    if (CAN_SEE(mob,vch))
	count2--;
	if (count2) vch = vch->next_in_room;
    }
    //rndm2 = vch;
  }*/  
 if(count) 
 {
   for ( vch = world[mob->in_room].people; vch && count; )
   {
     if ( CAN_SEE( mob, vch ) )
       count--;
     if (count) vch = vch->next_in_room;
  }
   rndm = vch;
 }

 strcpy( tmpcmndlst, com_list );
 command_list = tmpcmndlst;
 cmnd         = command_list;
 command_list = mprog_next_command( command_list );
 while ( *cmnd != '\0' )
   {
     morebuf = one_argument( cmnd, buf );
     if ( !str_cmp( buf, "if" ) ) {
       command_list = mprog_process_if( morebuf, command_list, mob,
				       actor, obj, vo, rndm );
       if(IS_SET(mprog_cur_result, eCH_DIED))
         return;
     }
     else {
       mprog_cur_result = mprog_process_cmnd( cmnd, mob, actor, obj, vo, rndm );
       if(IS_SET(mprog_cur_result, eCH_DIED))
         return;
     }
     cmnd         = command_list;
     command_list = mprog_next_command( command_list );
   }
 return;

}

/***************************************************************************
 * Global function code and brief comments.
 */

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
// Returns TRUE if match
// FALSE if no match
int mprog_wordlist_check( char *arg, CHAR_DATA *mob, CHAR_DATA *actor,
			  OBJ_DATA *obj, void *vo, int type )
{

  char        temp1[ MAX_STRING_LENGTH ];
  char        temp2[ MAX_INPUT_LENGTH ];
  char        word[ MAX_INPUT_LENGTH ];
  MPROG_DATA *mprg;
  MPROG_DATA *next;
  char       *list;
  char       *start;
  char       *dupl;
  char       *end;
  int         i;
  int         retval = 0;
  bool done = FALSE;
//  for ( mprg = mob_index[mob->mobdata->nr].mobprogs; mprg != NULL; mprg 
//= next )
 mprg = mob_index[mob->mobdata->nr].mobprogs;
 if (!mprg) { done = TRUE; mprg = mob_index[mob->mobdata->nr].mobspec; }
 for ( ; mprg != NULL; mprg = next )
  {
    next = mprg->next;
    if ( mprg->type & type )
      {
	strcpy( temp1, mprg->arglist );
	list = temp1;
	for ( i = 0; i < (signed) strlen( list ); i++ )
	  list[i] = LOWER( list[i] );
	strcpy( temp2, arg );
	dupl = temp2;
	for ( i = 0; i < (signed) strlen( dupl ); i++ )
	  dupl[i] = LOWER( dupl[i] );
	if ( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
	  {
	    list += 2;
	    while ( ( start = strstr( dupl, list ) ) )
	      if ( (start == dupl || *(start-1) == ' ' )
		  && ( *(end = start + strlen( list ) ) == ' '
		      || *end == '\n'
		      || *end == '\r'
		      || *end == '\0'
                      // allow punctuation at the end
                      || *end == '.'
                      || *end == '?'
                      || *end == '!' ) )
		{
                  retval = 1;
		  mprog_driver( mprg->comlist, mob, actor, obj, vo );
		  break;
		}
	      else
		dupl = start+1;
	  }
	else
	  {
	    list = one_argument( list, word );
	    for( ; word[0] != '\0'; list = one_argument( list, word ) )
	      while ( ( start = strstr( dupl, word ) ) )
		if ( ( start == dupl || *(start-1) == ' ' )
		    && ( *(end = start + strlen( word ) ) == ' '
			|| *end == '\n'
			|| *end == '\r'
			|| *end == '\0'
                        // allow punctuation at the end
                        || *end == '.'
                        || *end == '?'
                        || *end == '!' ) )
		  {
                    retval = 1;
		    mprog_driver( mprg->comlist, mob, actor, obj, vo );
		    break;
		  }
		else
		  dupl = start+1;
	  }
      }
     if (next == NULL && !done) { done = TRUE;
       next = mob_index[mob->mobdata->nr].mobspec; }
    }
  return retval;

}

void mprog_percent_check( CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *obj,
			 void *vo, int type)
{
 MPROG_DATA * mprg;
 MPROG_DATA *next;
 bool done = FALSE;
 mprg = mob_index[mob->mobdata->nr].mobprogs;
 if (!mprg) { done = TRUE; mprg = mob_index[mob->mobdata->nr].mobspec; }
 for ( ; mprg != NULL; mprg = next )
 {
  next = mprg->next;
   if ( ( mprg->type & type )
       && ( number(0, 99) < atoi( mprg->arglist ) ) )
     {
       mprog_driver( mprg->comlist, mob, actor, obj, vo );
       if ( type != GREET_PROG && type != ALL_GREET_PROG )
	 break;
     }
   if (!next && !done){done = TRUE; next = mob_index[mob->mobdata->nr].mobspec; }
 }
 return;

}

/* The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
void mprog_act_trigger( char *buf, CHAR_DATA *mob, CHAR_DATA *ch,
		       OBJ_DATA *obj, void *vo)
{

//  MPROG_ACT_LIST * tmp_act;
  //MPROG_ACT_LIST * curr;
//  MPROG_DATA *mprg;
 
  if(!MOBtrigger)
    return;

  if ( IS_NPC( mob )
      && ( mob_index[mob->mobdata->nr].progtypes & ACT_PROG ) )
             mprog_wordlist_check( buf, mob, ch,
                       obj, vo, ACT_PROG );

/* Why oh why was it like this? They can add lag themselves if needed.
  if 
( IS_NPC( mob )
      && ( mob_index[mob->mobdata->nr].progtypes & ACT_PROG ) )
    {
#ifdef LEAK_CHECK
      tmp_act = (MPROG_ACT_LIST *) calloc( 1, sizeof( MPROG_ACT_LIST ) );
#else
      tmp_act = (MPROG_ACT_LIST *) dc_alloc( 1, sizeof( MPROG_ACT_LIST ) );
#endif

      if(!mob->mobdata->mpact)
        mob->mobdata->mpact = tmp_act;
      else {
        curr = mob->mobdata->mpact;
        while(curr->next)
          curr = curr->next;
        curr->next = tmp_act;
      }

      tmp_act->next = NULL;
      tmp_act->buf = str_dup( buf );
      tmp_act->ch  = ch; 
      tmp_act->obj = obj; 
      tmp_act->vo  = vo; 
      mob->mobdata->mpactnum++;

    }
*/  return;

}

int mprog_bribe_trigger( CHAR_DATA *mob, CHAR_DATA *ch, int amount )
{

  MPROG_DATA *mprg;
  MPROG_DATA *next;
  OBJ_DATA   *obj;
  bool done = FALSE;
  if ( IS_NPC( mob )
      && ( mob_index[mob->mobdata->nr].progtypes & BRIBE_PROG ) )
    {
      mob->gold -= amount;

	 mprg = mob_index[mob->mobdata->nr].mobprogs;
	 if (!mprg) { done = TRUE; mprg = mob_index[mob->mobdata->nr].mobspec; }
	 for ( ; mprg != NULL; mprg = next )
	{
	next = mprg->next;
	if ( ( mprg->type & BRIBE_PROG )
	    && ( amount >= atoi( mprg->arglist ) ) )
	  {
	    mprog_driver( mprg->comlist, mob, ch, obj, NULL );
	    break;
	  }
	   if (!next && !done) { next = mob_index[mob->mobdata->nr].mobspec; done = TRUE;}
	}
    }
  
  return mprog_cur_result;

}

int mprog_death_trigger( CHAR_DATA *mob, CHAR_DATA *killer )
{

 if ( IS_NPC( mob )
     && ( mob_index[mob->mobdata->nr].progtypes & DEATH_PROG ) )
   {
     mprog_percent_check( mob, killer, NULL, NULL, DEATH_PROG );
   }
 if (!SOMEONE_DIED(mprog_cur_result))
  death_cry( mob );
 return mprog_cur_result;

}

int mprog_entry_trigger( CHAR_DATA *mob )
{

 if ( IS_NPC( mob )
     && ( mob_index[mob->mobdata->nr].progtypes & ENTRY_PROG ) )
   mprog_percent_check( mob, NULL, NULL, NULL, ENTRY_PROG );

 return mprog_cur_result;

}

int mprog_fight_trigger( CHAR_DATA *mob, CHAR_DATA *ch )
{

 if ( IS_NPC( mob )
 && MOB_WAIT_STATE(mob) <= 0
     && ( mob_index[mob->mobdata->nr].progtypes & FIGHT_PROG ) )
   mprog_percent_check( mob, ch, NULL, NULL, FIGHT_PROG );

 return mprog_cur_result;

}

int mprog_attack_trigger( CHAR_DATA *mob, CHAR_DATA *ch )
{

 if ( IS_NPC( mob )
     && ( mob_index[mob->mobdata->nr].progtypes & ATTACK_PROG ) )
   mprog_percent_check( mob, ch, NULL, NULL, ATTACK_PROG );

 return mprog_cur_result;

}

int mprog_give_trigger( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj )
{

 char        buf[MAX_INPUT_LENGTH];
 MPROG_DATA *mprg;
 MPROG_DATA *next;
 bool done = FALSE;
 if ( IS_NPC( mob )
     && ( mob_index[mob->mobdata->nr].progtypes & GIVE_PROG ) )
{
 mprg = mob_index[mob->mobdata->nr].mobprogs;
 if (!mprg) { done = TRUE; mprg = mob_index[mob->mobdata->nr].mobspec; }
 for ( ; mprg != NULL; mprg = next )
     {
	next = mprg->next;
       one_argument( mprg->arglist, buf );
       if ( ( mprg->type & GIVE_PROG )
	   && ( ( !str_cmp( obj->name, mprg->arglist ) )
	       || ( !str_cmp( "all", buf ) ) ) )
	 {
	   mprog_driver( mprg->comlist, mob, ch, obj, NULL );
	   break;
	 }
     if (!next && !done) { done = TRUE; next = mob_index[mob->mobdata->nr].mobspec;}
     }
 }
 return mprog_cur_result;

}

int mprog_greet_trigger( CHAR_DATA *ch )
{

 CHAR_DATA *vmob;

 mprog_cur_result = eSUCCESS;

 for ( vmob = world[ch->in_room].people; vmob != NULL; vmob = vmob->next_in_room )
   if ( IS_NPC( vmob )
       && ( vmob->fighting == NULL )
       && AWAKE( vmob ) )
   {
     if ( ch != vmob
       && CAN_SEE( vmob, ch )
       && ( mob_index[vmob->mobdata->nr].progtypes & GREET_PROG) )
       mprog_percent_check( vmob, ch, NULL, NULL, GREET_PROG );
     else
        if( ( mob_index[vmob->mobdata->nr].progtypes & ALL_GREET_PROG ) )
         mprog_percent_check(vmob,ch,NULL,NULL,ALL_GREET_PROG);
     
     if(SOMEONE_DIED(mprog_cur_result))
       break;
   }
 return mprog_cur_result;

}

int mprog_hitprcnt_trigger( CHAR_DATA *mob, CHAR_DATA *ch)
{

 MPROG_DATA *mprg;
 MPROG_DATA *next;
  bool done = FALSE;
 if ( IS_NPC( mob )
 	&& MOB_WAIT_STATE(mob) <= 0
      && ( 
mob_index[mob->mobdata->nr].progtypes & HITPRCNT_PROG ) )
  {
 mprg = mob_index[mob->mobdata->nr].mobprogs;
 if (!mprg) { done = TRUE; mprg = mob_index[mob->mobdata->nr].mobspec; }
 for ( ; mprg != NULL; mprg = next )
   {
     next = mprg->next;
     if ( ( mprg->type & HITPRCNT_PROG )
	 && ( ( 100*mob->hit / mob->max_hit ) < atoi( mprg->arglist ) ) )
       {
	 mprog_driver( mprg->comlist, mob, ch, NULL, NULL );
	 break;
       }
    if (!next && !done){done = TRUE; next = mob_index[mob->mobdata->nr].mobspec; }
 }
}
 return mprog_cur_result;

}

int mprog_random_trigger( CHAR_DATA *mob )
{
  mprog_cur_result = eSUCCESS;

  if ( mob_index[mob->mobdata->nr].progtypes & RAND_PROG)
    mprog_percent_check(mob,NULL,NULL,NULL,RAND_PROG);

  return mprog_cur_result;

}

int mprog_load_trigger(CHAR_DATA *mob)
{
  mprog_cur_result = eSUCCESS;
  if (mob_index[mob->mobdata->nr].progtypes & LOAD_PROG)
     mprog_percent_check(mob, NULL, NULL, NULL, LOAD_PROG);
  return mprog_cur_result;
}

int mprog_arandom_trigger( CHAR_DATA *mob)
{
  mprog_cur_result = eSUCCESS;
  if (mob_index[mob->mobdata->nr].progtypes & ARAND_PROG)
     mprog_percent_check(mob,NULL,NULL,NULL,ARAND_PROG);
   return mprog_cur_result;
}
 

int mprog_speech_trigger( char *txt, CHAR_DATA *mob )
{

  CHAR_DATA *vmob;

  mprog_cur_result = eSUCCESS;

  for ( vmob = world[mob->in_room].people; vmob != NULL; vmob = vmob->next_in_room )
    if ( IS_NPC( vmob ) && ( mob_index[vmob->mobdata->nr].progtypes & SPEECH_PROG ) )
    {
      if(mprog_wordlist_check( txt, vmob, mob, NULL, NULL, SPEECH_PROG ))
        break;
    }
  
  return mprog_cur_result;

}

int mprog_catch_trigger(char_data * mob, int catch_num, char *var, int opt)
{
 MPROG_DATA *mprg;
 MPROG_DATA *next;
 int curr_catch;
 bool done = FALSE;
 mprog_cur_result = eFAILURE;

 if ( IS_NPC( mob )
     && ( mob_index[mob->mobdata->nr].progtypes & CATCH_PROG ) )
 {
 mprg = mob_index[mob->mobdata->nr].mobprogs;
 if (!mprg || opt & 1) { done = TRUE; mprg = mob_index[mob->mobdata->nr].mobspec; }

 for ( ; mprg != NULL; mprg = next )
     {
	next = mprg->next;
       if ( mprg->type & CATCH_PROG )
       {
         if(!check_range_valid_and_convert(curr_catch, mprg->arglist, MPROG_CATCH_MIN, MPROG_CATCH_MAX)) {
           logf( IMMORTAL, LOG_WORLD, "Invalid catch argument: vnum %d", 
             mob_index[mob->mobdata->nr].virt);
           return eFAILURE;
         }
         if(curr_catch == catch_num) {
	  if (var) {
		struct tempvariable *eh;
		for (eh = mob->tempVariable; eh; eh = eh->next)
		{
		  if (!str_cmp(eh->name, "throw")) break;
		}
		if (eh) {
		  dc_free(eh->data);
		  eh->data = var;
		} else {
#ifdef LEAK_CHECK
        	eh = (struct tempvariable *)
                        calloc(1, sizeof(struct tempvariable));
#else
	       eh = (struct tempvariable *)
                        dc_alloc(1, sizeof(struct tempvariable));
#endif

	     eh->data = var;
	     eh->name = str_dup("throw");
	   	  eh->next = mob->tempVariable;
    		 mob->tempVariable = eh;

		}
	  }
           mprog_driver( mprg->comlist, mob, NULL, NULL, NULL );
           break;
         }
       }
	if (!next && !done){done = TRUE; next = mob_index[mob->mobdata->nr].mobspec;}

     }
}
 return mprog_cur_result;
}

void update_mprog_throws()
{
   struct mprog_throw_type * curr;
   struct mprog_throw_type * action;
   struct mprog_throw_type * last = NULL;
   char_data * vict;
   obj_data *vobj;
   for(curr = g_mprog_throw_list; curr; )
   {
      // update
      if(curr->delay > 0) {
        curr->delay--;
        last = curr;
        curr = curr->next;
        continue;
      }
	vobj = NULL;
      vict = NULL;
	if (curr->mob) {
      // find target
      if(*curr->target_mob_name) { // find me by name
        vict = get_mob(curr->target_mob_name);
      }
      else {                 // find me by num
        vict = get_mob_vnum(curr->target_mob_num);
      }
	} else {
	if (*curr->target_mob_name)
	vobj = get_obj(curr->target_mob_name);
	else vobj = get_obj_vnum(curr->target_mob_num);
       }
      // remove from list
      if(last) {
        last->next = curr->next;
        action = curr;
        curr = last->next;
        // last doesn't move
      }
      else {
        g_mprog_throw_list = curr->next;
        action = curr;
        curr = g_mprog_throw_list;
      }

      // This is done this way in case the 'catch' does a 'throw' inside of it

      // if !vict, oh well....remove it anyway.  Someone killed him.
      if(vict)  // activate
        mprog_catch_trigger(vict, action->data_num, action->var,action->opt);
      else if (vobj)
	oprog_catch_trigger(vobj, action->data_num, action->var,action->opt);
      dc_free(action);
   }
};

CHAR_DATA *initiate_oproc(CHAR_DATA *ch, OBJ_DATA *obj)
{ // Sneakiness.
  CHAR_DATA *temp;
  temp = clone_mobile(real_mobile(12));
  mob_index[real_mobile(12)].mobprogs = obj_index[obj->item_number].mobprogs;
  mob_index[real_mobile(12)].progtypes = obj_index[obj->item_number].progtypes;

  if (ch)    char_to_room(temp, ch->in_room);
  else       char_to_room(temp,obj->in_room);
  if (ch) temp->beacon = (OBJ_DATA*) ch;
  temp->mobdata->last_room = (int)obj;
//  temp->master = ch;
 // dc_free(temp->short_desc);
  temp->short_desc = str_hsh(obj->short_description);
  //dc_free(temp->name);
  char buf[MAX_STRING_LENGTH];
  sprintf(buf, "%s", obj->name);
  for (int i = strlen(buf)-1;i > 0;i--)
   if (buf[i] == ' ')
   { buf[i] = '\0'; break; }
  temp->name = str_hsh(buf);
  
  return temp;
}

void end_oproc(CHAR_DATA *ch)
{
  extract_char(ch, TRUE);
  mob_index[real_mobile(12)].progtypes = 0;
  mob_index[real_mobile(12)].mobprogs = 0;
}


int oprog_speech_trigger( char *txt, CHAR_DATA *ch )
{
  CHAR_DATA *vmob = NULL;
  OBJ_DATA *item;

  mprog_cur_result = eSUCCESS;


  for (item = world[ch->in_room].contents; item; item = item->next_content)
     if (obj_index[item->item_number].progtypes & SPEECH_PROG)
     {
	vmob = initiate_oproc(ch, item);
	if (mprog_wordlist_check(txt, vmob, ch, NULL, NULL, SPEECH_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }
  for (item = ch->carrying; item; item = item->next_content)
     if (obj_index[item->item_number].progtypes & SPEECH_PROG)
     {
	vmob = initiate_oproc(ch, item);
	if (mprog_wordlist_check(txt, vmob, ch, NULL, NULL, SPEECH_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }

  for (int i = 0; i < MAX_WEAR; i++)
  if (ch->equipment[i])
     if (obj_index[ch->equipment[i]->item_number].progtypes & SPEECH_PROG)
     {
	vmob = initiate_oproc(ch, ch->equipment[i]);
	if (mprog_wordlist_check(txt, vmob, ch,  NULL, NULL, SPEECH_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }
  return mprog_cur_result;
}


int oprog_catch_trigger(obj_data *obj, int catch_num, char *var, int opt)
{
 MPROG_DATA *mprg;
 int curr_catch;
 mprog_cur_result = eFAILURE;
 CHAR_DATA *vmob;
 if ( obj_index[obj->item_number].progtypes & CATCH_PROG ) 
 {
    mprg = obj_index[obj->item_number].mobprogs;
    for ( ; mprg != NULL; mprg = mprg->next )
     {
       if ( mprg->type & CATCH_PROG )
       {
         if(!check_range_valid_and_convert(curr_catch, mprg->arglist, MPROG_CATCH_MIN, MPROG_CATCH_MAX)) {
           logf( IMMORTAL, LOG_WORLD, "Invalid catch argument: vnum %d",
             obj_index[obj->item_number].virt);
           return eFAILURE;
         }
         if(curr_catch == catch_num) {
	   vmob = initiate_oproc(NULL, obj);
           if (var) {
	     struct tempvariable *eh;
	     
#ifdef LEAK_CHECK
     		eh = (struct tempvariable *)
                        calloc(1, sizeof(struct tempvariable));
#else
	       eh = (struct tempvariable *)
                        dc_alloc(1, sizeof(struct tempvariable));
#endif

	     eh->data = var;
	     eh->name = str_dup("throw");
	     eh->next = vmob->tempVariable;
	     vmob->tempVariable = eh;
           }

           mprog_driver( mprg->comlist, vmob, NULL, NULL, NULL );
	   end_oproc(vmob);
	   break;
         }
       }

     }
}
 return mprog_cur_result;
}


int oprog_act_trigger( char *txt, CHAR_DATA *ch )
{

  CHAR_DATA *vmob;
  OBJ_DATA *item;

  mprog_cur_result = eSUCCESS;


  for (item = world[ch->in_room].contents; item; item = 
item->next_content)
     if (obj_index[item->item_number].progtypes & ACT_PROG)
     {
	vmob = initiate_oproc(ch, item);
	if (mprog_wordlist_check(txt, vmob, ch, NULL, NULL, ACT_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }
  for (item = ch->carrying; item; item = item->next_content)
     if (obj_index[item->item_number].progtypes & ACT_PROG)
     {
	vmob = initiate_oproc(ch, item);
	if (mprog_wordlist_check(txt, vmob, ch, NULL, NULL, ACT_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }

  for (int i = 0; i < MAX_WEAR; i++)
  if (ch->equipment[i])
     if (obj_index[ch->equipment[i]->item_number].progtypes & ACT_PROG)
     {
	vmob = initiate_oproc(ch, ch->equipment[i]);
	if (mprog_wordlist_check(txt, vmob,ch,  NULL, NULL, ACT_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }
  return mprog_cur_result;
}

int oprog_greet_trigger( CHAR_DATA *ch )
{

  CHAR_DATA *vmob;
  OBJ_DATA *item;

  mprog_cur_result = eSUCCESS;


  for (item = world[ch->in_room].contents; item; item = 
item->next_content)
     if (obj_index[item->item_number].progtypes & ALL_GREET_PROG)
     {
	vmob = initiate_oproc(ch, item);
	mprog_percent_check(vmob, ch, item, NULL, ALL_GREET_PROG);
	end_oproc(vmob);
        return mprog_cur_result;
     }
  return mprog_cur_result;
}

int oprog_rand_trigger( OBJ_DATA *item )
{

  CHAR_DATA *vmob;
//  OBJ_DATA *item;
  CHAR_DATA *ch;
  mprog_cur_result = eSUCCESS;
  if (item->carried_by) ch=item->carried_by;
  else ch = NULL;
    if (obj_index[item->item_number].progtypes & RAND_PROG)
     {
        vmob = initiate_oproc(ch, item);
        mprog_percent_check(vmob, ch, item, NULL, RAND_PROG);
        end_oproc(vmob);
        return mprog_cur_result;
     }
 return mprog_cur_result;
}

int oprog_load_trigger( CHAR_DATA *ch )
{

  CHAR_DATA *vmob;
  OBJ_DATA *item;

  mprog_cur_result = eSUCCESS;


  for (item = world[ch->in_room].contents; item; item = 
item->next_content)
     if (obj_index[item->item_number].progtypes & LOAD_PROG)
     {
	vmob = initiate_oproc(ch, item);
	mprog_percent_check(vmob, ch, item, NULL, LOAD_PROG);
	end_oproc(vmob);
        return mprog_cur_result;
     }
  for (item = ch->carrying; item; item = item->next_content)
     if (obj_index[item->item_number].progtypes & LOAD_PROG)
     {
	vmob = initiate_oproc(ch, item);
	mprog_percent_check(vmob, ch, item, NULL, LOAD_PROG);
	end_oproc(vmob);
        return mprog_cur_result;
     }

  for (int i = 0; i < MAX_WEAR; i++)
  if (ch->equipment[i])
     if (obj_index[ch->equipment[i]->item_number].progtypes & LOAD_PROG)
     {
	vmob = initiate_oproc(ch, item);
	mprog_percent_check(vmob, ch, item, NULL, LOAD_PROG);
	end_oproc(vmob);
        return mprog_cur_result;
     }
  return mprog_cur_result;
}


int oprog_weapon_trigger( CHAR_DATA *ch, OBJ_DATA *item )
{

  CHAR_DATA *vmob;

  mprog_cur_result = eSUCCESS;

     if (obj_index[item->item_number].progtypes & WEAPON_PROG)
     {
	vmob = initiate_oproc(ch, item);
	mprog_percent_check(vmob, ch, item, NULL, WEAPON_PROG);
	end_oproc(vmob);
        return mprog_cur_result;
     }

   return mprog_cur_result;
}

int oprog_armour_trigger( CHAR_DATA *ch, OBJ_DATA *item )
{

  CHAR_DATA *vmob;

  mprog_cur_result = eSUCCESS;

     if (obj_index[item->item_number].progtypes & ARMOUR_PROG)
     {
	vmob = initiate_oproc(ch, item);
	mprog_percent_check(vmob, ch, item, NULL, ARMOUR_PROG);
	end_oproc(vmob);
        return mprog_cur_result;
     }

   return mprog_cur_result;
}

int oprog_command_trigger( char *txt, CHAR_DATA *ch )
{

  CHAR_DATA *vmob;
  OBJ_DATA *item;

  mprog_cur_result = eSUCCESS;


  for (item = world[ch->in_room].contents; item; item = 
item->next_content)
     if (obj_index[item->item_number].progtypes & COMMAND_PROG)
     {
	vmob = initiate_oproc(ch, item);
	if (mprog_wordlist_check(txt, ch, vmob, NULL, NULL, COMMAND_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }
  for (item = ch->carrying; item; item = item->next_content)
     if (obj_index[item->item_number].progtypes & COMMAND_PROG)
     {
	vmob = initiate_oproc(ch, item);
	if (mprog_wordlist_check(txt, ch, vmob, NULL, NULL, COMMAND_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }

  for (int i = 0; i < MAX_WEAR; i++)
  if (ch->equipment[i])
     if (obj_index[ch->equipment[i]->item_number].progtypes & 
COMMAND_PROG)
     {
	vmob = initiate_oproc(ch, ch->equipment[i]);
	if (mprog_wordlist_check(txt, ch, vmob, NULL, NULL, COMMAND_PROG))
        {
 	  end_oproc(vmob);
	  return mprog_cur_result;
        }
	end_oproc(vmob);
     }
  return mprog_cur_result;
}

