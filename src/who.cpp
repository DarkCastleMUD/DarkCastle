/************************************************************************
| $Id: who.cpp,v 1.60 2014/07/04 22:00:04 jhhudso Exp $
| who.C
| Commands for who, maybe? :P
*/
#include <cstring>

#include "connect.h"
#include "utility.h"
#include "character.h"
#include "mobile.h"
#include "terminal.h"
#include "player.h"
#include "levels.h"
#include "clan.h"
#include "room.h"
#include "interp.h"
#include "handler.h"
#include "db.h"
#include "returnvals.h"
#include "const.h"

// TODO - Figure out the weird bug for why when I do "who <class>" a random player
//        from another class will pop up who name is nowhere near matching.

clan_data * get_clan(struct char_data *);

//#define GWHOBUFFERSIZE   (MAX_STRING_LENGTH*2)
//char gWhoBuffer[GWHOBUFFERSIZE];

// We now use a allocated pointer for the who buffer stuff.  It stays allocated, so
// we're not repeatedly allocing it, and it grows as needed to fit all the data (like a CString)
// That way we never have to worry about having a bunch of players on, and overflowing it.
// -pir 2/20/01
char * gWhoBuffer = NULL;
long gWhoBufferCurSize = 0;
long gWhoBufferMaxSize = 0;
 
void add_to_who(char * strAdd)
{
   long strLength = 0;

   if(!strAdd)                                    return;
   if(!(strLength = strlen(strAdd)))              return;

   if((strLength + gWhoBufferCurSize) >= gWhoBufferMaxSize)  { // expand the buffer
     gWhoBufferMaxSize += (strLength + 500);                   // expand by the size + 500
#ifdef LEAK_CHECK
     gWhoBuffer = (char *) realloc(gWhoBuffer, gWhoBufferMaxSize);
     if(!gWhoBuffer) {
       fprintf(stderr, "Unable to realloc in who.C add_to_who");
       abort();
     }
#else       
     gWhoBuffer = (char *) dc_realloc(gWhoBuffer, gWhoBufferMaxSize);
#endif
   }

   // guaranteed to work, since we just allocated enough for it + 500
   strcat(gWhoBuffer, strAdd);
   gWhoBufferCurSize += strLength; // update current data size
}

void clear_who_buffer()
{
  if(gWhoBuffer)
    *gWhoBuffer = '\0';     // kill the string
  gWhoBufferCurSize = 0;  // update the size
}

