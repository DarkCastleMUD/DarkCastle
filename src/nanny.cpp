/***************************************************************************
*  File: nanny.c, for people who haven't logged in        Part of DIKUMUD *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*                                                                         *
*  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
*  Performance optimization and bug fixes by MERC Industries.             *
*  You can use our stuff in any way you like whatsoever so long as this   *
*  copyright notice remains intact.  If you like it please drop a line    *
*  to mec@garnet.berkeley.edu.                                            *
*                                                                         *
*  This is free software and you are benefitting.  We hope that you       *
*  share your changes too.  What goes around, comes around.               *
***************************************************************************/
/* $Id: nanny.cpp,v 1.7 2002/07/31 19:12:30 pirahna Exp $ */
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef WIN32
#include <arpa/telnet.h>
#include <unistd.h>

#else
		#include <winsock2.h>
	#include <process.h>
	#include <mmsystem.h>

	// swipe some defined out of arpa telnet
	#define	IAC				255		/* interpret as command: */
	#define	WONT			252		/* I won't use option */
	#define	WILL			251		/* I will use option */
	#define TELOPT_NAOCRD	10	/* negotiate about CR disposition */
	#define TELOPT_ECHO		1	/* echo */
	#define TELOPT_NAOFFD	13	/* negotiate about formfeed disposition */
#endif
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <comm.h>
#include <connect.h>
#include <race.h>
#include <player.h>
#include <structs.h> // TRUE
#include <utility.h>
#include <character.h>
#include <levels.h>
#include <ki.h>
#include <fileinfo.h> // SAVE_DIR
#include <db.h> // init_char..
#include <mobile.h>
#include <interp.h>
#include <room.h>
#include <act.h>
#include <clan.h>
#include <spells.h>
#include <fight.h>

#define STATE(d)    ((d)->connected)


char echo_off_str [] = { (char)IAC, (char)WILL, (char)TELOPT_ECHO, (char)NULL };
char echo_on_str  [] = { (char)IAC, (char)WONT, (char)TELOPT_ECHO, (char)NULL };

char menu[] = "\n\rWelcome to Dark Castle Mud\n\r\n\r"
              "0) Exit Dark Castle.\n\r"
              "1) Enter the game.\n\r"
              "2) Enter your character's description.\n\r"
              "3) Change your password.\n\r"
              "4) Archive this character.\n\r"
              "5) Delete this character.\n\r\n\r"
              "   Make your choice: ";

bool wizlock = false;

extern char greetings1[MAX_STRING_LENGTH];  
extern char greetings2[MAX_STRING_LENGTH];
extern char greetings3[MAX_STRING_LENGTH];
extern char greetings4[MAX_STRING_LENGTH];
extern char webpage[MAX_STRING_LENGTH];
extern char motd[MAX_STRING_LENGTH];
extern char imotd[MAX_STRING_LENGTH];
extern CHAR_DATA *character_list;
extern struct descriptor_data *descriptor_list;
extern char *nonew_new_list[30];


#ifndef WIN32
extern "C" {
  char *crypt(const char *key, const char *salt);
}
#else
 
char *crypt(const char *key, const char *salt)
{
	return((char *)key);
}
#endif


int isbanned(char *hostname);
int _parse_name(char *arg, char *name);
bool check_deny( struct descriptor_data *d, char *name );
void update_wizlist(CHAR_DATA *ch);
void isr_set(CHAR_DATA *ch);
bool check_reconnect( struct descriptor_data *d, char *name, bool fReconnect );
bool check_playing( struct descriptor_data *d, char *name );

char *str_str(char *first, char *second);


int is_clss_eligible(CHAR_DATA *ch, int clss)
{
   int x = 0;
   
   switch(clss) {
   case CLASS_MAGIC_USER:
      if(GET_RAW_INT(ch) >= 12)
         x = 1;;
      break; 
   case CLASS_CLERIC:
      if(GET_RAW_WIS(ch) >= 13)
         x = 1;
      break; 
   case CLASS_THIEF:
      if(GET_RAW_DEX(ch) >= 13)
         x = 1;
      break; 
   case CLASS_WARRIOR:
      if(GET_RAW_STR(ch) >= 13 && GET_RAW_CON(ch) >= 12)
         x = 1;
      break; 
   case CLASS_ANTI_PAL:
      if(GET_RAW_INT(ch) >= 16 && GET_RAW_CON(ch) >= 14 &&
         (GET_RACE(ch) == RACE_HUMAN || GET_RACE(ch) == RACE_ORC))
         x = 1;
      break; 
   case CLASS_PALADIN:
      if(GET_RAW_WIS(ch) >= 16 && GET_RAW_CON(ch) >= 14 && 
         (GET_RACE(ch) == RACE_HUMAN || GET_RACE(ch) == RACE_ELVEN))
         x = 1;
      break; 
   case CLASS_BARBARIAN:
      if(GET_RAW_STR(ch) >= 16 && GET_RAW_CON(ch) >= 16)
         x = 1;
      break; 
   case CLASS_MONK:
      if(GET_RAW_CON(ch) >= 14)
         x = 1;
      break; 
   case CLASS_RANGER:
      if(GET_RAW_WIS(ch) >= 13 && GET_RAW_DEX(ch) >= 13)
         x = 1;
      break;
   case CLASS_BARD:
      if(GET_RAW_WIS(ch) >= 14 && GET_RAW_INT(ch) >= 14)
         x = 1;
      break;
   case CLASS_DRUID:
      if(GET_RAW_WIS(ch) >= 15 &&
          GET_RACE(ch) != RACE_GIANT && GET_RACE(ch) != RACE_ORC)
         x = 1;
      break; 
   }
   return(x);
}

