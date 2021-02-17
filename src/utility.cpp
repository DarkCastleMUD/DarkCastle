
/***************************************************************************
 *  file: utility.c, Utility module.                       Part of DIKUMUD *
 *  Usage: Utility procedures                                              *
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
 *                                                                         *
 * Revision History                                                        *
 * 10/18/2003   Onager     Changed CAN_SEE() to hide Onager from everyone  *
 *                         except Pir and Valk                             *
 * 10/19/2003   Onager     Took out super-secret hidey code from CAN_SEE() *
 ***************************************************************************/
/* $Id: utility.cpp,v 1.129 2014/07/04 22:00:04 jhhudso Exp $ */

extern "C"
{
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include <iostream>
#include <sstream>
#include <map>
#include <fmt/format.h>
#include <algorithm>

#include "innate.h"
#include "structs.h"
#include "levels.h"
#include "player.h"
#include "timeinfo.h"
#include "character.h"
#include "utility.h"
#include "room.h"
#include "obj.h"
#include "interp.h"
#include "fileinfo.h"
#include "mobile.h"
#include "handler.h"
#include "db.h"
#include "connect.h"
#include "act.h"
#include "spells.h"
#include "clan.h"
#include "fight.h"
#include "returnvals.h"
#include "set.h"
#include "DC.h"
#include "const.h"

using namespace std;

#ifndef GZIP
  #define GZIP "gzip"
#endif

extern std::map<int, std::map<uint8_t, std::string> > professions;

// extern funcs
clan_data * get_clan(char_data *);
void release_message(CHAR_DATA *ch);

// local vars
char    log_buf[MAX_STRING_LENGTH];
struct timer_data *timer_list = NULL;

// local funcs
void update_wizlist(CHAR_DATA *ch);


size_t nocolor_strlen(const char *s)
{
    if (!s) {
	return 0;
    }

    size_t len = 0;

    while (*s != '\0') {
	if (*s == '$') {
	    s++;
	    if ((*s <= '9' && *s >= '0') ||
		*s == 'I' ||
		*s == 'L' || 
		*s == '*' ||
		*s == 'R' ||
		*s == 'B')
	    {
		// Do nothing
	    } else if (*s == '$') {
		len++;
	    } else if (*s == '\0') {
		len++;
		break;
	    } else {
		len+=2;
	    }
	    s++;
	} else {
	    len++;
	    s++;
	}
    }

    return len;
}

// This function is like str_dup except it returns 0 if passed 0
char *str_dup0(const char *str)
{
  if (str == 0) {
    return 0;
  }

  return str_dup(str);
}

// duplicate a string with it's own memory
char *str_dup( const char *str )
{
    char *str_new = 0;

#ifdef LEAK_CHECK
    str_new = (char *)calloc(strlen(str) + 1, sizeof(char));
#else
    str_new = (char *)dc_alloc(strlen(str) + 1, sizeof(char));
#endif

    if(!str_new)
    {
      fprintf(stderr, "NO MEMORY DUPLICATING STRING!");
      abort();
    }
    strcpy( str_new, str );
    return str_new;
}

#ifdef WIN32
char *index(char *buf, char op)
{
    int i = 0;
    
    while(buf[i] != 0)
    {
        if(buf[i] == op)
        {
            return(buf + i);
        }
        i++;
    }
    return(NULL);
}
#endif

// generate a (relatively) random number.
int number_old( int from, int to )
{
    if ( from >= to )
	return from;

    return (from + (random() % (to - from + 1)));
}

// simulates a dice roll
// basically we assign the total to the number of dice (since you always
// roll at least a one with each die) then add a random MOD of the die
// size.  ie, 4d10 would be 4 + loop*4 (0-9)
int dice( int num, int size )
{
   int r;
   int sum = 0; 

   if(size < 1)
      return 1;
  
   for(r = 1; r <= num; r++)
      sum += number(1,size);

   return sum;
}

// compare strings but ignore case (unlike strcmp)
int str_cmp( const char *arg1, const char *arg2 )
{
    int check, i;

    assert(arg1 && arg2);

    if(!arg1 || !arg2) {
      log("NULL args sent to str_cmp in utility.c!", ANGEL, LOG_BUG);
      return 0;
    }

    for ( i = 0; arg1[i] || arg2[i]; i++ )
    {
	check = LOWER(arg1[i]) - LOWER(arg2[i]);
	if ( check < 0 )
	    return -1;
	if ( check > 0 )
	    return 1;
    }

    return 0;
}

char *str_nospace(const char *stri)
{
  if (!stri) return "";

  char *stri_new = str_dup(stri);
  int i = 0;

  while (*(stri+i))
  {
     if (*(stri+i) == ' ')
       stri_new[i] = '_';
     i++;
  }
  return stri_new; // Must be freed by caller to avoid memory leak
}

// compare strings but ignore case and change all spaces to underscores
int str_nosp_cmp( const char *arg1, const char *arg2 )
{
  char *tmp_arg1 = str_nospace(arg1);
  char *tmp_arg2 = str_nospace(arg2);
  int retval = str_cmp(tmp_arg1, tmp_arg2);
  dc_free(tmp_arg2);
  dc_free(tmp_arg1);
  
  return retval;
}

int str_n_nosp_cmp( const char *arg1, const char *arg2, int size)
{
  char *tmp_arg1 = str_nospace(arg1);
  char *tmp_arg2 = str_nospace(arg2);
  int retval = strncasecmp(tmp_arg1, tmp_arg2, size);
  dc_free(tmp_arg2);
  dc_free(tmp_arg1);
  
  return retval;
}

// TODO - Declare these in a more appropriate place
FILE * bug_log = 0;
FILE * god_log = 0;
FILE * mortal_log = 0;
FILE * socket_log = 0;
FILE * player_log = 0;
FILE * world_log   = 0;
FILE * arena_log   = 0;
FILE * clan_log   = 0;
FILE * objects_log = 0;
FILE * quest_log = 0;

void log(const char *str, int god_level, long type)
{
  log(str, god_level, type, nullptr);
}

// writes a string to the log 
void log(const char *str, int god_level, long type, char_data *vict)
{
  FILE **f = 0;
  int stream = 1;
  stringstream logpath;
  DC &dc = DC::instance();
  DC::config &cf = dc.cf;

  if (DC::instance().cf.bport)
  {
    logpath << "../blog/";
  } else
  {
    logpath << "../log/";
  }

  switch (type) {
  default:
    stream = 0;
    break;
  case LOG_BUG:
    f = &bug_log;
    logpath << BUG_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open bug log.\n");
      exit(1);
    }

    // TODO - need some sort of thing to automatically have bugs switch from file to
    //        to non-file when we're up in gdb

    //stream = 0;
    // I want bugs to be right in the gdblog.
    // -Sadus

    break;
  case LOG_GOD:
    f = &god_log;
    logpath << GOD_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open god log.\n");
      exit(1);
    }
    break;
  case LOG_MORTAL:
    f = &mortal_log;
    logpath << MORTAL_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open mortal log.\n");
      exit(1);
    }
    break;
  case LOG_SOCKET:
    f = &socket_log;
    logpath << SOCKET_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open socket log: %s\n", logpath.str().c_str());
      exit(1);
    }
    break;
  case LOG_PLAYER:
    f = &player_log;
    if (vict && vict->name)
    {
      logpath << PLAYER_DIR << vict->name;
      if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
      {
        fprintf(stderr, "Unable to open player log '%s'.\n", logpath.str().c_str());
      }
    } else
    {
      logpath << PLAYER_LOG;
      if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
      {
        fprintf(stderr, "Unable to open player log.\n");
      }
    }
    break;
  case LOG_WORLD:
    f = &world_log;
    logpath << WORLD_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open world log.\n");
      exit(1);
    }
    break;
  case LOG_ARENA:
    f = &arena_log;
    logpath << ARENA_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open arena log.\n");
      exit(1);
    }
    break;
  case LOG_CLAN:
    f = &clan_log;
    logpath << CLAN_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open clan log.\n");
      exit(1);
    }
    break;
  case LOG_OBJECTS:
    f = &objects_log;
    logpath << OBJECTS_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open objects log.\n");
      exit(1);
    }
    break;
  case LOG_QUEST:
    f = &quest_log;
    logpath << QUEST_LOG;
    if (!(*f = dc_fopen(logpath.str().c_str(), "a")))
    {
      fprintf(stderr, "Unable to open quest log.\n");
      exit(1);
    }
    break;
  }

  time_t t = time(0);
  const tm *lt = localtime(&t);
  char *tmstr = asctime(lt);
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (!stream)
  {
    if (cf.stderr_timestamp == true) {

      fprintf(stderr, "%s :: %s\n", tmstr, str);
    } else {
      fprintf(stderr, "%s\n", str);
    }
  } else
  {
    fprintf(*f, "%s :: %s\n", tmstr, str);
    dc_fclose(*f);
  }

  if (god_level >= IMMORTAL)
    send_to_gods(str, god_level, type);
}