int do_whogroup(struct char_data *ch, char *argument, int cmd)
{

   descriptor_data *d;
   char_data *k, *i;
   follow_type *f;
   char target[MAX_INPUT_LENGTH];
   char tempbuffer[800];
   int foundtarget = 0;
   int foundgroup = 0;
   int hasholylight;

   one_argument(argument, target);
      
   hasholylight = IS_MOB(ch) ? 0 : ch->pcdata->holyLite;

   send_to_char(
     "$B$7($4:$7)=======================================================================($4:$7)\n\r"
     "$7|$5/$7|                     $5Current Grouped Adventurers                       $7|$5/$7|\n\r"
     "$7($4:$7)=======================================================================($4:$7)$R\n\r", ch);

   if(*target) {
     sprintf(gWhoBuffer, "Searching for '$B%s$R'...\r\n", target);
     send_to_char(gWhoBuffer, ch);
   }
     
   clear_who_buffer();

   for( d = descriptor_list; d; d = d->next ) {
      foundtarget = 0;

      if ((d->connected) || (!CAN_SEE(ch, d->character)))
         continue;

//  What the hell is this line supposed to be checking? -pir
//  If this occurs, we got alot bigger problems than 'who_group'
//      if (ch->desc->character != ch)
//         continue;
          
      i = d->character;

      // If I'm the leader of my group, process it
      if ((!i->master) && (IS_AFFECTED(i, AFF_GROUP))) 
      {
         foundgroup = 1;  // we found someone!
         k = i;
         sprintf(tempbuffer, "\n\r"
                             "   $B$7[$4: $5%s $4:$7]$R\n\r"
                             "   Number of PKs: %-3d  Average level of victim: %d\n\r", 
                             k->group_name,
                             IS_MOB(k) ? 0 : k->pcdata->group_kills, 
                             IS_MOB(k) ? 0 : (k->pcdata->group_kills ?  
                                               (k->pcdata->grplvl/k->pcdata->group_kills) : 0));
         add_to_who(tempbuffer);

         // If we're searching, see if this is the target
         if(is_abbrev(target, GET_NAME(i)))
            foundtarget = 1;

         // First, if they're not anonymous
         if ((!IS_MOB(ch) && hasholylight) || (!IS_ANONYMOUS(k) || (k->clan == ch->clan && ch->clan))) 
         {
             sprintf(tempbuffer,
                 "   $B%-18s %-10s %-14s   Level %2d      $1($7Leader$1)$R \n\r",
                 GET_NAME(k), race_info[(int)GET_RACE(k)].singular_name,
	         pc_clss_types[(int)GET_CLASS(k)], GET_LEVEL(k));
         }
         else {
             sprintf(tempbuffer,
                 "   $B%-18s %-10s Anonymous                      $1($7Leader$1)$R \n\r", 
                 GET_NAME(k), race_info[(int)GET_RACE(k)].singular_name);
         }
         add_to_who(tempbuffer);

         // loop through my followers and process them
         for(f = k->followers; f; f = f->next) {
            if (!IS_NPC(f->follower))
               if (IS_AFFECTED(f->follower, AFF_GROUP)) {
                  // If we're searching, see if this is the target
                  if(is_abbrev(target, GET_NAME(f->follower)))
                     foundtarget = 1;
                  // First if they're not anonymous
                  if (!IS_ANONYMOUS(f->follower) || (f->follower->clan == ch->clan && ch->clan))
                     sprintf(tempbuffer, "   %-18s %-10s %-14s   Level %2d\n\r",
                          GET_NAME(f->follower), race_info[(int)GET_RACE(f->follower)].singular_name,
                          pc_clss_types[(int)GET_CLASS(f->follower)], GET_LEVEL(f->follower));
                  else 
                     sprintf(tempbuffer,
                          "   %-18s %-10s Anonymous            \n\r",
                          GET_NAME(f->follower), race_info[(int)GET_RACE(f->follower)].singular_name);
                  add_to_who(tempbuffer);
               }
         } // for f = k->followers
      } //  ((!i->master) && (IS_AFFECTED(i, AFF_GROUP)) )

      // if we're searching (target exists) and we didn't find it, clear out 
      // the buffer cause we only want the target's group.  
      // If we found it, send it out, clear the buffer, and keep going in case someone else
      // matches the same target pattern.   ('whog a' gets Anarchy and Alpha's groups)
      // -pir
      if(*target && !foundtarget)
      {
         clear_who_buffer();
         foundgroup = 0; 
      }
      else if(*target && foundtarget)
      {
         send_to_char(gWhoBuffer, ch);
         clear_who_buffer();
      }
   }    // End for(d).

   if(0 == foundgroup) 
     add_to_who("\n\rNo groups found.\n\r");

   // page it to the player.  the 1 tells page_string to make it's own copy of the data
   page_string(ch->desc, gWhoBuffer, 1);
   return eSUCCESS;
}

int do_whosolo(struct char_data *ch, char *argument, int cmd)
{
   descriptor_data *d;
   char_data *i;
   char tempbuffer[800];
   char buf[MAX_INPUT_LENGTH+1];
   bool foundtarget;

   one_argument(argument, buf);

   send_to_char(
    "$B$7($4:$7)=======================================================================($4:$7)\n\r"
    "$7|$5/$7|                      $5Current SOLO Adventurers                         $7|$5/$7|\n\r"
    "$7($4:$7)=======================================================================($4:$7)$R\n\r"
    "   $BName            Race      Class        Level  PKs Deaths Avg-vict-level$R\n\r", ch);

   clear_who_buffer();

   for(d = descriptor_list; d; d = d->next) {
      foundtarget = FALSE;

      if ((d->connected) || !(i = d->character) || (!CAN_SEE(ch, i)))
         continue;

      if (is_abbrev(buf, GET_NAME(i)))
         foundtarget = TRUE;

      if (*buf && !foundtarget)
         continue;

      if (GET_LEVEL(i) <= MORTAL)
         if (!IS_AFFECTED(i, AFF_GROUP)) {
            if (!IS_ANONYMOUS(i) || (i->clan && i->clan == ch->clan))
               sprintf(tempbuffer,
                 "   %-15s %-9s %-13s %2d     %-4d%-7d%d\n\r",
                 i->name,
                 race_info[(int)GET_RACE(i)].singular_name,
                 pc_clss_types[(int)GET_CLASS(i)], GET_LEVEL(i),
                 IS_MOB(i) ? 0 : i->pcdata->totalpkills, 
                 IS_MOB(i) ? 0 : i->pcdata->pdeathslogin, 
                 IS_MOB(i) ? 0 : (i->pcdata->totalpkills ? 
                                  (i->pcdata->totalpkillslv/i->pcdata->totalpkills) : 0)
                 );
            else
               sprintf(tempbuffer,
                "   %-15s %-9s Anonymous            %-4d%-7d%d\n\r",
	        i->name, 
                race_info[(int)GET_RACE(i)].singular_name,
                 IS_MOB(i) ? 0 : i->pcdata->totalpkills, 
                 IS_MOB(i) ? 0 : i->pcdata->pdeathslogin, 
                 IS_MOB(i) ? 0 : (i->pcdata->totalpkills ? 
                                  (i->pcdata->totalpkillslv/i->pcdata->totalpkills) : 0)
	        );
            add_to_who(tempbuffer);
         } // if is affected by group
   } // End For Loop.

   // page it to the player.  the 1 tells page_string to make it's own copy of the data
   page_string(ch->desc, gWhoBuffer, 1);
   return eSUCCESS;
}