void do_inate_race_abilities(char_data * ch)
{

   // Add race base saving throw mods
   // Yes, I could combine this 'switch' with the next one, but this is
   // alot more readable
   switch(GET_RACE(ch)) {
     case RACE_HUMAN:
       ch->saves[SAVE_TYPE_FIRE]   += RACE_HUMAN_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   += RACE_HUMAN_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] += RACE_HUMAN_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   += RACE_HUMAN_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  += RACE_HUMAN_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] += RACE_HUMAN_POISON_MOD;
       break;
     case RACE_ELVEN:
       ch->saves[SAVE_TYPE_FIRE]   += RACE_ELVEN_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   += RACE_ELVEN_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] += RACE_ELVEN_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   += RACE_ELVEN_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  += RACE_ELVEN_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] += RACE_ELVEN_POISON_MOD;
       break;
     case RACE_DWARVEN:
       ch->saves[SAVE_TYPE_FIRE]   += RACE_DWARVEN_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   += RACE_DWARVEN_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] += RACE_DWARVEN_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   += RACE_DWARVEN_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  += RACE_DWARVEN_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] += RACE_DWARVEN_POISON_MOD;
       break;
     case RACE_GIANT:
       ch->saves[SAVE_TYPE_FIRE]   += RACE_GIANT_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   += RACE_GIANT_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] += RACE_GIANT_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   += RACE_GIANT_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  += RACE_GIANT_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] += RACE_GIANT_POISON_MOD;
       break;
     case RACE_PIXIE:
       ch->saves[SAVE_TYPE_FIRE]   += RACE_PIXIE_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   += RACE_PIXIE_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] += RACE_PIXIE_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   += RACE_PIXIE_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  += RACE_PIXIE_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] += RACE_PIXIE_POISON_MOD;
       break;
     case RACE_HOBBIT:
       ch->saves[SAVE_TYPE_FIRE]   += RACE_HOBBIT_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   += RACE_HOBBIT_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] += RACE_HOBBIT_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   += RACE_HOBBIT_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  += RACE_HOBBIT_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] += RACE_HOBBIT_POISON_MOD;
       break;
     case RACE_GNOME:
       ch->saves[SAVE_TYPE_FIRE]   += RACE_GNOME_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   += RACE_GNOME_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] += RACE_GNOME_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   += RACE_GNOME_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  += RACE_GNOME_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] += RACE_GNOME_POISON_MOD;
       break;
     case RACE_ORC:
       ch->saves[SAVE_TYPE_FIRE]   += RACE_ORC_FIRE_MOD;
       ch->saves[SAVE_TYPE_COLD]   += RACE_ORC_COLD_MOD;
       ch->saves[SAVE_TYPE_ENERGY] += RACE_ORC_ENERGY_MOD;
       ch->saves[SAVE_TYPE_ACID]   += RACE_ORC_ACID_MOD;
       ch->saves[SAVE_TYPE_MAGIC]  += RACE_ORC_MAGIC_MOD;
       ch->saves[SAVE_TYPE_POISON] += RACE_ORC_POISON_MOD;
       break;
     default: 
       break;
   }

// TODO - Add in realistic race abilities
/*
   struct affected_type af;

   switch(GET_RACE(ch)) {
   case RACE_PIXIE:
      af.type = SPELL_FLY;
      af.duration = -1;
      af.modifier = 0;
      af.location = 0;
      af.bitvector = AFF_FLYING;
      affect_to_char(ch, &af);
      break;

   case RACE_HOBBIT:
      af.type = SPELL_DETECT_MAGIC;
      af.duration = -1;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_DETECT_MAGIC;
      affect_to_char(ch, &af);

      af.type = SPELL_DETECT_EVIL;
      af.duration = -1;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_DETECT_MAGIC;
      affect_to_char(ch, &af);

      af.type = SPELL_DETECT_GOOD;
      af.duration = -1;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_DETECT_GOOD;
      affect_to_char(ch, &af);
      break;

   case RACE_ELVEN:
      af.type = SPELL_DETECT_INVISIBLE;
      af.duration = -1;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_DETECT_INVISIBLE;
      affect_to_char(ch, &af);
      break;

   case RACE_DWARVEN:
      af.type = SPELL_INFRAVISION;
      af.duration = -1;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_INFRARED;
      affect_to_char(ch, &af);
      break;

   case RACE_GIANT:
      af.type = SPELL_RESIST_COLD;
      af.duration = -1;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = 0;
      affect_to_char(ch, &af);
      break;

   default:
      break;
   }
*/
}

// stuff that has to be done on both a normal login, as well as on 
// a hotboot login
void do_on_login_stuff(char_data * ch)
{
    void add_to_bard_list(char_data * ch);

    add_to_bard_list(ch);
    ch->pcdata->bad_pw_tries = 0;
    redo_hitpoints (ch);
    redo_mana (ch);

    do_inate_race_abilities(ch);

    // add character base saves to saving throws
    for(int i = 0; i <= SAVE_TYPE_MAX; i++) {
      ch->saves[i] += GET_LEVEL(ch)/2;
      ch->saves[i] += ch->pcdata->saves_mods[i];
    }

    if(GET_TITLE(ch) == NULL) {
       GET_TITLE(ch) = str_dup("is a virgin.");
    }
    
    if (GET_CLASS(ch) == CLASS_MONK)
    {
       GET_AC(ch) -= (GET_LEVEL(ch) * 2);
    }

    /* Set ISR's cause they're not saved...   */
    isr_set(ch);
    
    if(!IS_MOB(ch) && GET_LEVEL(ch) >= IMMORTAL) {
       ch->pcdata->holyLite   = TRUE;
       ch->pcdata->wizinvis = GET_LEVEL(ch);
       GET_COND(ch, THIRST) = -1;
       GET_COND(ch, FULL) = -1;
    }

    if(ch->in_room >= 2)                 char_to_room( ch, ch->in_room );
    else if(GET_LEVEL(ch) >=  IMMORTAL)  char_to_room( ch, real_room(17) );
    else                                 char_to_room( ch, real_room(START_ROOM) );
}