// function for new SETBIT et al. commands
void sprintbit( uint value[], const char *names[], char *result )
{
   int i;
   *result = '\0';
   
   for (i = 0; *names[i] != '\n';i++)
   {
     int a = i/ASIZE;
     if (IS_SET(value[a], 1 << (i - a*32)))
     {
      if (!strcmp(names[i], "UNUSED")) continue;
      strcat(result, names[i]);
     strcat(result, " ");
     }
   }

   if ( *result == '\0' )
      strcat( result, "NoBits " );
}

void sprintbit( unsigned long vektor, const char *names[], char *result )
{
    long nr;

    *result = '\0';

    if(vektor < 0)
    {
      logf(IMMORTAL, LOG_WORLD, "Negative value sent to sprintbit");
      return;
    }

    for ( nr=0; vektor; vektor>>=1 )
    {
	if ( IS_SET(1, vektor) )
	{
	    if (!strcmp(names[nr], "unused")) continue;
	    if ( *names[nr] != '\n')
		strcat( result, names[nr] );
	    else
		strcat( result, "Undefined" );
	    strcat( result, " " );
	}

	if ( *names[nr] != '\n' )
	  nr++;
    }

    if ( *result == '\0' )
	strcat( result, "NoBits " );
}


void sprinttype( int type, const char *names[], char *result )
{
    int nr;

    for ( nr = 0; *names[nr] != '\n'; nr++ )
	;
    if ( type > -1 && type < nr )
	strcpy( result, names[type] );
    else
	strcpy( result, "Undefined" );
}

int consttype( char * search_str, const char *names[] )
{
    int nr;

    for ( nr = 0; *names[nr] != '\n'; nr++ )
        if ( is_abbrev(search_str, names[nr]) )
	   return nr;

    return -1;
}

const char * constindex( int index, const char *names[] )
{
    int nr;

    for ( nr = 0; *names[nr] != '\n'; nr++ )
        if (nr == index)
           return names[nr];

    return (char *)0;
}


// Calculate the MUD time passed over the last t2-t1 centuries (secs) 
struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
    long secs;
    struct time_info_data now;

    secs = (long) (t2 - t1);

    now.hours = (secs/SECS_PER_MUD_HOUR) % 24;  /* 0..23 hours */
    secs -= SECS_PER_MUD_HOUR*now.hours;

    now.day = (secs/SECS_PER_MUD_DAY) % 35;     /* 0..34 days  */
    secs -= SECS_PER_MUD_DAY*now.day;

    now.month = (secs/SECS_PER_MUD_MONTH) % 17; /* 0..16 months */
    secs -= SECS_PER_MUD_MONTH*now.month;

    now.year = (secs/SECS_PER_MUD_YEAR);        /* 0..XX? years */

    return now;
}

struct time_info_data age(CHAR_DATA *ch)
{
    struct time_info_data player_age;

    // TODO - make this return some sensible value for mobs
    if(IS_MOB(ch)) {
      player_age.year = 5;
      return player_age;
    }

    time_t birth = ch->pcdata->time.birth;
    player_age = mud_time_passed(time(0), birth);

    player_age.year += 17;   /* All players start at 17 */
    player_age.year += GET_AGE_METAS(ch);

    return player_age;
}

bool file_exists(const char *filename)
{
  FILE *fp;
  bool r;

  fp = dc_fopen(filename, "r");
  r = (fp ? 1 : 0);
  dc_fclose(fp);
  return(r);
}

void util_archive(const char *char_name, CHAR_DATA *caller)
{
  char buf[256];
  char buf2[256];
  int i;

  // Ok, ok, we'll do some sanity checking on the
  // string to make sure that it has no meta chars in
  // it.  Grumble. -Morc 
  for(i = 0; (unsigned) i < strlen(char_name); i++)  {
    if(!isalpha(char_name[i])) {
       if(caller) {
        sprintf(buf, "Illegal archive attempt: %s by %s.",
	  char_name, GET_NAME(caller));
        log(buf, OVERSEER, LOG_GOD);
	return;
      }
      else {
	sprintf(buf, "Someone got a weird char name in there: %s.", char_name);
	log(buf, OVERSEER, LOG_GOD);
	return;
      }
    }
  }
	
  sprintf(buf, "%s/%c/%s", SAVE_DIR, UPPER(char_name[0]), char_name);
  sprintf(buf2, "%s/%s", ARCHIVE_DIR, char_name);
  if(!file_exists(buf) || file_exists(buf2))
  {
    if(caller) send_to_char("That character does not exist.\n\r", caller);
    else log("Attempt to archive a non-existent char.", IMMORTAL, LOG_BUG);
    return;
  }
  sprintf(buf, "%s -9 %s/%c/%s", GZIP, SAVE_DIR, UPPER(char_name[0]), char_name);
  if(system(buf))
  {
    sprintf(buf, "Unsuccessful archive: %s", char_name);
    if(caller) csendf(caller, "%s\n\r", buf);
    else log(buf, IMMORTAL, LOG_GOD);
    return;
  }
  sprintf(buf, "%s/%c/%s.gz", SAVE_DIR, UPPER(char_name[0]), char_name);
  sprintf(buf2, "%s/%s.gz", ARCHIVE_DIR, char_name);
  rename(buf, buf2);
  sprintf(buf, "Character archived: %s", char_name);
  if(caller) csendf(caller, "%s\n\r", buf);
  log(buf, IMMORTAL, LOG_GOD);
}

void util_unarchive(char *char_name, CHAR_DATA *caller)
{
  char buf[256];
  char buf2[256];
  int i;
  
  for(i = 0; (unsigned) i < strlen(char_name); i++) {
    if(!isalpha(char_name[i]))  {
        if(caller) {
	  sprintf(buf, "Illegal unarchive attempt: %s by %s.", char_name,
	          GET_NAME(caller));
          log(buf, OVERSEER, LOG_GOD);
	  return;
        }
        else {
	  sprintf(buf, "Someone got a weird char name in there: %s.",
	          char_name);
          log(buf, OVERSEER, LOG_GOD);
	  return;
        }
     }
  }  
  
  sprintf(buf, "%s/%s.gz", ARCHIVE_DIR, char_name);

  if(!file_exists(buf))
  {
    if(caller)
      send_to_char("Character not archived or already deleted!\n\r", caller);
    return;
  }
  sprintf(buf, "%s -d %s/%s.gz", GZIP, ARCHIVE_DIR, char_name);
  if(system(buf))
  {
    sprintf(buf, "Unsuccessful unarchive: %s", char_name);
    if(caller) csendf(caller, "%s\n\r", buf);
    else log(buf, IMMORTAL, LOG_GOD);
    return;
  }
  sprintf(buf, "%s/%s", ARCHIVE_DIR, char_name);
  sprintf(buf2, "%s/%c/%s", SAVE_DIR, UPPER(char_name[0]), char_name);
  rename(buf, buf2);
  sprintf(buf, "Character unarchived: %s", char_name);
  if(caller) csendf(caller, "%s\n\r", buf);
  log(buf, IMMORTAL, LOG_GOD);
}

bool ARE_CLANNED( struct char_data *sub, struct char_data *obj)
{
    if (IS_PC(sub) &&
	IS_MOB(obj) &&
	obj->master &&
	ARE_CLANNED(sub, obj->master) &&
	(IS_AFFECTED(obj, AFF_CHARM) || IS_AFFECTED(obj, AFF_FAMILIAR)))
	return TRUE;

   // make sure we're clanned, and the person we're looking at is in same clan
   // (have to check if we're clanned, cause otherwise two non-clanned people
   // would count as being in "same clan")
   if (!sub->clan || sub->clan != obj->clan)
      return FALSE;
   
   return TRUE;
}

int DARK_AMOUNT(int room)
{
   int glow = world[room].light;

   // indoors and cities are always lit
   if( world[room].sector_type == SECT_INSIDE ||
       world[room].sector_type == SECT_CITY)
     glow += 3;

   if(IS_SET(world[room].room_flags, DARK))
     glow -= 2;

   if(IS_SET(world[room].room_flags, LIGHT_ROOM))
     glow += 2;

   if(weather_info.sunlight == SUN_DARK)
     glow -= 1;

   return glow;
}

// Room light. 
// 0 to + is light
// -1 to - is dark
// SUN_DARK = -1
bool IS_DARK( int room )
{
   int glow = DARK_AMOUNT(room);

   if(glow < 0)
     return TRUE;

   return FALSE;
}

