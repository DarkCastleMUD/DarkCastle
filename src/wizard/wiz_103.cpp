/********************************
| Level 103 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"

#include <utility.h>
#include <mobile.h>
#include <player.h>
#include <db.h>
#include <connect.h>
#include <interp.h>
#include <room.h>
#include <handler.h>
#include <returnvals.h>
#include <spells.h>
#include <clan.h>

int do_boot(struct char_data *ch, char *arg, int cmd)
{
  struct char_data *victim;
  int room;
  char name[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], buf[500];

  half_chop(arg,name,type);

  if(!(*name)) {
    send_to_char("Boot who?\n\r", ch);
    return eFAILURE;
  }
  
  victim = get_pc_vis(ch, name);

  if(victim) {
    if (!IS_NPC(victim) && (GET_LEVEL(ch) <= GET_LEVEL(victim))) {
      act("You cast a stream of fire at $N.", ch, 0, victim, TO_CHAR, 0);
      act("$n casts a stream of fire at you.", ch, 0, victim, TO_VICT, 0);
      act("$n casts a stream of fire at $N.", ch, 0, victim, TO_ROOM, NOTVICT);
      return eFAILURE;
    } 
    if (!IS_MOB(victim) && victim->pcdata->possesing) {
       send_to_char ("Oops! They ain't linkdead! Just possessing.", ch);
       return eFAILURE;
    }
    if(IS_AFFECTED(victim, AFF_CANTQUIT))
    {
      if(affected_by_spell(victim, FUCK_PTHIEF))
      {
        act("$N is a thief.  Don't boot $M.", ch, 0, victim, TO_CHAR, 0);
        return eFAILURE;
      }
      act("$N is a pkiller.  Don't boot $M.", ch, 0, victim, TO_CHAR, 0);
      return eFAILURE;
    }

    /* Still here? Ok, the boot continues */
    send_to_char("You have been disconnected.\r\n", victim);
    send_to_char("Ok.\n\r", ch);
    if (!IS_NPC(victim)) {
      sprintf(buf, "A stream of fire arcs down from the heavens, striking "
              "you between the eyes.\n\rYou have been removed from the "
              "world by %s.\n\r",
             GET_SHORT(ch));
      send_to_char(buf, victim);
    }

    room = victim->in_room;

    act("A stream of fire arcs down from the heavens, striking "
        "$n between the eyes.\r\n$n has been removed from the world "
        "by $N.", victim, 0, ch, TO_ROOM, INVIS_NULL);

    sprintf(name,"%s has booted %s.", GET_NAME(ch), GET_NAME(victim));
    log(name, GET_LEVEL(ch), LOG_GOD);

    if(!strcmp(type, "boot")) {
       send_to_char(
"\n\r"
"                       $1/               /\n\r"
"                      $1/               /\n\r"
"                     $1/               /\n\r"
"                    $1/               /$R\n\r"
"                   $5/\\_             $1/$R\n\r"
"      $5____        /   \\_          $1/$R\n\r"
"     $5/    \\      /\\     \\_       $1/$R\n\r"
"    $5/      \\    /\\        \\_    $1/$R\n\r"
"   $5/        \\  /\\           \\_ $1/$R\n\r"
"  $5/          \\/\\              /\n\r"
"  |\\                         /\n\r"
"   \\\\                       /\n\r"
"    \\\\                     /\n\r"
"     \\\\                   /\n\r"
"      \\\\                 /\n\r"
"       \\\\               /\n\r"
"        \\\\             /\n\r"
"         \\\\           /\n\r"
"          \\\\         /\n\r"
"           \\\\       /\n\r"
"            \\\\     /\n\r"
"             \\\\   /\n\r"
"              \\__/$R\n\r", victim);

}
    move_char(victim, real_room(3001));
    do_quit(victim, "", 666);
  }

  else 
     send_to_char("Boot Who?\n\r", ch); 

  return eSUCCESS;
}
    