void roll_and_display_stats(CHAR_DATA * ch)
{
  int x, a, b;
  char buf[MAX_STRING_LENGTH];

         for(x = 0; x <= 4; x++) {
            a=dice(6, 3); b=dice(6, 3); ch->desc->stats->str[x] = ((a > b) ? a : b);
            a=dice(6, 3); b=dice(6, 3); ch->desc->stats->tel[x] = ((a > b) ? a : b);
            a=dice(6, 3); b=dice(6, 3); ch->desc->stats->wis[x] = ((a > b) ? a : b);
            a=dice(6, 3); b=dice(6, 3); ch->desc->stats->dex[x] = ((a > b) ? a : b);
            a=dice(6, 3); b=dice(6, 3); ch->desc->stats->con[x] = ((a > b) ? a : b);
         }

         SEND_TO_Q("\n\r  Choose from any of the following groups of abilities...     \n\r", ch->desc);
         
         SEND_TO_Q( "Group: 1     2     3     4     5\n\r",ch->desc); 
         sprintf(buf,"Str:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
            ch->desc->stats->str[0], ch->desc->stats->str[1], ch->desc->stats->str[2],
            ch->desc->stats->str[3], ch->desc->stats->str[4]);
         SEND_TO_Q(buf, ch->desc);
         sprintf(buf,"Int:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
            ch->desc->stats->tel[0], ch->desc->stats->tel[1], ch->desc->stats->tel[2],
            ch->desc->stats->tel[3], ch->desc->stats->tel[4]);
         SEND_TO_Q(buf, ch->desc);
         sprintf(buf,"Wis:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
            ch->desc->stats->wis[0], ch->desc->stats->wis[1], ch->desc->stats->wis[2],
            ch->desc->stats->wis[3], ch->desc->stats->wis[4]);
         SEND_TO_Q(buf, ch->desc);
         sprintf(buf,"Dex:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
            ch->desc->stats->dex[0], ch->desc->stats->dex[1], ch->desc->stats->dex[2],
            ch->desc->stats->dex[3], ch->desc->stats->dex[4]);
         SEND_TO_Q(buf, ch->desc);
         sprintf(buf,"Con:   %-2d    %-2d    %-2d    %-2d    %-2d\n\r",
            ch->desc->stats->con[0], ch->desc->stats->con[1], ch->desc->stats->con[2],
            ch->desc->stats->con[3], ch->desc->stats->con[4]);
         SEND_TO_Q(buf, ch->desc);
         SEND_TO_Q("Choose a group <1-5>, or press return to reroll --> ", ch->desc);
}