bool ARE_GROUPED( struct char_data *sub, struct char_data *obj)
{
   struct follow_type *f;
   struct char_data *k;

   if (obj == sub)
      return TRUE;

   if (obj == NULL || sub == NULL)
     return FALSE;

   if (IS_PC(sub) &&
       IS_NPC(obj) &&
       obj->master &&
       ARE_GROUPED(sub, obj->master) &&
       (IS_AFFECTED(obj, AFF_CHARM) || IS_AFFECTED(obj, AFF_FAMILIAR)))
     return TRUE;

   if (!(k=sub->master))
      k = sub;

   if( (IS_AFFECTED(k, AFF_GROUP)) && (IS_AFFECTED(sub, AFF_GROUP)) ) {
      if ((k == obj) && (IS_AFFECTED(obj, AFF_GROUP)) )
         return TRUE;

      for (f = k->followers; f; f = f->next) 
      {
         if ((f->follower == obj) && (IS_AFFECTED(obj, AFF_GROUP)) )
            return TRUE;
      }
   }
   return FALSE;
}

int SWAP_CH_VICT(int value)
{
  int newretval = 0;

  if(IS_SET(value, eCH_DIED))
    SET_BIT(newretval, eVICT_DIED);
  else REMOVE_BIT(newretval, eVICT_DIED);

  if(IS_SET(value, eVICT_DIED)) 
    SET_BIT(newretval, eCH_DIED);
  else REMOVE_BIT(newretval, eCH_DIED);

  return newretval;
}

bool SOMEONE_DIED(int value)
{
   if(IS_SET(value, eCH_DIED) || IS_SET(value, eVICT_DIED))
     return TRUE;
   return FALSE;
}

bool CAN_SEE( struct char_data *sub, struct char_data *obj, bool noprog )
{
   if (obj == sub)
      return TRUE;
    
   if (!sub || !obj) {
      log("Invalid pointer passed to CAN_SEE!", ANGEL, LOG_BUG);
      return FALSE;
      }	

   if (!IS_MOB(obj)) 
   {
      if(!obj->pcdata) // noncreated char
         return TRUE;

      if(GET_LEVEL(sub) < obj->pcdata->wizinvis) {
         if (obj->pcdata->incognito == TRUE) {
            if (sub->in_room != obj->in_room)
               return FALSE;
            return TRUE;
         }
         else
            return FALSE;
      }
   }


   if ( !IS_MOB(sub) && sub->pcdata->holyLite )
      return TRUE;

   if (!noprog && IS_NPC(obj))
   {
      int prog = mprog_can_see_trigger(sub,obj);
      if (IS_SET(prog, eEXTRA_VALUE)) return TRUE;
      else if (IS_SET(prog, eEXTRA_VAL2)) return FALSE;
   }
   if ( IS_AFFECTED(obj, AFF_GLITTER_DUST) && GET_LEVEL(obj) < IMMORTAL )
      return TRUE;

   if (obj->in_room == NOWHERE) {
	 return FALSE;
   }
   
   if (world[obj->in_room].sector_type == SECT_FOREST &&
       IS_AFFECTED(obj, AFF_FOREST_MELD) &&
       IS_AFFECTED(obj, AFF_HIDE))
      return FALSE;

   if ( IS_AFFECTED(sub, AFF_BLIND) )
      return FALSE;

   if ( !IS_LIGHT(sub->in_room) && !IS_AFFECTED(sub, AFF_INFRARED) )
      return FALSE;

   if (IS_AFFECTED(obj, AFF_HIDE))
   {
	   if (IS_AFFECTED(sub, AFF_TRUE_SIGHT))
      		return TRUE;

           if (ARE_GROUPED(sub, obj))
           {
             if ( !IS_AFFECTED( obj, AFF_INVISIBLE ) )
               return TRUE; //if they're not invis.. they can always see

             if ( IS_AFFECTED( sub, AFF_DETECT_INVISIBLE ) )
               return TRUE; //if they have det invis.. they can see

              return FALSE; //else they can't
           }

      if (is_hiding(obj, sub)) return FALSE;
   }

   if ( !IS_AFFECTED( obj, AFF_INVISIBLE ) )
      return TRUE;

   if ( IS_AFFECTED( sub, AFF_DETECT_INVISIBLE ) )
      return TRUE;

   return FALSE;
}

bool CAN_SEE_OBJ( struct char_data *sub, struct obj_data *obj, bool blindfighting )
{
   int skill = 0;
   struct affected_type * cur_af;

   if ( !IS_MOB(sub) && sub->pcdata->holyLite )
	return TRUE;

   int prog = oprog_can_see_trigger(sub,obj);
   if (IS_SET(prog, eEXTRA_VALUE)) return TRUE;
    else if (IS_SET(prog, eEXTRA_VAL2)) return FALSE;
   
   skill = 0;
   if ((cur_af = affected_by_spell(sub, SPELL_DETECT_GOOD)))
     skill = (int)cur_af->modifier;
   if ((skill >= 80 || GET_LEVEL(sub) >= IMMORTAL) && isname("consecrateitem", GET_OBJ_NAME(obj)) && obj->obj_flags.value[0] == SPELL_CONSECRATE)
     return TRUE;

   skill = 0;
   if ((cur_af = affected_by_spell(sub, SPELL_DETECT_EVIL)))
     skill = (int)cur_af->modifier;
   if ((skill >= 80 || GET_LEVEL(sub) >= IMMORTAL) && isname("consecrateitem", GET_OBJ_NAME(obj)) && obj->obj_flags.value[0] == SPELL_DESECRATE)
     return TRUE;

   if (IS_OBJ_STAT(obj, ITEM_NOSEE))
      return FALSE;

   if ( IS_AFFECTED( sub, AFF_BLIND ) ) {
     if(blindfighting && skill_success(sub, NULL, SKILL_BLINDFIGHTING))
       return TRUE;
     else
       return FALSE;
   }

   // only see beacons if you have detect magic up
   if ((cur_af = affected_by_spell(sub, SPELL_DETECT_MAGIC)))
     skill = (int)cur_af->modifier;

   if (GET_ITEM_TYPE(obj) == ITEM_BEACON && IS_SET(obj->obj_flags.extra_flags, ITEM_INVISIBLE)) {
     if (!IS_AFFECTED(sub, AFF_DETECT_MAGIC)) {
       return FALSE;
     } else {
       if (skill < 50)
        return FALSE;
     }
   }

//   if (IS_AFFECTED(sub, AFF_TRUE_SIGHT) )
  //      return TRUE;

   if(IS_OBJ_STAT(obj, ITEM_INVISIBLE) && !IS_AFFECTED(sub, AFF_DETECT_INVISIBLE))
      return FALSE;

   if( IS_DARK(sub->in_room) && !IS_AFFECTED(sub, AFF_INFRARED) && !IS_OBJ_STAT(obj, ITEM_GLOW) )
      return FALSE;

    return TRUE;
}

bool check_blind( struct char_data *ch )
{

//   if (IS_AFFECTED(ch, AFF_TRUE_SIGHT))
  //    return FALSE;

    if ( !IS_NPC(ch) && ch->pcdata->holyLite )
       return FALSE;

    if ( IS_AFFECTED(ch, AFF_BLIND) && number(0,4)) // 20% chance of seeing
    {
	send_to_char( "You can't see a damn thing!\n\r", ch );
	return TRUE;
    }

    return FALSE;
}


int do_order(struct char_data *ch, char *argument, int cmd)
{
    char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
    char buf[256];
    bool found = FALSE;
    int org_room;
    int retval;
    struct char_data *victim;
    struct follow_type *k;
  
    half_chop(argument, name, message);


    if (IS_SET(world[ch->in_room].room_flags, QUIET))
    {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
    }

    if (!*name || !*message)
	send_to_char("Order who to do what?\n\r", ch);
    else if (!(victim = get_char_room_vis(ch, name)) &&
	     str_cmp("follower", name) && str_cmp("followers", name))
	    send_to_char("That person isn't here.\n\r", ch);
    else if (ch == victim)
	send_to_char("You obviously suffer from schitzophrenia.\n\r", ch);
    else {
       if (IS_AFFECTED(ch, AFF_CHARM)) {
          send_to_char("Your superior would not aprove of you giving orders.\n\r",ch);
          return eFAILURE;
       }

       if (victim) {
          sprintf(buf, "$N orders you to '%s'", message);
          act(buf,  victim, 0, ch, TO_CHAR, 0);
          act("$n gives $N an order.", ch, 0, victim, TO_ROOM, NOTVICT);
          if ( (victim->master!=ch) || 
               !(IS_AFFECTED(victim, AFF_CHARM) ||
               IS_AFFECTED(victim, AFF_FAMILIAR)))
             act("$n has an indifferent look.", victim, 0, 0, TO_ROOM, 0);
          else {
             send_to_char("Ok.\n\r", ch);
             command_interpreter(victim, message);
          }
       } else {  /* This is order "followers" */
          sprintf(buf, "$n issues the order '%s'.", message);
          act(buf,  ch, 0, victim, TO_ROOM, 0);

          org_room = ch->in_room;

			if (ch->followers)
				for (k = ch->followers; k && k != (follow_type *) 0x95959595;
						k = k->next) {
					if (org_room == k->follower->in_room)
						if (IS_AFFECTED(k->follower, AFF_CHARM)) {
							found = TRUE;
							retval = command_interpreter(k->follower, message);
							if (IS_SET(retval, eCH_DIED))
								break; // k is no longer valid if it was a mob(always), get out now
						}
				}

			if (found)
				send_to_char("Ok.\n\r", ch);
			else
				send_to_char("Nobody here are loyal subjects of yours!\n\r",
						ch);
		}
    }
    return eSUCCESS;
}