int do_disconnect(struct char_data *ch,  char *argument, int cmd)
{
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *d;
    unsigned sdesc;

    if (IS_NPC(ch))
            return eFAILURE;


    one_argument(argument, arg);
    sdesc=atoi(arg);
    if(arg==0){
            send_to_char("Illegal descriptor number.\n\r",ch);
            send_to_char("Usage: release <#>\n\r",ch);
            return eFAILURE;
      }
    for(d=descriptor_list;d;d=d->next)
    {
        if (d->descriptor==sdesc)
        {
            if (d->character && (GET_LEVEL(d->character) > GET_LEVEL(ch))) 
            {
                sprintf(buf, "Heh, %s tried to disconnect you. He has paid.\n\r", GET_NAME(ch));
                send_to_char(buf, d->character);
                send_to_char("You dummy, can't do that to your elders!\n\r", ch);
                close_socket(ch->desc);
                return eFAILURE;
            }
            else {
                close_socket(d);
                sprintf(buf,"Closing socket to descriptor #%d\n\r",sdesc);
                send_to_char(buf,ch);
                return eFAILURE;
            }
        }
    }
    send_to_char("Descriptor not found!\n\r",ch);
    return eSUCCESS;
}

int do_fsave(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict;
  char name[100], buf[400]; 

  if(IS_NPC(ch))
    return eFAILURE;

  one_argument(argument, name);

  if(!*name)
    send_to_char("Who do you wish to force to save?\n\r", ch);

  if(!(vict = get_char_vis(ch, name)))
    send_to_char("No-one by that name here..\n\r", ch);

  else {
    /*if((GET_LEVEL(ch) <= GET_LEVEL(vict)) && !IS_NPC(vict)) {
      send_to_char("Hahaha\n\r", ch);
      sprintf(buf, "$n has failed to force you to save.");
      act(buf,  ch, 0, vict, TO_VICT, 0);
    }
    else */{
      if (ch->pcdata->stealth == FALSE) {
        sprintf(buf, "$n has forced you to 'save'.");
        act(buf,  ch, 0, vict, TO_VICT, 0);
        strcpy(buf,"");
        send_to_char("Ok.\n\r", ch);
      }
      do_save(vict, "", 9);
      sprintf(buf,"%s just forced %s to save.", GET_NAME(ch),
              GET_NAME(vict));
      log(buf, GET_LEVEL(ch), LOG_GOD);
    }
  }
  return eSUCCESS;
}

int do_fighting(struct char_data *ch, char *argument, int cmd)
{
  const int CLANTAG_LEN = MAX_CLAN_LEN+3; // "[Foobar]"
  struct char_data *i;
  bool arenaONLY = false;
  int countFighters = 0;
  char buf[80];
  char arg[MAX_STRING_LENGTH];
  struct clan_data *ch_clan = 0;
  char ch_clan_name[CLANTAG_LEN];
  ch_clan_name[0] = 0;
  struct clan_data *victim_clan = 0;
  char victim_clan_name[CLANTAG_LEN];
  victim_clan_name[0] = 0;
  
  one_argument(argument, arg);
  if (!strcmp(arg, "arena"))
    arenaONLY = true;

  for(i = combat_list; i; i = i->next_fighting) {
    // Don't show mobs fighting or people not in the arena when arena
    // keyword was specified.
    if(IS_NPC(i)
       || (arenaONLY && !IS_SET(world[i->in_room].room_flags, ARENA))) {
      continue;
    } else {
      countFighters++;
    }
  
    // If they're in a clan
    if ((ch_clan = get_clan(i)))
      snprintf(ch_clan_name, CLANTAG_LEN, "[%s]", ch_clan->name);    

    if((victim_clan = get_clan(i->fighting)))
      snprintf(victim_clan_name, CLANTAG_LEN, "[%s]", victim_clan->name);

    snprintf(buf, 80, "%14s %-17s fighting %14s %-17s",
	    GET_NAME(i), ch_clan_name,
	    GET_NAME(i->fighting), victim_clan_name);
    send_to_char(buf, ch);
     
    snprintf(buf, 80, "\n\rin room: %s [%ld]\n\r",
	     (world[i->in_room].name),
	     (long)(world[i->in_room].number));
    send_to_char(buf, ch);
  }
  
  
  if(countFighters == 0) {
    if (arenaONLY)
      send_to_char("No fighting characters found in the arena.\n\r", ch);
    else
      send_to_char("No fighting characters found.\n\r", ch);
  }
  return eSUCCESS;
}

int do_peace( struct char_data *ch, char *argument, int cmd )
{
    struct char_data *rch;

    for (rch = world[ch->in_room].people; rch!=NULL; rch = rch->next_in_room) {
        if ( IS_MOB(rch) && rch->mobdata->hatred != NULL )
            remove_memory(rch, 'h');
        if ( rch->fighting != NULL )
            stop_fighting(rch);
    }
    act("$n makes a gesture and all fighting stops.", ch,0,0,TO_ROOM, 0);
    send_to_char( "You stop all fighting in this room.\n\r", ch );
    return eSUCCESS;
}