// Deal with sockets that haven't logged in yet.
void nanny(struct descriptor_data *d, char *arg)
{
   char    buf[MAX_STRING_LENGTH];
   char    str_tmp[25];
   char    tmp_name[20];
   bool    fOld;
   struct  char_data *ch;
   int x, y;
   char    badclssmsg[] = "You must choose a class that matches your stats. These are marked by a '*'.\n\rSelect a class-> ";
   
   CHAR_DATA *get_pc(char *name);
   void remove_clan_member(int clannumber, struct char_data * ch);
   
   ch  = d->character;
   if(arg) for ( ; isspace(*arg); arg++ );
   
   switch (STATE(d))
   {
      
   default:
      log( "Nanny: illegal STATE(d)", 0, LOG_BUG );
      close_socket(d);
      return;
      
   case CON_PRE_DISPLAY_ENTRANCE:
      // _shouldn't_ get here, but if we do, let it fall through to CON_DISPLAY_ENTRANCE
      // This is here to allow the mud to 'skip' this descriptor until the next pulse on
      // a new connection.  That allows the "GET" and "POST" from a webbrowser to get there.
      // no break;

   case CON_DISPLAY_ENTRANCE:

      // Check for people trying to connect to a webserver
      if(!strncmp(arg, "GET", 3) || !strncmp(arg, "POST", 4)) {
         // send webpage
         SEND_TO_Q(  webpage, d );
         close_socket( d );
         return;
      }

      // if it's not a webbrowser, display the entrance greeting

      x = number(1, 100);
    //  if(x < 25)
      SEND_TO_Q(greetings1, d);
/* TODO - remove this comment line when we're back up
      else if(x < 50)
        SEND_TO_Q(greetings2, d);
      else if(x < 75)
        SEND_TO_Q(greetings3, d);
      else
        SEND_TO_Q(greetings4, d);
*/

      SEND_TO_Q("What name for the roster? ", d);
      STATE(d) = CON_GET_NAME;

      // if they have already entered their name, drop through.  Otherwise stop and wait for input
      if(!*arg)
        break;

   case CON_GET_NAME:

      if (!*arg) { 
         close_socket( d ); 
         return; 
      }

      // Capitlize first letter, lowercase rest      
      arg[0] = UPPER(arg[0]);
      for(y = 1; arg[y] != '\0'; y++)
         arg[y] = LOWER(arg[y]);
      
      if ( _parse_name(arg, tmp_name)) { 
         SEND_TO_Q( "Illegal name, try another.\n\rName: ", d );
         return;
      }
      
      if (check_deny(d, tmp_name))
         return;

// Uncomment this if you think a playerfile may be crashing the mud. -pir
//      sprintf(str_tmp, "Trying to login: %s", tmp_name);
//      log(str_tmp, 0, LOG_MISC);

      // ch is allocated in load_char_obj
      fOld     = load_char_obj( d, tmp_name );
      if(!fOld) {
         sprintf(str_tmp, "../archive/%s.gz", tmp_name);
         if(file_exists(str_tmp))
         {
            SEND_TO_Q("That character is archived.\n\rPlease mail "
               "Pirahna with unarchive requests.\n\r", d);
            close_socket(d);
            return;
         }
      }
      ch    = d->character;
      
      // This is needed for "check_reconnect"  we free it later during load_char_obj
      // TODO - this is memoryleaking ch->name.  Check if ch->name is not there before
      // doing it to fix it.  (No time to verify this now, so i'll do it later)
      GET_NAME(ch) = str_dup(tmp_name);
      
      if(check_reconnect(d, tmp_name, FALSE))
         fOld = TRUE;
      
      else if((wizlock) && strcmp(GET_NAME(ch),"Sadus") &&
         strcmp(GET_NAME(ch),"Pirahna") &&
         strcmp(GET_NAME(ch),"Valkyrie"))
      {
         SEND_TO_Q( "The game is wizlocked.\n\r", d );
         close_socket( d );
         return;
      }
      
      if ( fOld ) {
         /* Old player */
         SEND_TO_Q( "Password: ", d );
         SEND_TO_Q( echo_off_str, d );
         STATE(d) = CON_GET_OLD_PASSWORD;
         return;
      }
      else {
         /* New player */
         sprintf( buf, "Did I get that right, %s (Y/N)? ", tmp_name );
         SEND_TO_Q( buf, d );
         STATE(d) = CON_CONFIRM_NEW_NAME;
         return;
      }
      break;
      
   case CON_GET_OLD_PASSWORD:
      SEND_TO_Q( "\n\r", d );
      
      if(strncmp( (char *)crypt((char *)arg, (char *)ch->pcdata->pwd), ch->pcdata->pwd, (PASSWORD_LEN) )) {
         SEND_TO_Q( "Wrong password.\n\r", d );
         sprintf(log_buf, "%s wrong password: %s", GET_NAME(ch), d->host);
         log( log_buf, SERAPH, LOG_SOCKET );
         if((ch = get_pc(GET_NAME(d->character))))
         {
            sprintf(log_buf, "$4$BWARNING: Someone just tried to log in as you with the wrong password.\r\n"
               "Attempt was from %s.$R\r\n"
               "(If it's only once or twice, you can ignore it.  If it's several dozen tries, let a god know.)\r\n", d->host);
            send_to_char(log_buf, ch);
         }
         else {
            if(d->character->pcdata->bad_pw_tries > 100) {
              sprintf(log_buf, "%s has 100+ bad pw tries...", GET_NAME(ch));
              log(log_buf, SERAPH, LOG_SOCKET);
            }
            else {
              d->character->pcdata->bad_pw_tries++;
              save_char_obj(d->character);
            }
         }
         close_socket(d);
         return;
      }
      
      SEND_TO_Q( echo_on_str, d ); 
      
      check_playing(d, GET_NAME(ch));
      
      if(check_reconnect(d, GET_NAME(ch), TRUE))
         return;
      
      sprintf( log_buf, "%s@%s has connected.", GET_NAME(ch), d->host);
      if(GET_LEVEL(ch) < ANGEL)
         log( log_buf, DEITY, LOG_SOCKET );
      else
         log( log_buf, GET_LEVEL(ch), LOG_SOCKET );
      
      //    SEND_TO_Q(motd, d);
      if(GET_LEVEL(ch) < IMMORTAL)
        colorCharSend(motd, d->character);
      else colorCharSend(imotd, d->character);

      sprintf(log_buf, "\n\rLast connected from:\n\r%s\n\r", ch->pcdata->last_site); 
      SEND_TO_Q(log_buf, d);
      
      if(d->character->pcdata->bad_pw_tries)
      {
         sprintf(buf, "\r\n\r\n$4$BYou have had %d wrong passwords entered since your last complete login.$R\r\n\r\n", d->character->pcdata->bad_pw_tries);
         SEND_TO_Q(buf, d);
      }
      STATE(d) = CON_READ_MOTD;
      break;
      
   case CON_CONFIRM_NEW_NAME:
      switch (*arg)
      {
      case 'y': case 'Y':

         if(isbanned(d->host) >= BAN_NEW) {
            sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
               GET_NAME(d->character), d->host);
            log(buf, ANGEL, LOG_SOCKET);
            SEND_TO_Q("Sorry, new chars are not allowed from your site.\n\r"
               "Questions may be directed to Pirahna at pirahna@dcastle.ad1440.net\n\r",
               d);
            STATE(d) = CON_CLOSE;
            return;
         }
         sprintf( buf, "New character.\n\rGive me a password for %s: ", GET_NAME(ch) );
         SEND_TO_Q( buf, d );
         SEND_TO_Q( echo_off_str, d );
         STATE(d) = CON_GET_NEW_PASSWORD;
         // at this point, pcdata hasn't yet been created.  So we're going to go ahead and
         // allocate it since a new character is obviously a PC
#ifdef LEAK_CHECK
         ch->pcdata = (pc_data *)calloc(1, sizeof(pc_data));
#else
         ch->pcdata = (pc_data *)dc_alloc(1, sizeof(pc_data));
#endif
         break;
         
      case 'n': case 'N':
         SEND_TO_Q( "Ok, what IS it, then? ", d );
         // TODO - double check this to make sure we're free'ing properly
         dc_free( GET_NAME(ch) );
         dc_free( d->character );
         d->character = 0;
         STATE(d) = CON_GET_NAME;
         break;
         
      default:
         SEND_TO_Q( "Please type Yes or No? ", d );
         break;
      }
      break;
      
   case CON_GET_NEW_PASSWORD:
      SEND_TO_Q("\n\r", d);
      
      if(arg == 0 || strlen(arg) < 6) {
         SEND_TO_Q("Password must be at least six characters long.\n\rPassword: ", d );
         return;
      }
      
      strncpy( ch->pcdata->pwd, (char *)crypt((char *)arg, (char *)ch->name), PASSWORD_LEN );
      ch->pcdata->pwd[PASSWORD_LEN] = '\0';
      SEND_TO_Q( "Please retype password: ", d );
      STATE(d) = CON_CONFIRM_NEW_PASSWORD;
      break;
      
   case CON_CONFIRM_NEW_PASSWORD:
      SEND_TO_Q( "\n\r", d );
      
      if(strncmp( (char *)crypt((char *)arg, (char *)ch->pcdata->pwd ), ch->pcdata->pwd, PASSWORD_LEN )) {
         SEND_TO_Q("Passwords don't match.\n\rRetype password: ", d );
         STATE(d) = CON_GET_NEW_PASSWORD;
         return;
      }
      
      SEND_TO_Q( echo_on_str, d );
      SEND_TO_Q( "What is your sex (M/F)? ", d );
      STATE(d) = CON_GET_NEW_SEX;
      break;
      
      
   case CON_GET_NEW_SEX:
      switch ( *arg )
      {
      case 'm': case 'M':
         ch->sex  = SEX_MALE;
         break;
      case 'f': case 'F':
         ch->sex  = SEX_FEMALE;
         break;
      default:
         SEND_TO_Q( "That's not a sex.\n\rWhat IS your sex? ", d );
         return;
      }

#ifdef LEAK_CHECK
      ch->desc->stats = (struct stat_shit *)calloc(1, sizeof(struct stat_shit));
#else
      ch->desc->stats = (struct stat_shit *)dc_alloc(1, sizeof(struct stat_shit));
#endif
      
      STATE(d) = CON_CHOOSE_STATS;
      *arg = '\0';
      // break;  no break on purpose...might as well do this now.  no point in waiting
      
      case CON_CHOOSE_STATS:
         
         if(*arg != '\0')
            *(arg+1) = '\0';

         // first time, this hasthe m/f from sex and goes right through
         
         if((y = atoi(arg)) && (y <= 5) && y >= 1) {
            GET_RAW_STR(ch) = ch->desc->stats->str[y-1];
            GET_RAW_INT(ch) = ch->desc->stats->tel[y-1];
            GET_RAW_WIS(ch) = ch->desc->stats->wis[y-1];
            GET_RAW_DEX(ch) = ch->desc->stats->dex[y-1];
            GET_RAW_CON(ch) = ch->desc->stats->con[y-1];
            dc_free(ch->desc->stats);
            SEND_TO_Q("\n\rChoose a race.\n\r", d );
            sprintf(buf, "   1: Human\n\r   2: Elf\n\r   3: Dwarf\n\r"
               "   4: Hobbit\n\r   5: Pixie\n\r   6: Giant\n\r"
               "   7: Gnome\r\n   8: Orc\r\n"
               "\n\rSelect a race-> ");
            SEND_TO_Q(buf, d);
            STATE(d) = CON_GET_RACE;
            break;
         }
         roll_and_display_stats(ch);
         break;
         
      case CON_GET_RACE:
         switch ( *arg )
         {
         default:
            SEND_TO_Q( "That's not a race.\n\rWhat IS your race? ", d );
            return;
            
         case '1':
            ch->race = RACE_HUMAN;
            ch->height = number(48, 84);
            ch->weight = number(100, 140);
            GET_RAW_STR(ch) += RACE_HUMAN_STR_MOD;
            GET_RAW_INT(ch) += RACE_HUMAN_INT_MOD;
            GET_RAW_WIS(ch) += RACE_HUMAN_WIS_MOD;
            GET_RAW_DEX(ch) += RACE_HUMAN_DEX_MOD;
            GET_RAW_CON(ch) += RACE_HUMAN_CON_MOD;
            break;
            
         case '2':
            ch->race = RACE_ELVEN;
            ch->height = number(71, 84);
            ch->weight = number(70, 90);
            ch->alignment = 1000;
            GET_RAW_STR(ch) += RACE_ELVEN_STR_MOD;
            GET_RAW_INT(ch) += RACE_ELVEN_INT_MOD;
            GET_RAW_WIS(ch) += RACE_ELVEN_WIS_MOD;
            GET_RAW_DEX(ch) += RACE_ELVEN_DEX_MOD;
            GET_RAW_CON(ch) += RACE_ELVEN_CON_MOD;
            break;
            
         case '3':
            ch->race = RACE_DWARVEN;
            ch->height = number(51, 60);
            ch->weight = number(80, 110);
            GET_RAW_STR(ch) += RACE_DWARVEN_STR_MOD;
            GET_RAW_INT(ch) += RACE_DWARVEN_INT_MOD;
            GET_RAW_WIS(ch) += RACE_DWARVEN_WIS_MOD;
            GET_RAW_DEX(ch) += RACE_DWARVEN_DEX_MOD;
            GET_RAW_CON(ch) += RACE_DWARVEN_CON_MOD;
            break;
            
         case '4':
            ch->race = RACE_HOBBIT;
            ch->height = number(36, 48);
            ch->weight = number(50, 80);
            GET_RAW_STR(ch) += RACE_HOBBIT_STR_MOD;
            GET_RAW_INT(ch) += RACE_HOBBIT_INT_MOD;
            GET_RAW_WIS(ch) += RACE_HOBBIT_WIS_MOD;
            GET_RAW_DEX(ch) += RACE_HOBBIT_DEX_MOD;
            GET_RAW_CON(ch) += RACE_HOBBIT_CON_MOD;
            break;
            
         case '5':
            ch->race = RACE_PIXIE;
            ch->height = number(24, 38);
            ch->weight = number(20, 30);
            GET_RAW_STR(ch) += RACE_PIXIE_STR_MOD;
            GET_RAW_INT(ch) += RACE_PIXIE_INT_MOD;
            GET_RAW_WIS(ch) += RACE_PIXIE_WIS_MOD;
            GET_RAW_DEX(ch) += RACE_PIXIE_DEX_MOD;
            GET_RAW_CON(ch) += RACE_PIXIE_CON_MOD;
            break;
            
         case '6':
            ch->race = RACE_GIANT;
            ch->height = number(85, 100);
            ch->weight = number(150, 200);
            GET_RAW_STR(ch) += RACE_GIANT_STR_MOD;
            GET_RAW_INT(ch) += RACE_GIANT_INT_MOD;
            GET_RAW_WIS(ch) += RACE_GIANT_WIS_MOD;
            GET_RAW_DEX(ch) += RACE_GIANT_DEX_MOD;
            GET_RAW_CON(ch) += RACE_GIANT_CON_MOD;
            break;

         case '7':
            ch->race = RACE_GNOME;
            ch->height = number(36, 48);
            ch->weight = number(50, 70);
            GET_RAW_STR(ch) += RACE_GNOME_STR_MOD;
            GET_RAW_INT(ch) += RACE_GNOME_INT_MOD;
            GET_RAW_WIS(ch) += RACE_GNOME_WIS_MOD;
            GET_RAW_DEX(ch) += RACE_GNOME_DEX_MOD;
            GET_RAW_CON(ch) += RACE_GNOME_CON_MOD;
            break;
            
         case '8':
            ch->race = RACE_ORC;
            ch->height = number(71, 84);
            ch->weight = number(130, 170);
            ch->alignment = -1000;
            GET_RAW_STR(ch) += RACE_ORC_STR_MOD;
            GET_RAW_INT(ch) += RACE_ORC_INT_MOD;
            GET_RAW_WIS(ch) += RACE_ORC_WIS_MOD;
            GET_RAW_DEX(ch) += RACE_ORC_DEX_MOD;
            GET_RAW_CON(ch) += RACE_ORC_CON_MOD;
            break;
         }
         
         SEND_TO_Q("\n\rA '*' denotes a class that fits your chosen stats.\n\r", d );
         sprintf(buf, " %c 1: Warrior\n\r"
                      " %c 2: Cleric\n\r"
                      " %c 3: Magic User\n\r"
                      " %c 4: Thief\n\r"
                      " %c 5: Anti-Paladin\n\r"
                      " %c 6: Paladin\n\r"
                      " %c 7: Barbarian \n\r"
                      " %c 8: Monk\n\r"
                      " %c 9: Ranger\n\r"
                      " %c 10: Bard\n\r"
                      " %c 11: Druid\n\r"
                      "\n\rSelect a class-> ", 
            (is_clss_eligible(ch, CLASS_WARRIOR) ? '*' : ' '),
            (is_clss_eligible(ch, CLASS_CLERIC) ? '*' : ' '),
            (is_clss_eligible(ch, CLASS_MAGIC_USER) ? '*' : ' '),
            (is_clss_eligible(ch, CLASS_THIEF) ? '*' : ' '),
            (is_clss_eligible(ch, CLASS_ANTI_PAL) ? '*' : ' '),
            (is_clss_eligible(ch, CLASS_PALADIN) ? '*' : ' '),
            (is_clss_eligible(ch, CLASS_BARBARIAN) ? '*' : ' '),
            (is_clss_eligible(ch, CLASS_MONK) ? '*' : ' '),
//            (is_clss_eligible(ch, CLASS_RANGER) ? '*' : ' '),
// Rangers are always eligible
            '*',
            (is_clss_eligible(ch, CLASS_BARD) ? '*' : ' '), 
            (is_clss_eligible(ch, CLASS_DRUID) ? '*' : ' ') 
            );
         SEND_TO_Q(buf, d);
         STATE(d) = CON_GET_NEW_CLASS;
         break;
         
    case CON_GET_NEW_CLASS:
       switch ( atoi(arg) )
       {
       default:
          SEND_TO_Q( "That's not a class.\n\rWhat IS your class? ", d );
          return;

       case 1:
          if(!is_clss_eligible(ch, CLASS_WARRIOR)) 
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_WARRIOR;
          break;
       case 2:
          if(!is_clss_eligible(ch, CLASS_CLERIC))
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_CLERIC;
          break;
       case 3:
          if(!is_clss_eligible(ch, CLASS_MAGIC_USER))
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_MAGIC_USER;
          break;
       case 4:
          if(!is_clss_eligible(ch, CLASS_THIEF))
           { SEND_TO_Q(badclssmsg, d);  return; }
          SEND_TO_Q( "Pstealing is legal on this mud :)\n\r"
                     "Check 'help psteal' before you try though.", d );
          GET_CLASS(ch) = CLASS_THIEF;
          break;
       case 5:
          if(!is_clss_eligible(ch, CLASS_ANTI_PAL)) 
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_ANTI_PAL;
          GET_ALIGNMENT(ch) = -1000;
          break;
       case 6:
          if(!is_clss_eligible(ch, CLASS_PALADIN)) 
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_PALADIN;
          GET_ALIGNMENT(ch) = 1000;
          break;
       case 7:
          if(!is_clss_eligible(ch, CLASS_BARBARIAN)) 
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_BARBARIAN;
          break;
       case 8:
          if(!is_clss_eligible(ch, CLASS_MONK)) 
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_MONK;
          break;
       case 9:
// TODO - Eventually, we should put in something to make sure that you always get at least one
//   class that you can be.  Until then we'll just make ranger's always choosable.
//          if(!is_clss_eligible(ch, CLASS_RANGER)) 
//           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_RANGER;
          break;
       case 10:
          if(!is_clss_eligible(ch, CLASS_BARD)) 
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_BARD;
          break;
       case 11:
          if(!is_clss_eligible(ch, CLASS_DRUID)) 
           { SEND_TO_Q(badclssmsg, d);  return; }
          GET_CLASS(ch) = CLASS_DRUID;
          break;
       }
       
       init_char( ch );

       sprintf( log_buf, "%s@%s new player.", GET_NAME(ch), d->host );
       log( log_buf, ANGEL, LOG_SOCKET );
       SEND_TO_Q( "\n\r", d );
       SEND_TO_Q( motd, d );
       STATE(d) = CON_READ_MOTD;
       break;
       
    case CON_READ_MOTD:
       SEND_TO_Q( menu, d );
       STATE(d) = CON_SELECT_MENU;
       break;
       
    case CON_SELECT_MENU:
       switch( *arg )
       {
       case '0':
          close_socket( d );
          d = NULL;
          break;
          
       case '1':
          // I believe this is here to stop a dupe bug 
          // by logging in twice, and leaving one at the password: prompt
          if(GET_LEVEL(ch) > 0) {
             strcpy(tmp_name, GET_NAME(ch));
             free_char(d->character);
             d->character = 0;
             load_char_obj(d, tmp_name);
             ch = d->character;
          }
          
          send_to_char("\n\rWelcome to Dark Castle Diku Mud.  May your visit here suck.\n\r", ch );
          ch->next            = character_list;
          character_list      = ch;
          
          /* tjs hack - till mob bug is fixed */
          // TODO - figure out what this is for
          if (GET_LEVEL(ch) == 1) {
             advance_level(ch, 0);
             advance_level(ch, 0);
          }
          
          do_on_login_stuff(ch);
          
          if(GET_LEVEL(ch) < OVERSEER)
             clan_login(ch);
          
          act( "$n has entered the game.", ch, 0, 0, TO_ROOM , INVIS_NULL);
          if(!GET_SHORT_ONLY(ch)) GET_SHORT_ONLY(ch) = str_dup(GET_NAME(ch)); 
          update_wizlist(ch);
          
          STATE(d) = CON_PLAYING;
          if ( GET_LEVEL(ch) == 0 )
             do_start( ch );
          do_look( ch, "", 8 );
          break;
          
       case '2':
          SEND_TO_Q("Enter a text you'd like others to see when they look at you.\n\r"
                    "Terminate with an '~'\n\r", d);
          if(ch->description) {
             SEND_TO_Q("Old description:\n\r", d);
             SEND_TO_Q(ch->description, d);
             dc_free(ch->description);
          }
#ifdef LEAK_CHECK
          ch->description = (char *)calloc(240, sizeof(char));
#else
          ch->description = (char *)dc_alloc(240, sizeof(char));
#endif

 // TODO - what happens if I get to this point, then disconnect, and reconnect?  memory leak?

          d->str     = &ch->description;
          d->max_str = 239;
          STATE(d)   = CON_EXDSCR;
          break;
      
       case '3':
          SEND_TO_Q( "Enter current password: ", d );
          SEND_TO_Q( echo_off_str, d );
          STATE(d) = CON_CONFIRM_PASSWORD_CHANGE;
          break;
      
       case '4':
          // Archive the charater 
          SEND_TO_Q("This will archive your character until you ask to be unarchived.\n\rYou will need to speak to a god greater than level 107.\n\rType ARCHIVE ME if this is what you want:  ", d);
          STATE(d) = CON_ARCHIVE_CHAR;
          break;
      
       case '5':
          // delete this character 
          SEND_TO_Q("This will _permanently_ erase you.\n\rType ERASE ME if this is really what you want: ", d);
          STATE(d) = CON_DELETE_CHAR;
          break;
      
       default:
          SEND_TO_Q( menu, d );
          break;
       }
       break;
   
    case CON_ARCHIVE_CHAR:
       if(!strcmp(arg, "ARCHIVE ME")) {
          strcpy(str_tmp, GET_NAME(d->character));
          SEND_TO_Q("\n\rCharacter Archived.\n\r", d);
          update_wizlist(d->character);
          close_socket(d);
          util_archive(str_tmp, 0);
       }
       else {
          STATE(d) = CON_SELECT_MENU;
          SEND_TO_Q(menu, d);
       }
       break;
       
    case CON_DELETE_CHAR:
       if(!strcmp(arg, "ERASE ME")) {
          sprintf(buf, "%s just deleted themself.", d->character->name);
          log(buf, IMMORTAL, LOG_MORTAL);
          if(d->character->clan)
             remove_clan_member(d->character->clan, d->character);
          sprintf(buf, "rm -f %s/%c/%s", SAVE_DIR, d->character->name[0], d->character->name);
          update_wizlist(d->character);
          close_socket(d); 
          d = NULL;
          system (buf);
       }
       else {
          STATE(d) = CON_SELECT_MENU;
          SEND_TO_Q( menu, d );
       }
       break;
       
    case CON_CONFIRM_PASSWORD_CHANGE:
       SEND_TO_Q( "\n\r", d );
       if(!strncmp( (char *)crypt((char *)arg, (char *)ch->pcdata->pwd), ch->pcdata->pwd, PASSWORD_LEN)) {
          SEND_TO_Q( "Enter a new password: ", d );
          SEND_TO_Q( echo_off_str, d );
          STATE(d) = CON_RESET_PASSWORD;
          break;
       } else {
          SEND_TO_Q( "Incorrect.", d);
          SEND_TO_Q( echo_on_str, d);
          STATE(d) = CON_SELECT_MENU;
          SEND_TO_Q( menu, d );
       }
       break;
       
    case CON_RESET_PASSWORD:
       SEND_TO_Q( "\n\r", d );
       
       if(strlen(arg) < 6) {
          SEND_TO_Q("Password must be at least six characters long.\n\rPassword: ", d );
          return;
       }
       strncpy( ch->pcdata->pwd, (char *)crypt((char *)arg, (char *)ch->name ), PASSWORD_LEN );
       ch->pcdata->pwd[PASSWORD_LEN] = '\0';
       SEND_TO_Q( "Please retype password: ", d );
       STATE(d) = CON_CONFIRM_RESET_PASSWORD;
       break;
       
    case CON_CONFIRM_RESET_PASSWORD:
       SEND_TO_Q( "\n\r", d );
       
       if (strncmp( (char *)crypt((char *)arg, (char *)ch->pcdata->pwd ), ch->pcdata->pwd, PASSWORD_LEN )) {
          SEND_TO_Q( "Passwords don't match.\n\rRetype password: ", d );
          STATE(d) = CON_RESET_PASSWORD;
          return;
       }
       
       SEND_TO_Q( echo_on_str, d);
       SEND_TO_Q( "\n\rDone.\n\r", d );
       SEND_TO_Q( menu, d );
       STATE(d)    = CON_SELECT_MENU;
       if(GET_LEVEL(ch) > 1) {
          char blah1[50], blah2[50];
          // this prevents a dupe bug
          strcpy(blah1, GET_NAME(ch));
          strcpy(blah2, ch->pcdata->pwd);
          free_char(d->character);
          d->character = 0;
          load_char_obj(d, blah1);
          ch = d->character;
          strcpy(ch->pcdata->pwd, blah2);
          save_char_obj(ch);
          sprintf(log_buf, "%s password changed", GET_NAME(ch));
          log( log_buf, SERAPH, LOG_SOCKET );
       }
       
       break;
    case CON_CLOSE:
       close_socket(d);
       break;
    }
}