int do_idea(struct char_data *ch, char *argument, int cmd)
{
    FILE *fl;
    char str[MAX_STRING_LENGTH];

    if (IS_NPC(ch)) {
	send_to_char("Monsters can't have ideas - Go away.\n\r", ch);
	return eFAILURE;
    }

    /* skip whites */
    for (; isspace(*argument); argument++);

    if (!*argument) {
	send_to_char("That doesn't sound like a good idea to me.  Sorry.\n\r", ch);
	return eFAILURE;
    }

    if (!(fl = dc_fopen(IDEA_LOG, "a"))) {
	perror ("do_idea");
	send_to_char("Could not open the idea log.\n\r", ch);
	return eFAILURE;
    }

    sprintf(str, "**%s[%d]: %s\n", GET_NAME(ch), world[ch->in_room].number, argument);
    fputs(str, fl);
    dc_fclose(fl);
    send_to_char("Ok.  Thanks.\n\r", ch);
    return eSUCCESS;
}

int do_typo(struct char_data *ch, char *argument, int cmd)
{
    FILE *fl;
    char str[MAX_STRING_LENGTH];

    if (IS_NPC(ch)) {
	send_to_char("Monsters can't spell - leave me alone.\n\r", ch);
	return eFAILURE;
    }

    /* skip whites */
    for (; isspace(*argument); argument++);

    if (!*argument) {
	send_to_char("I beg your pardon?\n\r", ch);
	return eFAILURE;
    }

    if (!(fl = dc_fopen(TYPO_LOG, "a")))
    {
	perror ("do_typo");
	send_to_char("Could not open the typo log.\n\r", ch);
	return eFAILURE;
    }

    sprintf(str, "**%s[%d]: %s\n",
	GET_NAME(ch), world[ch->in_room].number, argument);
    fputs(str, fl);
    dc_fclose(fl);
    send_to_char("Ok.  Thanks.\n\r", ch);
    return eSUCCESS;
}

int do_bug(struct char_data *ch, char *argument, int cmd)
{
    FILE *fl;
    char str[MAX_STRING_LENGTH];

    if (IS_NPC(ch)) {
	send_to_char("You are a monster! Bug off!\n\r", ch);
	return eFAILURE;
    }

    /* skip whites */
    for (; isspace(*argument); argument++);

    if (!*argument) {
	send_to_char("Pardon?\n\r", ch);
	return eFAILURE;
    }

    if (!(fl = dc_fopen(BUG_LOG, "a"))) {
	perror ("do_bug");
	send_to_char("Could not open the bug log.\n\r", ch);
	return eFAILURE;
    }

    sprintf(str, "**%s[%d]: %s\n", GET_NAME(ch), world[ch->in_room].number, argument);
    fputs(str, fl);
    dc_fclose(fl);
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
}

int do_recall( CHAR_DATA *ch, char *argument, int cmd )
{
  int location, percent, level, cost = 0, x;
  CHAR_DATA *victim;
  CHAR_DATA *loop_ch;
  float cf;
  char name[256] = "";
  clan_data * clan;
  struct clan_room_data * room;
  int found = 0;
  int retval;
  int is_mob;

  act( "$n prays to $s God for transportation!", ch, 0, 0, TO_ROOM , INVIS_NULL);

  if(IS_AFFECTED(ch, AFF_CHARM))
    return eFAILURE;

  if(IS_SET(world[ch->in_room].room_flags, ARENA)) {
    send_to_char("To the DEATH, you wimp.\n\r", ch);
    return eFAILURE;
  }

  if(IS_SET(world[ch->in_room].room_flags, NO_MAGIC)) {
    send_to_char("Your magic fizzles and dies.\n\r", ch);
    return eFAILURE;
  }

  if(IS_SET(ch->combat, COMBAT_BASH1) ||
     IS_SET(ch->combat, COMBAT_BASH2)) {
     send_to_char("You can't, you're bashed!\r\n", ch);
     return eFAILURE;
  }

  one_argument(argument, name);

  if (!(*name))
     victim = ch;
  else if (!(victim = get_char_room_vis(ch, name)) ||
           (!ARE_GROUPED(ch, victim) && !ARE_CLANNED(ch, victim))) {
     send_to_char( "Whom do you want to recall?\n\r", ch );
     return eFAILURE;
  }

  if(!IS_NPC(ch))
  {
    x = GET_WIS(ch);
    percent = number(1, 100);
    if (percent > x) {
      percent -= x;
    }

    // Additional 5% chance of failure when recalling across continents
    location = GET_HOME(victim);
    if (location > 0 && zone_table[world[victim->in_room].zone].continent != zone_table[world[location].zone].continent) {
      percent += 5;
    }

    if(percent > 50) {
      send_to_char( "You failed in your recall!\n\r", ch );
      return eFAILURE;
    }
  }


  if(victim->fighting && !IS_NPC(victim->fighting)) //PvP fight?
  {
     send_to_char("The gods refuse to answer your prayers while you're fighting!\r\n",victim);
     return eFAILURE;
  }


  if(affected_by_spell(victim, FUCK_PTHIEF) || affected_by_spell(victim, FUCK_GTHIEF)) {
     send_to_char("The gods frown upon your thieving ways and refuse to aid your escape.\n\r", victim);
     return eFAILURE;
  }

  if (IS_NPC(ch))
    location = real_room(GET_HOME(ch));
  else
  {
    if( GET_HOME(victim) == 0 || GET_LEVEL(victim) < 11 ||
        IS_AFFECTED(victim, AFF_CANTQUIT) )
      location = real_room(START_ROOM);
    else
      location = real_room(GET_HOME(victim));
    if (location < 0)
    {
	send_to_char("Failed.\r\n",ch);
	return eFAILURE;
    }


      if (IS_SET(world[location].room_flags, NOHOME))
      {
	send_to_char("The gods reset your home.\r\n",victim);
	location = real_room(START_ROOM);
	GET_HOME(victim) = START_ROOM;
	}

    // make sure they arne't recalling into someone's chall

    if(IS_SET(world[location].room_flags, CLAN_ROOM)) {
       if(!victim->clan || !(clan = get_clan(victim))) {
         send_to_char("The gods frown on you, and reset your home.\r\n", ch);
         location = real_room(START_ROOM);
         GET_HOME(victim) = START_ROOM;
       }
       else {
          for(room = clan->rooms; room; room = room->next)
            if(room->room_number == GET_HOME(victim))
               found = 1;

          if(!found) {
             send_to_char("The gods frown on you, and reset your home.\r\n", ch);
             location = real_room(START_ROOM);
             GET_HOME(victim) = START_ROOM;
          }
       }
    }
  }

  if(location == -1) {
    send_to_char("You are completely lost.\n\r", victim);
    return eFAILURE|eINTERNAL_ERROR;
  }

  if((IS_SET(world[location].room_flags, CLAN_ROOM) || location == real_room(2354) || location == real_room(2355)) && IS_AFFECTED(victim, AFF_CHAMPION)) {
     send_to_char("No recalling into a clan hall whilst Champion, go to the Tavern!.\n\r", victim);
     location = real_room(START_ROOM);
  }
  if(location >= 1900 && location <= 1999 && IS_AFFECTED(victim, AFF_CHAMPION)) {
     send_to_char("No recalling into a guild hall whilst Champion, go to the Tavern!.\n\r", victim);
     location = real_room(START_ROOM);
  }


  // calculate the gold needed
  level = GET_LEVEL(victim);
  if ((level > 10) && (level <= MAX_MORTAL))
  {
    cf   = 1 + ((level - 11) * .347f);
    cost = (int)(3440 * cf);

    if (zone_table[world[victim->in_room].zone].continent != zone_table[world[location].zone].continent) {
      // Cross-continent recalling costs twice as much
      cost *= 2;
    }

    if (GET_GOLD(ch) < (uint32)cost)
    {
         csendf(ch, "You don't have %d gold!\n\r", cost);
         return eFAILURE;
    }

    GET_GOLD(ch) -= cost;
  }

   if (IS_AFFECTED(victim, AFF_CURSE) || IS_AFFECTED(victim, AFF_SOLIDITY))
   {
	send_to_char("Something blocks it.\r\n",ch);
	return eFAILURE;
   }

    for(loop_ch = world[victim->in_room].people; loop_ch; loop_ch = loop_ch->next_in_room)
       if(loop_ch == victim || loop_ch->fighting == victim)
          stop_fighting(loop_ch);

    act( "$n disappears.", victim, 0, 0, TO_ROOM , INVIS_NULL);
    is_mob = IS_MOB(victim);    
    retval = move_char(victim, location);

    if(!is_mob && !IS_SET(retval, eCH_DIED)) { // if it was a mob, we might have died moving 
       act( "$n appears out of nowhere.", victim, 0, 0, TO_ROOM , INVIS_NULL);
       do_look( victim, "", 0 );
    }
    return retval;
}