int do_who(struct char_data *ch, char *argument, int cmd)
{
    struct descriptor_data *d;
    struct char_data *i;
    clan_data * clan;
    int   numPC = 0;
    int   numImmort = 0;
    char* infoField;
    char  infoBuf[64];
    char  extraBuf[128];
    char  tailBuf[64];
    char  preBuf[64];
    char  arg[MAX_INPUT_LENGTH];
    char  oneword[MAX_INPUT_LENGTH];
    char  buf[MAX_STRING_LENGTH];
    char  immbuf[MAX_STRING_LENGTH];
    int   charmatch = 0;
    char  charname[MAX_INPUT_LENGTH];
    int   charmatchistrue = 0;
    int   clss = 0;
    int   levelarg = 0;
    int   lowlevel = 0;
    int   highlevel = 110;
    int   anoncheck = 0;
    int   sexcheck = 0;
    int   sextype = 0;
    int   nomatch = 0;
    int   hasholylight = 0;
    int   lfgcheck = 0;
    int   guidecheck = 0;
    int   race = 0;
    bool  addimmbuf;
    charname[0] = '\0';
    immbuf[0] = '\0';
    char* immortFields[] = {
      "   Immortal  ",
      "  Architect  ",
      "    Deity    ",
      "   Overseer  ",
      "   Divinity  ",
      "   --------  ",
      " Coordinator ",
      "   --------  ",
      " Implementor "
    };
    char *clss_types[] = {
      "mage",
      "cleric",
      "thief",
      "warrior",
      "antipaladin",
      "paladin",
      "barbarian",
      "monk",
      "ranger",
      "bard",
      "druid",
      "psionicist",
      "\n"
    };
    char *lowercase_race_types[] = {
      "human",
      "elf",
      "dwarf",
      "hobbit",
      "pixie",
      "ogre",
      "gnome",
      "orc",
      "troll",
      "\n"
    };
    
    hasholylight = IS_MOB(ch) ? 0 : ch->pcdata->holyLite;

    //  Loop through all our arguments
    for (half_chop(argument, oneword, arg);  strlen(oneword);  half_chop(arg, oneword, arg)) {
      if ((levelarg = atoi(oneword))) {
	if (!lowlevel) {
	  lowlevel = levelarg;
	} else if (lowlevel > levelarg) {
	  highlevel = lowlevel;
	  lowlevel = levelarg;
	} else {
	  highlevel = levelarg;
	}
	continue;        
      }

      // note that for all these, we don't 'continue' cause we want
      // to check for a name match at the end in case some annoying mortal
      // named himself "Anonymous" or "Penis", etc.        
      if (is_abbrev(oneword, "anonymous")) {
	anoncheck = 1;
      } else if (is_abbrev(oneword, "penis")) {
	sexcheck = 1;
	sextype = SEX_MALE;
      } else if (is_abbrev(oneword, "guide")) {
	guidecheck = 1;
      } else if (is_abbrev(oneword, "vagina")) {
	sexcheck = 1;
	sextype = SEX_FEMALE;
      } else if (is_abbrev(oneword, "other")) {
	sexcheck = 1;
	sextype = SEX_NEUTRAL;
      } else if (is_abbrev(oneword, "lfg")) {
	lfgcheck = 1;
      } else {
	for (clss = 0; clss <= 12; clss++)  {
	  if (clss == 12) {
	    clss = 0;
	    break;
	  } else if (is_abbrev(oneword, clss_types[clss])) {
	    clss++;
	    break;
	  }
	}

	for (race = 0; race <= 9; race++)  {
	  if (race == 9) {
	    race = 0;
	    break;
	  } else if (is_abbrev(oneword, lowercase_race_types[race])) {
	    race++;
	    break;
	  }
	}
      }

      // if there's anything left, we'll assume it's a partial name.
      // and we only take the "last" one in the list, so 'who warrior thief' only matches thief
      // This is consistent with how the class stuff works too
      strcpy(charname, oneword);
      charmatch = 1;

    } // end of for loop
    
    // Display the actual stuff
    send_to_char("[$4:$R]===================================[$4:$R]\n\r"
		 "|$5/$R|      $BDenizens of Dark Castle$R      |$5/$R|\n\r"
		 "[$4:$R]===================================[$4:$R]\n\r\n\r", ch);
    
    clear_who_buffer();
    
    for (d = descriptor_list; d; d = d->next) {
      // we have an invalid match arg, so nothing is going to match
      if (nomatch) {
	break;
      }
        
      if ((d->connected) && (d->connected) != conn::WRITE_BOARD && (d->connected) != conn::EDITING
	  && (d->connected) != conn::EDIT_MPROG) {
	continue;
      }
        
      if (d->original) {
	i = d->original;
      } else {
	i = d->character;
      }
        
      if (IS_NPC(i)) {
	continue;
      }
      if (!CAN_SEE(ch, i)) {
	continue;
      }
      // Level checks.  These happen no matter what
      if (GET_LEVEL(i) < lowlevel) {
	continue;
      }

      if (GET_LEVEL(i) > highlevel) {
	continue;
      }

      if (clss && !hasholylight && (!i->clan || i->clan != ch->clan) && IS_ANONYMOUS(i) && GET_LEVEL(i) < MIN_GOD) {
	continue;
      }
      if (lowlevel > 0 && IS_ANONYMOUS(i) && !hasholylight) {
	continue;
      }

      // Skip string based checks if our name matches
      if (!charmatch || !is_abbrev(charname, GET_NAME(i))) {
	if (clss && GET_CLASS(i) != clss && !charmatchistrue) {
	  continue;
	}
	if (anoncheck && !IS_ANONYMOUS(i) && !charmatchistrue) {
	  continue;
	}
	if (sexcheck && ( GET_SEX(i) != sextype || ( IS_ANONYMOUS(i) && !hasholylight ) ) && !charmatchistrue) {
	  continue;
	}
	if (lfgcheck && !IS_SET(i->pcdata->toggles, PLR_LFG)) {
	  continue;
	}
	if (guidecheck && !IS_SET(i->pcdata->toggles, PLR_GUIDE_TOG)) {
	  continue;
	}
	if (race && GET_RACE(i) != race && !charmatchistrue) {
	  continue;
	}
      }

      // At this point, we either pass a filter or our name matches.
      if ( ! ( clss || anoncheck || sexcheck || lfgcheck ||guidecheck || race ) ) {
	// If there were no filters, the it's only a name match so filter out
	// anyone that doesn't match the filters
	if (charmatch && !is_abbrev(charname, GET_NAME(i))) {
	  continue;
	}
      }
        
      infoField = infoBuf;
      extraBuf[0] = '\0';
      buf[0]      = '\0';
      addimmbuf = FALSE;
      if (GET_LEVEL(i) > MORTAL) {
	/* Immortals can't be anonymous */
	if (!str_cmp(GET_NAME(i), "Urizen")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, "   Meatball  ");
	} else if (!str_cmp(GET_NAME(i), "Julian")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, "    $B$7S$4a$7l$4m$7o$4n$R   ");
	} else if (!strcmp(GET_NAME(i), "Apocalypse")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, "    $5Moose$R    ");
	} else if (!strcmp(GET_NAME(i),"Wendy")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, " $B$2Ital$7ian $4Chef$R");
	} else if (!strcmp(GET_NAME(i), "Pirahna")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, "   $B$4>$5<$1($2($1($5:$4>$R   ");
	} else if (!strcmp(GET_NAME(i), "Wynn")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, "  $1$B//\\$4o.o$1/\\\\$R  ");
	} else if (!strcmp(GET_NAME(i), "Scyld")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, "    $B$4H$5i$2p$3p$1i$6e$R   ");
	} else if (!strcmp(GET_NAME(i), "Elder")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, " $B$5Fear$0The$5Beard$R");
	} else if (!strcmp(GET_NAME(i), "Sergio")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, " $4Evil Genius$R");
	} else if (!strcmp(GET_NAME(i), "Petra")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, "    $B$1R$2o$1a$2d$1i$2e$R   ");
	} else if (!strcmp(GET_NAME(i), "Zen")) {
	  infoField = infoBuf;
	  sprintf(infoBuf, "  $2$BLead $5Dingo$R ");  
	} else {
	  infoField = immortFields[GET_LEVEL(i) - IMMORTAL];
	}
	
	if (GET_LEVEL(ch) >= IMMORTAL && !IS_MOB(i) && i->pcdata->wizinvis > 0) {
	  if(!IS_MOB(i) && i->pcdata->incognito == TRUE) {
	    sprintf(extraBuf," (Incognito / WizInvis %ld)", i->pcdata->wizinvis);
	  } else {
	    sprintf(extraBuf," (WizInvis %ld)", i->pcdata->wizinvis);
	  }
	}
	numImmort++;
	addimmbuf = TRUE;
      } else {
	if (!IS_ANONYMOUS(i) || (ch->clan && ch->clan == i->clan) || hasholylight) {
	  sprintf(infoBuf, " $B$5%2d$7-$1%s  $2%s$R$7 ",
		  GET_LEVEL(i), pc_clss_abbrev[(int)GET_CLASS(i)], race_abbrev[(int)GET_RACE(i)]);
	} else {
	  sprintf(infoBuf, "  $6-==-   $B$2%s$R ", race_abbrev[(int)GET_RACE(i)]);
	}
	numPC++;
      }
      
      if ((d->connected) == conn::WRITE_BOARD || (d->connected) == conn::EDITING ||
	  (d->connected) == conn::EDIT_MPROG) {
	strcpy(tailBuf, "$1$B(writing) ");
      } else {
	*tailBuf = '\0'; // clear it
      }
      
      if (IS_SET(i->pcdata->toggles, PLR_GUIDE_TOG)) {
	strcpy(preBuf, "$7$B(Guide)$R ");
      } else {
	*preBuf = '\0';
      }
      
      if (IS_SET(i->pcdata->toggles, PLR_LFG)) {
	strcat(tailBuf, "$3(LFG) ");
      }
      
      if (IS_AFFECTED(i, AFF_CHAMPION)) {
	strcat(tailBuf, "$B$4(Champion)$R");
      }
      
      if (i->clan && (clan = get_clan(i)) && GET_LEVEL(i) < OVERSEER) {
	sprintf(buf,"[%s] %s$3%s %s %s $2[%s$R$2] %s$R\n\r",
		infoField,   preBuf,   GET_SHORT(i),   i->title,
		extraBuf,    clan->name,     tailBuf);
      } else {
	sprintf(buf,"[%s] %s$3%s %s$R$3 %s %s$R\n\r",
		infoField,   preBuf, GET_SHORT(i),   i->title,
		extraBuf,    tailBuf);
      }
      
      if (addimmbuf) {
	strcat(immbuf, buf);
      } else {
	add_to_who(buf);
      }
    }
    
    if (numPC && numImmort) {
      add_to_who("\n\r");
    }
    
    if (numImmort) {
      add_to_who(immbuf);
    }
    
    if ((numPC + numImmort) > max_who) {
      max_who = numPC + numImmort;
    }
    
    sprintf(buf, "\n\r"
	    "    Visible Players Connected:   %d\n\r"
	    "    Visible Immortals Connected: %d\n\r"
	    "    (Max this boot is %d)\n\r",
	    numPC, numImmort, max_who);
    
    add_to_who(buf);
    
    // page it to the player.  the 1 tells page_string to make it's own copy of the data
    page_string(ch->desc, gWhoBuffer, 1);
    
    return eSUCCESS;
}