// Parse a name for acceptability.
int _parse_name(char *arg, char *name)
{
   int i;
   
   /* skip whitespaces */
   for (; isspace(*arg); arg++);
   
   for (i = 0; (name[i] = arg[i]) != '\0'; i++)
   {
      if ( name[i] < 0 || !isalpha(name[i]) || i > MAX_NAME_LENGTH )
         return 1;
   }
   
   if ( i < 2 )
      return 1;
   
   if ( !str_cmp( name, "all" )        || !str_cmp( name, "local" ) ||
      !str_cmp( name, "at" )           || !str_cmp( name, "out" )   ||
      !str_cmp( name, "in" )           || !str_cmp( name, "someone") ||
      !str_cmp( name, "warrior" )      || !str_cmp( name, "cleric") ||
      !str_cmp( name, "bard" )         || !str_cmp( name, "barbarian") ||
      !str_cmp( name, "monk" )         || !str_cmp( name, "druid") ||
      !str_cmp( name, "ranger" )       || !str_cmp( name, "god") ||
      !str_cmp( name, "antipaladin" )  || !str_cmp( name, "paladin") ||
      !str_cmp( name, "only" )         || !str_cmp( name, "mage") ||
      !str_cmp( name, "the")           || !str_cmp( name, "to") ||
      !str_cmp( name, "someone")       || !str_cmp( name, "through") ||
      !str_cmp( name, "pc")            || !str_cmp( name, "corpse")
      )
      return 1;
   
   return 0;
}