int do_qui(struct char_data *ch, char *argument, int cmd)
{
    send_to_char("You have to write quit - no less, to quit!\n\r",ch);
    return eSUCCESS;
}

int do_quit(struct char_data *ch, char *argument, int cmd)
{
  int iWear;
  struct follow_type *k;
   clan_data * clan;
   struct clan_room_data * room;
   int found = 0;
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *obj, *tmp_obj;

  void find_and_remove_player_portal(char_data * ch);

  /*
  | Code inserted by Morc 9 Apr 1997 to fix crasher
  */
  if(ch == 0)
  {
    log("do_quit received null char - problem!", LOG_BUG, OVERSEER);
    return eFAILURE|eINTERNAL_ERROR;
  }

  if(IS_NPC(ch))
    return eFAILURE;
   
  if (!IS_SET(world[ch->in_room].room_flags, SAFE) && cmd != 666
      && GET_LEVEL(ch) < IMMORTAL) 
  {
     send_to_char("This room doesn't feel...SAFE enough to do that.\n\r", ch);
     return eFAILURE;
  } 
 
  // If ch has follower, cant quit
  // NOTE: If we sent a 666 to do_quit, then it came from a zap or a boot
  // at this point, we've set the char back to level 1 (if from a zap), so
  // we'll end up with a fully equipped char with huge stats reset to level
  // 1

  if(cmd != 666) {
    for(k = ch->followers; k; k = k->next) {   
          if(IS_AFFECTED(k->follower, AFF_CHARM)) {
          send_to_char ("But you wouldn't want to just abandon your followers!\r\n", ch);
          return eFAILURE;
       }
    }

    if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
    }

    if(GET_POS(ch) == POSITION_FIGHTING&&cmd!=666) {
      send_to_char( "No way! You are fighting.\n\r", ch );
      return eFAILURE;
    }

    if(GET_POS(ch) < POSITION_STUNNED) {
      send_to_char( "You're not DEAD yet.\n\r", ch );
      return eFAILURE;
    }

    if( (IS_AFFECTED(ch, AFF_CANTQUIT) && cmd!=666 ) ||
	affected_by_spell(ch, FUCK_PTHIEF) ||
	affected_by_spell(ch, FUCK_GTHIEF) ) {
      send_to_char("You can't quit, because you are still wanted!\n\r", ch);
      return eFAILURE;
    }

    if (IS_SET(world[ch->in_room].room_flags, NO_QUIT) && cmd!=666)
    {
      send_to_char("Something about this room makes it seem like a bad place to quit.\r\n", ch);
      return eFAILURE;
    }

    if (IS_SET(world[ch->in_room].room_flags, ARENA))
    {
      send_to_char("Don't make me zap you.....\r\n", ch);
      return eFAILURE;
    }

    if(IS_SET(world[ch->in_room].room_flags, CLAN_ROOM) && cmd!=666) 
    {
      if(!ch->clan || !(clan = get_clan(ch))) {
         send_to_char("This is a clan room dork.  Try joining one first.\r\n", ch);
         return eFAILURE;
      }
      
      for(room = clan->rooms; room; room = room->next)
         if(ch->in_room == real_room(room->room_number))
            found = 1;

      if(!found) {
         send_to_char("Chode! You can't quit in another clan's hall!\r\n", ch);
         return eFAILURE;
      }
    }
    act( "$n has left the game.", ch, 0, 0, TO_ROOM , INVIS_NULL);
    csendf(ch, "Deleting %s.\n\r", GET_NAME(ch));
  }

  // Finish off any performances
  if(IS_SINGING(ch))
    do_sing(ch, "stop", 9);

  extractFamiliar(ch);
  struct follow_type *fol, *fol_next;

  for (fol = ch->followers; fol; fol = fol_next)
  {
    fol_next = fol->next; 
   if (IS_NPC(fol->follower) &&
mob_index[fol->follower->mobdata->nr].virt == 8)
    {
      release_message(fol->follower);
      extract_char(fol->follower, FALSE);
    }
  }
  affect_from_char(ch, SPELL_IRON_ROOTS);
  affect_from_char(ch, SPELL_DIVINE_INTER);  //sloppy sloppy
  affect_from_char(ch, SPELL_NO_CAST_TIMER); // *sigh*
  affect_from_char(ch, SKILL_CM_TIMER);
  affect_from_char(ch, SPELL_IMMUNITY);
  affect_from_char(ch, SKILL_BATTLESENSE);
  affect_from_char(ch, SKILL_SMITE);

  if(ch->beacon)
    extract_obj(ch->beacon);

  if(ch->cRooms) {
   for(obj = object_list; obj; obj = tmp_obj) {
    tmp_obj = obj->next;
    if(obj_index[obj->item_number].virt == CONSECRATE_OBJ_NUMBER)
      if(ch == (CHAR_DATA *)(obj->obj_flags.value[3]) )
       extract_obj(obj);
   }
  }

  if(IS_AFFECTED(ch, AFF_CHAMPION)) {
     REMBIT(ch->affected_by, AFF_CHAMPION);
     struct affected_type af;
     af.type      = OBJ_CHAMPFLAG_TIMER;
     af.duration  = 5;
     af.modifier  = 0;
     af.location  = APPLY_NONE;
     af.bitvector = -1;
     affect_to_char(ch, &af);
     sprintf(buf, "\n\r##%s has just logged out, watch for the Champion flag to reappear!\n\r", GET_NAME(ch));
     send_info(buf);
  }
  find_and_remove_player_portal(ch);
  stop_all_quests(ch);

  if(cmd != 666)
     clan_logout(ch);

  update_wizlist(ch);

  if (!IS_MOB(ch) && ch->desc && ch->desc->host) {
    if(ch->pcdata->last_site)
      dc_free(ch->pcdata->last_site);
#ifdef LEAK_CHECK
    ch->pcdata->last_site = (char *)calloc(strlen(ch->desc->host) + 1, sizeof(char));
#else
    ch->pcdata->last_site = (char *)dc_alloc(strlen(ch->desc->host) + 1, sizeof(char));
#endif
    strcpy (ch->pcdata->last_site, ch->desc->host);
    ch->pcdata->time.logon = time(0);
  }
 
  if(ch->desc) {
    save_char_obj(ch);
    if(!close_socket(ch->desc)) // if returns 0, then it already quit us out
      return eFAILURE|eCH_DIED;
  } else {
    save_char_obj(ch);
  } 

  SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

  for(iWear = 0; iWear < MAX_WEAR; iWear++) 
     if(ch->equipment[iWear])
       obj_to_char( unequip_char( ch, iWear,1 ), ch );

  while(ch->carrying)
    extract_obj(ch->carrying);

  extract_char( ch, TRUE );
  return eSUCCESS|eCH_DIED;
}

// TODO - make some sort of auto-save, or "save" flag, so player's
//        that save after every other kill don't actually do it, but it
//        pretends that it does.  That way we can start reducing the amount
//        of writing we're doing.
int do_save(struct char_data *ch, char *argument, int cmd)
{
  // With the cmd numbers
  // 666 = save quietly
  // 10 = save
  // 9 = save with a round of lag
  // -pir 3/15/1999

  if (IS_NPC(ch) || GET_LEVEL(ch) > IMP)
    return eFAILURE;

  if (cmd != 666)
  {
    csendf(ch, "Saving %s.\n\r", GET_NAME(ch));
  }

  if (IS_PC(ch))
  {
    save_char_obj(ch);
#ifdef USE_SQL
    save_char_obj_db(ch);
#endif

    if (ch->followers)
    {
      save_charmie_data(ch);
    }

    if (ch->pcdata->golem)
    {
      save_golem_data(ch); // Golem data, eh!
    }
  }

  return eSUCCESS;
}