int do_whoarena(struct char_data *ch, char *argument, int cmd)
{
   int count = 0;
   clan_data * clan;

   send_to_char("\n\rPlayers in the Arena:\n\r--------------------------\n\r", ch);

   if (GET_LEVEL(ch) <= MORTAL) 
   {
		auto &character_list = DC::instance().character_list;
		for (auto& tmp : character_list) {
         if (CAN_SEE(ch, tmp))
         {
            if (IS_SET(world[tmp->in_room].room_flags, ARENA) 
                && !IS_SET(world[tmp->in_room].room_flags, NO_WHERE)) 
            {
               if((tmp->clan) && (clan = get_clan(tmp)) && GET_LEVEL(tmp) < IMMORTAL)
                 csendf(ch, "%-20s - [%s$R]\n\r", GET_NAME(tmp), clan->name);
               else csendf(ch, "%-20s\n\r", GET_NAME(tmp));
               count++;
            }
         }
      }
	 
      if (count == 0)
	csendf(ch, "\n\rThere are no visible players in the arena.\n\r");
      
    return eSUCCESS;
   }
      
   // If they're here that means they're a god
	auto &character_list = DC::instance().character_list;
	for (auto& tmp : character_list) {
      if (CAN_SEE(ch, tmp)) {
         if (IS_SET(world[tmp->in_room].room_flags, ARENA)) {
            if((tmp->clan) && (clan = get_clan(tmp)) && GET_LEVEL(tmp) < IMMORTAL)
               csendf(ch, "%-20s  Level: %-3d  Hit: %-5d  Room: %-5d - [%s$R]\n\r",
	           GET_NAME(tmp),
                   GET_LEVEL(tmp), GET_HIT(tmp), tmp->in_room, clan->name);
            else csendf(ch, "%-20s  Level: %-3d  Hit: %-5d  Room: %-5d\n\r",
	           GET_NAME(tmp),
                   GET_LEVEL(tmp), GET_HIT(tmp), tmp->in_room);
            count++;
            }
         }
      }

   if (count == 0)
      csendf(ch, "\n\rThere are no visible players in the arena.\n\r");
   return eSUCCESS;
}