// Check for denial of service.
bool check_deny( struct descriptor_data *d, char *name )
{
   FILE *  fpdeny  = NULL;
   char    strdeny[MAX_INPUT_LENGTH];
   char    bufdeny[MAX_STRING_LENGTH];
   
   sprintf( strdeny, "%s/%c/%s.deny", SAVE_DIR, UPPER(name[0]), name );
   if ( ( fpdeny = dc_fopen( strdeny, "rb" ) ) == NULL )
      return FALSE;
   dc_fclose( fpdeny );
   
   sprintf( log_buf, "Denying access to player %s@%s.", name, d->host );
   log( log_buf, ARCHANGEL, LOG_MORTAL );
   file_to_string( strdeny, bufdeny );
   SEND_TO_Q( bufdeny, d );
   close_socket( d );
   return TRUE;
}



// Look for link-dead player to reconnect.
bool check_reconnect( struct descriptor_data *d, char *name, bool fReconnect )
{
   CHAR_DATA * tmp_ch;

   for(tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next) {
      if(IS_NPC(tmp_ch) || tmp_ch->desc != NULL)
         continue;
      
      if(str_cmp( GET_NAME(d->character), GET_NAME(tmp_ch)))
         continue;

      if(GET_LEVEL(d->character) < 2)
         continue;

      if(fReconnect == FALSE)
      {
         // TODO - why are we doing this?  we load the password doing load_char_obj
         // unless someone changed their password and didn't save this doesn't seem useful
         if(d->character->pcdata)
           strncpy( d->character->pcdata->pwd, tmp_ch->pcdata->pwd, PASSWORD_LEN );
      }
      else {
         free_char( d->character );
         d->character            = tmp_ch;
         tmp_ch->desc            = d;
         tmp_ch->timer  = 0;
         send_to_char( "Reconnecting.\n\r", tmp_ch );
         sprintf(log_buf, "%s@%s has reconnected.",
            GET_NAME(tmp_ch), d->host );
         act( "$n has reconnected and is ready to kick ass.", tmp_ch, 0,
            0, TO_ROOM, INVIS_NULL);
         if(GET_LEVEL(tmp_ch) < ANGEL) {
            WAIT_STATE(tmp_ch, PULSE_VIOLENCE * 4);
            send_to_char("Heya sport, how about some RECONNECT LAG?\n\r", tmp_ch); 
            log( log_buf, SERAPH, LOG_SOCKET );
         } 
         else
            log( log_buf, GET_LEVEL(tmp_ch), LOG_SOCKET );
         
         STATE(d)            = CON_PLAYING;
      }
      return TRUE;
   }
   return FALSE;
}