int do_home(struct char_data *ch, char *argument, int cmd)
{
   clan_data * clan;
   struct clan_room_data * room;
   int found = 0;
   
  if (!IS_SET(world[ch->in_room].room_flags, SAFE) ||
      IS_SET(world[ch->in_room].room_flags, ARENA)) {
     send_to_char("This place doesn't sit right with you...not enough "
                  "security.\n\r", ch);
     return eFAILURE;
     }
     if (IS_SET(world[ch->in_room].room_flags, NOHOME))
	{
     send_to_char("Something prevents it.\n\r", ch);
     return eFAILURE;

	}

    if(GET_LEVEL(ch) < 11) {
      send_to_char("You must grow a bit before you can leave the nursery.\n\r", ch);
      GET_HOME(ch) = START_ROOM;
      return eFAILURE;
    }
  
   if(IS_SET(world[ch->in_room].room_flags, CLAN_ROOM)) {
      if(!ch->clan || !(clan = get_clan(ch))) {
         send_to_char("This is a clan room dork.  Try joining one first.\r\n", ch);
         return eFAILURE;
      }
      
      for(room = clan->rooms; room; room = room->next)
         if(ch->in_room == real_room(room->room_number))
            found = 1;

      if(!found) {
         send_to_char("Chode! You can't set home in another clan's hall!\r\n", ch);
         return eFAILURE;
      }
   }

   send_to_char("You now consider this place to be your home.\n\r", ch);
   GET_HOME(ch) = world[ch->in_room].number;
   return eSUCCESS;
}

int do_not_here(struct char_data *ch, char *argument, int cmd)
{
    send_to_char("Sorry, but you cannot do that here!\n\r",ch);
    return eSUCCESS;
}

// Used for debugging with dmalloc
int do_memoryleak(struct char_data *ch, char *argument, int cmd)
{
   if(GET_LEVEL(ch) < OVERSEER)
   {
      send_to_char("The 'leak' command is not available to you.\r\n", ch);
      return eFAILURE;
   }
   void *ptr = malloc(10);
   if (ptr == nullptr)
   {
     perror("malloc");
   }

   send_to_char("A memory leak was just caused.\r\n", ch);
   return eSUCCESS;
}

// Used for debugging with dmalloc
void cause_leak()
{
   void *ptr = malloc(10);
   if (ptr == nullptr)
   {
     perror("malloc");
   }
}

int do_beep(struct char_data *ch, char *argument, int cmd)
{
  send_to_char("Beep!\a\r\n", ch);
  return eSUCCESS;
}


// if a skill has a valid name, return it, else NULL
const char* get_skill_name(int skillnum) {

  if (skillnum >= SKILL_SONG_BASE && skillnum <= SKILL_SONG_MAX)
    return songs[skillnum - SKILL_SONG_BASE];
  else if (skillnum >= SKILL_BASE && skillnum <= SKILL_MAX)
    return skills[skillnum - SKILL_BASE];
  else if (skillnum >= KI_OFFSET && skillnum <= (KI_OFFSET + MAX_KI_LIST))
    return ki[skillnum - KI_OFFSET];
  else if (skillnum >= 0 && skillnum <= MAX_SPL_LIST)
    return spells[skillnum - 1];
  else if (skillnum >= SKILL_INNATE_BASE && skillnum <= SKILL_INNATE_MAX)
    return innate_skills[skillnum - SKILL_INNATE_BASE];
  else if (skillnum >= BASE_SETS && skillnum <= SET_MAX)
    return set_list[skillnum - BASE_SETS].SetName;
  else if (skillnum >= RESERVED_BASE && skillnum <= RESERVED_MAX)
    return reserved[skillnum - RESERVED_BASE];
  return NULL;
}

void double_dollars(char * destination, char * source)
{
  while(*source != '\0')
    if(*source == '$') {
      *destination++ = '$';
      *destination++ = '$';
      source++;
    }
    else *destination++ = *source++;

  *destination = '\0';
}

// convert char string to int
// return true if successful, false if error
// also check to make sure it's in the valid range
bool check_range_valid_and_convert(int & value, char * buf, int begin, int end)
{
   value = atoi(buf);
   if(value == 0 && strcmp(buf, "0"))
     return FALSE; 

   if(value < begin)
     return FALSE;

   if(value > end)
     return FALSE;

   return TRUE;
}

// convert char string to int
// return true if successful, false if error
bool check_valid_and_convert(int & value, char * buf)
{
   value = atoi(buf);
   if(value == 0 && strcmp(buf, "0"))
     return FALSE; 

   return TRUE;
}

// modified for new SETBIT et al. commands
void parse_bitstrings_into_int(const char * bits[], const char * strings, char_data *ch, uint value[])
{
  bool found = FALSE;
  string remainder_args(strings);

  if(!ch)
    return;

  for(;;) 
  {
    if(remainder_args.empty())
      break;

    string arg1;
    tie (arg1, remainder_args) = half_chop(remainder_args);
                       
    for(int x = 0 ;*bits[x] != '\n'; x++) 
    {
      if (!strcmp("unused",bits[x])) continue;
      if(is_abbrev(arg1, bits[x])) 
      {
        if(ISSET(value, x+1)) {
          REMBIT(value, x+1);
          csendf(ch, "%s flag REMOVED.\n\r", bits[x]);
        }
        else {
          SETBIT(value, x+1);
          csendf(ch, "%s flag ADDED.\n\r", bits[x]);
        }
        found = TRUE;
        break;
      }
    } 
  }
  if(!found)
    send_to_char("No matching bits found.\r\n", ch);
}

// calls below uint32 version
void parse_bitstrings_into_int(const char * bits[], const char * strings, char_data * ch, uint16 & value)
{
  int  found = FALSE;
  string remainder_args(strings);

  if(!ch)
    return;

  for(;;) 
  {
    if(remainder_args.empty())
      break;

    string arg1;
    tie (arg1, remainder_args) = half_chop(remainder_args);
                       
    for(int x = 0 ;*bits[x] != '\n'; x++) 
    {
      if (!strcmp("unused",bits[x])) continue;
      if(is_abbrev(arg1, bits[x])) 
      {
        if(IS_SET(value, (1<<x))) {
          REMOVE_BIT(value, (1<<x));
          csendf(ch, "%s flag REMOVED.\n\r", bits[x]);
        }
        else {
          SET_BIT(value, (1<<x));
          csendf(ch, "%s flag ADDED.\n\r", bits[x]);
        }
        found = TRUE;
        break;
      }
    } 
  }
  if(!found)
    send_to_char("No matching bits found.\r\n", ch);

}

// Assumes bits is array of strings, ending with a "\n" string
// Finds the bits[] strings listed in "strings" and toggles the bit in "value"
// Informs 'ch' of what has happened
//
void parse_bitstrings_into_int(const char * bits[], const char * strings, char_data * ch, uint32 & value)
{
  int  found = FALSE;
  string remainder_args(strings);

  if(!ch)
    return;

  for(;;) 
  {
    if(remainder_args.empty())
      break;

    string arg1;
    tie (arg1, remainder_args) = half_chop(remainder_args);

    for(int x = 0 ;*bits[x] != '\n'; x++) 
    {
      if (!strcmp("unused",bits[x])) continue;
      if(is_abbrev(arg1, bits[x])) 
      {
        if(IS_SET(value, (1<<x))) {
          REMOVE_BIT(value, (1<<x));
          csendf(ch, "%s flag REMOVED.\n\r", bits[x]);
        }
        else {
          SET_BIT(value, (1<<x));
          csendf(ch, "%s flag ADDED.\n\r", bits[x]);
        }
        found = TRUE;
        break;
      }
    } 
  }
  if(!found)
    send_to_char("No matching bits found.\r\n", ch);
}

// Display a \n terminated list to the character
//
void display_string_list(const char * list[], char_data *ch)
{
  char buf[MAX_STRING_LENGTH];
  *buf = '\0';

  for(int i = 1; *list[i-1] != '\n'; i++)        
  {
    sprintf(buf + strlen(buf), "%18s", list[i-1]);      
    if (!(i % 4))
    {
      strcat(buf, "\r\n");  
      send_to_char(buf, ch);
      *buf = '\0';
    }
  }
  if(*buf)
      send_to_char(buf, ch);
  send_to_char("\r\n", ch);
}

void check_timer()
{ // Called once/sec
  struct timer_data *curr,*nex,*las;
  las = NULL;
  for (curr = timer_list; curr; curr = nex)
  {
    nex = curr->next;
    curr->timeleft--;
    if (curr->timeleft <= 0)
    {
	(*(curr->function))(curr->arg1,curr->arg2,curr->arg3);	
        if (!nex && curr->next) nex = curr->next;
	if (las)
	  las->next = curr->next;
	else
	  timer_list = curr->next;
	dc_free(curr);
	continue;
    }
    las = curr;
  }
	DC::instance().removeDead();
}

int get_line(FILE * fl, char *buf)
{ 
  char temp[256];
  int lines = 0;

  do {
    lines++; 
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));
    
  if (feof(fl))
    return 0;
  else {
    strcpy(buf, temp);
    return lines;
  }
}