int do_where(struct char_data *ch, char *argument, int cmd)
{
  struct descriptor_data *d;
  int zonenumber;
  char buf[MAX_INPUT_LENGTH];

  one_argument(argument, buf);

  if (GET_LEVEL(ch) >= IMMORTAL && *buf && !strcmp(buf, "all")) { //  immortal noly, shows all
    send_to_char("All Players:\n\r--------\n\r", ch);
    for (d = descriptor_list; d; d = d->next) {
      if (d->character && (d->connected == conn::PLAYING) && (CAN_SEE(ch, d->character)) && (d->character->in_room != NOWHERE)) {
        if (d->original) {  // If switched
          csendf(ch, "%-20s - %s$R [%d] In body of %s\n\r", d->original->name, world[d->character->in_room].name,
                 world[d->character->in_room].number, fname(d->character->name));
        } else {
          csendf(ch, "%-20s - %s$R [%d]\n\r", 
                      d->character->name, world[d->character->in_room].name, world[d->character->in_room].number);
        }
      }
    } // for
  } else if (GET_LEVEL(ch) >= IMMORTAL && *buf) { // immortal only, shows ONE person
    send_to_char("Search of Players:\n\r--------\n\r", ch);
    for (d = descriptor_list; d; d = d->next) {
      if (d->character && (d->connected == conn::PLAYING) && (CAN_SEE(ch, d->character)) && (d->character->in_room != NOWHERE)) {
        if (d->original) {  // If switched
          if (is_abbrev(buf, d->original->name)) {
            csendf(ch, "%-20s - %s$R [%d] In body of %s\n\r", d->original->name, world[d->character->in_room].name,
                   world[d->character->in_room].number, fname(d->character->name));
          }
        } else {
          if (is_abbrev(buf, d->character->name)) {
            csendf(ch, "%-20s - %s$R [%d]\n\r", 
                        d->character->name, world[d->character->in_room].name, world[d->character->in_room].number);
          }
        }
      }
    } // for
  } else {  // normal, mortal where
    zonenumber = world[ch->in_room].zone;
    send_to_char("Players in your vicinity:\n\r-------------------------\n\r", ch);
    if (IS_SET(world[ch->in_room].room_flags, NO_WHERE))
       return eFAILURE;
    for (d = descriptor_list; d; d = d->next) {
      if (d->character && (d->connected == conn::PLAYING) && (d->character->in_room != NOWHERE) &&
          !IS_SET(world[d->character->in_room].room_flags, NO_WHERE) &&
 	  CAN_SEE(ch, d->character) && !IS_MOB(d->character) /*Don't show snooped mobs*/)  {
          if (world[d->character->in_room].zone == zonenumber)
            csendf(ch, "%-20s - %s$R\n\r", d->character->name,
          world[d->character->in_room].name);
       }
    }
  }

  return eSUCCESS;
}