/*
* Check if already playing (on an open descriptor.)
*/
bool check_playing(struct descriptor_data *d, char *name)
{
   struct descriptor_data *dold, *next_d;
   CHAR_DATA *compare = 0;
   
   for(dold = descriptor_list; dold; dold = next_d) {
      next_d = dold->next;
      
      if((dold == d) || (dold->character == 0))
         continue;
      
      compare = ((dold->original != 0) ? dold->original : dold->character);
      
      // If this is the case we fail our precondition to str_cmp 
      if(name == 0) continue;
      if(GET_NAME(compare) == 0)
         continue;
      
      if(str_cmp(name, GET_NAME(compare)))
         continue;
      
      if(STATE(dold) == CON_GET_NAME)
         continue;
      
      if(STATE(dold) == CON_GET_OLD_PASSWORD) {
         free_char(dold->character);
         dold->character = 0;
         close_socket(dold);
         continue;
      }
      close_socket(dold);
      return 0;
   }
   return 0;
}


char *str_str(char *first, char *second)
{
   char *pstr;
   
   for(pstr = first; *pstr; pstr++)
      *pstr = LOWER(*pstr);
   
   for(pstr = second; *pstr; pstr++)
      *pstr = LOWER(*pstr);
   
   pstr = strstr(first, second);
   
   return pstr;
}

// these are for my special lag that only keeps you from doing certain
// commands, while still allowing other.  (shooting arrows for example) 
void add_command_lag(CHAR_DATA * ch, int amount)
{
   if(GET_LEVEL(ch) < IMMORTAL)
      ch->timer += amount;
}

int check_command_lag(CHAR_DATA * ch)
{
   return ch->timer;
}

void remove_command_lag(CHAR_DATA * ch)
{
   ch->timer = 0;
}

void update_command_lag_and_poison()
{
   CHAR_DATA *i, *next_dude;
   int tmp, retval;
   
   for(i = character_list; i; i = next_dude) 
   {
      next_dude = i->next;

      // handle poison
      if(IS_AFFECTED(i, AFF_POISON)) {
        tmp = GET_POISON_AMOUNT(i);
        if(tmp) {
           retval = damage(i, i, tmp, TYPE_POISON, SPELL_POISON, 0);
           if(SOMEONE_DIED(retval))
             continue;
        }
      }

      // handle command lag
      if(i->timer > 0)
      {
        if(GET_LEVEL(i) < IMMORTAL)
          i->timer--;
        else i->timer = 0;
      }
   }
}