void init_random()
{ 
    int urandom_fd;
    unsigned int seed = time(0); //In case getting data from /dev/urandom fails
    
    if ((urandom_fd = open("/dev/urandom", O_RDONLY)) == -1) {
	logf(0, LOG_MISC, "Unable to open /dev/urandom: %s", strerror(errno));
    } else {
	if (read(urandom_fd, &seed, sizeof(seed)) == -1) {
	    logf(0, LOG_MISC, "Read error: %s", strerror(errno));
	}

	close(urandom_fd);
    }
    
    logf(0, LOG_MISC, "Seeding random numbers with %u", seed);
    char *state = (char *)malloc(256);
    initstate(seed, state, 256);
    return;
}


//
// return value can include the from or to variable
//
int number( int from, int to )
{
    if(from == to)
      return to;

    if(from > to)
    {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "BACKWARDS usage: numbers(%d, %d)!", from, to);
      log(buf, ANGEL, LOG_BUG);
      produce_coredump();
      return to;
    }
    int number = (to+1) - from;

    number = from + (int)((double)number * ((double)random() / ((double)RAND_MAX + 1.0)));
    return number;
}

int random_percent_change(int from, int to, int value) {
	return ((number(from, to) / 100.0) * value)+0.50+value;
}


bool is_in_game(char_data *ch)
{
  // Bug in code if this happens
  if (ch == 0) {
    log("NULL args sent to is_pc_playing in utility.c!", ANGEL, LOG_BUG);
    return false;
  }

  // ch is a mob
  if (IS_NPC(ch)) {
    return false;
  }

  // Linkdead
  if (ch->desc == 0) {
    return true;
  }

  switch(STATE(ch->desc)) {
  case CON_PLAYING:
  case CON_EDIT_MPROG:
  case CON_WRITE_BOARD:
  case CON_EDITING:
  case CON_SEND_MAIL:
    return true;
    break;
  }

  return false;
}

void produce_coredump(void *ptr)
{
  logf(IMMORTAL, LOG_BUG, "produce_coredump called with pointer %p", ptr);

  static int counter = 0;

  if (++counter > COREDUMP_MAX) {
    logf(IMMORTAL, LOG_BUG, "Error detected: Unable to produce coredump. Limit of %d reached.", COREDUMP_MAX);
    return;
  }

  pid_t pid = fork();
  if (pid == 0) {
    //Child process
    abort();
  } else if (pid > 0) {
    //Parent process
    logf(IMMORTAL, LOG_BUG, "Error detected: Producing coredump %d of %d.", counter, COREDUMP_MAX);
  } else {
    logf(IMMORTAL, LOG_BUG, "Error detected: Unable to fork process.");
  }
    
  return;
}

char *pluralize(int qty, char *ending)
{
  if (qty == 0 || qty > 1) {
    return ending;
  } else {
    return "";
  }
}

void remove_character(char *name, BACKUP_TYPE backup)
{
  char src_filename[256];
  char dst_dir[256] = {0};
  char syscmd[512];
  struct stat statbuf;

  if (name == NULL) {
    return;
  }

  name[0] = UPPER(name[0]);

  switch(backup) {
  case SELFDELETED:
    strncpy(dst_dir, "../archive/selfdeleted/", 256);
    break;
  case CONDEATH:
    strncpy(dst_dir, "../archive/condeath/", 256);
    break;
  case ZAPPED:
    strncpy(dst_dir, "../archive/zapped/", 256);
    break;
  case NONE:
    break;
  default:
    logf(108, LOG_GOD, "remove_character passed invalid BACKUP_TYPE %d for %s.", backup,
	 name);
    break;
  }

  if (DC::instance().cf.bport) {
    snprintf(src_filename, 256, "%s/%c/%s", BSAVE_DIR, name[0], name);
  } else {
    snprintf(src_filename, 256, "%s/%c/%s", SAVE_DIR, name[0], name);
  }
  
  if (0 == stat(src_filename, &statbuf)) { 
    if (dst_dir[0] != 0) {
      snprintf(syscmd, 512, "mv %s %s", src_filename, dst_dir);
      system(syscmd);
    } else {
      unlink(src_filename);
    }
  }

  if (DC::instance().cf.bport) {
    snprintf(src_filename, 256, "%s/%c/%s.backup", BSAVE_DIR, name[0], name);
  } else {
    snprintf(src_filename, 256, "%s/%c/%s.backup", SAVE_DIR, name[0], name);
  }
  
  if (0 == stat(src_filename, &statbuf)) { 
    if (dst_dir[0] != 0) {
      snprintf(syscmd, 512, "mv %s %s", src_filename, dst_dir);
      system(syscmd);
    } else {
      unlink(src_filename);
    }
  }

}

void remove_familiars(char *name, BACKUP_TYPE backup)
{
  char src_filename[256];
  char dst_dir[256] = {0};
  char syscmd[512];
  struct stat statbuf;

  if (name == NULL) {
    return;
  }

  name[0] = UPPER(name[0]);

  switch(backup) {
  case SELFDELETED:
    strncpy(dst_dir, "../archive/selfdeleted/", 256);
    break;
  case CONDEATH:
    strncpy(dst_dir, "../archive/condeath/", 256);
    break;
  case ZAPPED:
    strncpy(dst_dir, "../archive/zapped/", 256);
    break;
  case NONE:
    break;
  default:
    logf(108, LOG_GOD, "remove_familiars passed invalid BACKUP_TYPE %d for %s.",
	 backup, name);
    break;
  }
  
  for (int i=0; i < MAX_GOLEMS; i++) {
    snprintf(src_filename, 256, "%s/%c/%s.%d", FAMILIAR_DIR, name[0], name, i);
    
    if (0 == stat(src_filename, &statbuf)) { 
      if (dst_dir[0] != 0) {
	snprintf(syscmd, 512, "mv %s %s", src_filename, dst_dir);
	system(syscmd);
      } else {
	unlink(src_filename);
      }
    }
  }

}

bool check_make_camp(int room)
{
  CHAR_DATA *i, *next_i;
  bool campok = FALSE;

  for(i = world[room].people; i; i = next_i) {
    next_i = i->next_in_room;

    if(i->fighting)
    	return FALSE;
    if(IS_MOB(i) && !IS_AFFECTED(i, AFF_CHARM) && !IS_AFFECTED(i, AFF_FAMILIAR))
    	return FALSE;
    if(affected_by_spell(i, SKILL_MAKE_CAMP) && affected_by_spell(i, SKILL_MAKE_CAMP)->modifier == room)
      campok = TRUE;
  }

  return campok;
}

int get_leadership_bonus(CHAR_DATA *ch)
{
  CHAR_DATA *leader;
  struct follow_type *f, *next_f;
  int highlevel = 0, bonus = 0;

  if(ch->master) leader = ch->master;
  else leader = ch;

  if(IS_MOB(ch) || ch->in_room != leader->in_room) return 0;
  if(!affected_by_spell(leader, SKILL_LEADERSHIP)) return 0;

  if(affected_by_spell(ch, SKILL_LEADERSHIP) && ch->master)
    affect_from_char(ch, SKILL_LEADERSHIP);

  for(f = leader->followers; f; f = next_f) {
    next_f = f->next;

    if(highlevel < GET_LEVEL(f->follower))
      highlevel = GET_LEVEL(f->follower);
  }

  for(f = leader->followers; f; f = next_f) {
    next_f = f->next;

    if(IS_MOB(f->follower)) continue;
    if(leader->in_room != f->follower->in_room) continue;
    if(GET_LEVEL(f->follower) + 25 <= highlevel) continue;
    if(!IS_AFFECTED(f->follower, AFF_GROUP)) continue;

    bonus++;
  }

  return MIN(bonus, affected_by_spell(leader, SKILL_LEADERSHIP)->modifier);
}

void update_make_camp_and_leadership(void)
{
  struct affected_type af;
  int bonus = 0;
	auto &character_list = DC::instance().character_list;

  for_each(character_list.begin(), character_list.end(),
  				[&af, &bonus](char_data *i) {

    if(!i->fighting) {
      if(affected_by_spell(i, SKILL_SMITE))
        affect_from_char(i, SKILL_SMITE);

      if(affected_by_spell(i, SKILL_PERSEVERANCE)) {
        affect_from_char(i, SKILL_PERSEVERANCE);
        affect_from_char(i, SKILL_PERSEVERANCE_BONUS);
      }

      if(affected_by_spell(i, SKILL_BATTLESENSE))
        affect_from_char(i, SKILL_BATTLESENSE);
    }
    
    if (i->in_room != NOWHERE) {
      if(!check_make_camp(i->in_room)) {
        if(affected_by_spell(i, SKILL_MAKE_CAMP)) {
          affect_from_char(i, SKILL_MAKE_CAMP);
          send_to_room("The camp has been disturbed.\n\r", i->in_room);
        }
        if(affected_by_spell(i, SPELL_FARSIGHT) && affected_by_spell(i, SPELL_FARSIGHT)->modifier == 111)
          affect_from_char(i, SPELL_FARSIGHT);
      } else {
        if(!affected_by_spell(i, SPELL_FARSIGHT) && !IS_AFFECTED(i, AFF_FARSIGHT)) {
          af.type = SPELL_FARSIGHT;
          af.duration = -1;
          af.modifier = 111;
          af.location = 0;
          af.bitvector = AFF_FARSIGHT;

          affect_to_char(i, &af);
        }
      }
    }
    bonus = get_leadership_bonus(i);
    
    if(i->changeLeadBonus == TRUE) {
      i->changeLeadBonus = FALSE;

      if(i->curLeadBonus != bonus) {
        i->curLeadBonus = bonus;
        affect_from_char(i, SKILL_LEADERSHIP_BONUS);

        if(i->curLeadBonus) {
          af.type = SKILL_LEADERSHIP_BONUS;
          af.duration = -1;
          af.bitvector = -1;

          if(affected_by_spell(i, SKILL_LEADERSHIP)) {
            af.modifier = bonus * 2;
            af.location = APPLY_HIT_N_DAM;
            affect_to_char(i, &af);
          } else {
            af.modifier = bonus * -8;
            af.location = APPLY_AC;
            affect_to_char(i, &af);
          }
        }
      }
    }

    if(i->curLeadBonus != bonus)
      i->changeLeadBonus = TRUE;
  });
}

void unique_scan(struct char_data *victim)
{
  if(!victim) return;

  struct obj_data *i = NULL;
  struct obj_data *j = NULL;
  int k;
  map<int, int> virtnums;
  queue<OBJ_DATA *> found_items;

  for (k=0; k< MAX_WEAR; k++) 
  {
    if (victim->equipment[k])
    {
      if(IS_SET(victim->equipment[k]->obj_flags.more_flags, ITEM_UNIQUE))
      {
        if(virtnums.end() == virtnums.find(obj_index[victim->equipment[k]->item_number].virt))
          virtnums[obj_index[victim->equipment[k]->item_number].virt] = 1;
        else
          found_items.push(victim->equipment[k]);
      }
      if(GET_ITEM_TYPE(victim->equipment[k]) == ITEM_CONTAINER)
      { // search inside
        for(j = victim->equipment[k]->contains; j ; j = j->next_content) 
        {
          if(IS_SET(j->obj_flags.more_flags, ITEM_UNIQUE))
          {
            if(virtnums.end() == virtnums.find(obj_index[j->item_number].virt))
              virtnums[obj_index[j->item_number].virt] = 1;
            else
              found_items.push(j); 
          }
        }
      }
    }
  }

  for(i = victim->carrying; i ; i = i->next_content) 
  {
    if(IS_SET(i->obj_flags.more_flags, ITEM_UNIQUE))
    {
      if(virtnums.end() == virtnums.find(obj_index[i->item_number].virt))
        virtnums[obj_index[i->item_number].virt] = 1;
      else
        found_items.push(i);
    }

    // does not support containers inside containers
    if(GET_ITEM_TYPE(i) == ITEM_CONTAINER) 
    { // search inside
      for(j = i->contains; j ; j = j->next_content) 
      {
        if(IS_SET(j->obj_flags.more_flags, ITEM_UNIQUE))
        {
          if(virtnums.end() == virtnums.find(obj_index[j->item_number].virt))
            virtnums[obj_index[j->item_number].virt] = 1;
          else
            found_items.push(j);
        }
      }
    }
  }

  if(!found_items.empty())
  {
    logf(IMMORTAL, LOG_WARNINGS, "Player %s has duplicate unique items.", GET_NAME(victim));
    while(!found_items.empty())
    {
      logf(IMMORTAL, LOG_WARNINGS, "%s", found_items.front()->short_description);
      found_items.pop();
    }
  }

  return;
}

string replaceString(string message, string find, string replace) {
	size_t j;

	if (find.empty())
		return message;
	if (replace.empty())
		return message;
	if (find == replace)
		return message;

	size_t find_length = find.length();
	for (; (j = message.find(find)) != string::npos;) {
		message.replace(j, find_length, replace);
	}
	return message;
}

char * numToStringTH(int number)
{
 switch(number) {
  case 1: return "first";
  case 2: return "second";
  case 3: return "third";
  case 4: return "fourth";
  case 5: return "fifth";
  case 6: return "sixth";
  case 7: return "seventh";
  case 8: return "eighth";
  case 9: return "ninth";
  case 10: return "tenth";
  case 11: return "eleventh";
  default: return "";
 }
}

bool champion_can_go(int room)
{
  try {
    // Champions can't enter class restricted rooms
    for (int c_class=1; c_class < CLASS_MAX; c_class++) {
      if (world[room].allow_class[c_class] == TRUE) {
	return false;
      }
    }

    // Champions can't enter clan rooms
    if (IS_SET(world[room].room_flags, CLAN_ROOM)) {
      return false;
    }
  } catch(...) {
    return false;
  }

  return true;
}

/*
splitstring
This function does NOT ignore empty strings unless you tell it to.
example:
splitstring("string  with 2 spaces", " ")
returns:
"string"
""
"with"
"2"
"spaces"

If you do not want this behavior, include a third argument as true.
splitstring("string  with 2 spaces", " ", true)
*/
vector<string> splitstring(string splitme, string delims, bool ignore_empty)
{
  vector<std::string> result;
  unsigned int splitter;
  while( (splitter = splitme.find_first_of(delims)) != splitme.npos)
  {
    if(ignore_empty && splitter > 0)
      result.push_back(splitme.substr(0,splitter));
    splitme = splitme.substr(splitter+1);
  }
  if(ignore_empty && splitme.length() > 0)
    result.push_back(splitme);
  return result;
}

/*
joinstring
This function does NOT ignore empty strings unless you tell it to.
example:
joinstring("hi" "im" "" "some" "vector", ",")
returns:
"hi,im,,some,vector"

If you do not want this behavior, include a third argument as true.
joinstring(somevectorofstrings, ",", true)
*/
string joinstring(vector<string> joinme, string delims, bool ignore_empty)
{
  string result;
  unsigned int i = 0;
  unsigned int joined = 0;
  for(i = 0; i < joinme.size(); i++)
  {
    if(ignore_empty && joinme[i].empty())
      continue;

    if(joined > 0)
      result += delims;

    result += joinme[i];
    joined++;
  }
  return result;
}

bool class_can_go(int ch_class, int room)
{
  bool classRestrictions = false;

  try {
    // Determine if any class restrictions are in place
    for (int c_class=1; c_class < CLASS_MAX; c_class++) {
      if (world[room].allow_class[c_class] == true) {
	classRestrictions = true;
      }
    }

    if (classRestrictions) {
      if (world[room].allow_class[ch_class] != true) {
	return false;
      }
    }
  } catch(...) {
    return false;
  }

  return true;
}

const char *find_profession(int c_class, uint8_t profession)
{
	//TODO Fix
  return "Unknown";

  map<uint8_t, string> profession_list = professions[c_class];
  
  if (profession == 0) {
    return "None";
  } else if (profession_list[profession].empty()) {
    return "Unknown";
  } else {
    return profession_list[profession].c_str();
  }
}


string get_isr_string(uint32 isr, int8_t loc)
{
   if(!IS_SET(isr,1<<loc)) {
      return string();
   }

   switch(loc) {
	case 0: return "Pierce";
	case 1: return "Slash";
	case 2: return "Magic";
	case 3: return "Charm";
	case 4: return "Fire";
	case 5: return "Energy";
	case 6: return "Acid";
	case 7: return "Poison";
	case 8: return "Sleep";
	case 9: return "Cold";
	case 10: return "Paralyze";
	case 11: return "Bludgeon";
	case 12: return "Whip";
	case 13: return "Crush";
	case 14: return "Hit";
	case 15: return "Bite";
	case 16: return "Sting";
	case 17: return "Claw";
	case 18: return "Physical";
	case 19: return "Non-Magic";
	case 20: return "Ki";
	case 21: return "Song";
	case 22: return "Water";
	case 23: return "Fear";
	default: return "ErCode: Somebodydunfuckedup";
   }
}

bool isDead(char_data *ch)
{
	return (ch && ch->position == POSITION_DEAD);
}

bool isNowhere(char_data *ch)
{
	return (ch && ch->in_room == NOWHERE);
}

bool file_exists(string filename)
{
  struct stat buffer;

  if (stat(filename.c_str(), &buffer) == 0) {
    return true;
  }

  return false;
}

bool char_file_exists(string name)
{
  if (all_of(name.begin(), name.end(), [](char i){return isalpha(i);}) == 0) {
    return false;
  }

  string filename;

  if (DC::instance().cf.bport) {
    filename = fmt::format("{}/{}/{}", BSAVE_DIR, name[0], name);
  } else {
    filename = fmt::format("{}/{}/{}", SAVE_DIR, name[0], name);
  }

  return file_exists(filename);
}
