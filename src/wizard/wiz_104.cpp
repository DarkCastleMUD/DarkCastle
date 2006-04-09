/********************************
| Level 104 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"

#include <utility.h>
#include <connect.h>
#include <mobile.h>
#include <player.h>
#include <levels.h>
#include <obj.h>
#include <handler.h>
#include <db.h>
#include <room.h>
#include <interp.h>
#include <returnvals.h>
#include <spells.h>
#include <race.h>

extern struct room_data ** world_array;
void save_corpses(void);
extern char *obj_types[];

int do_thunder(struct char_data *ch, char *argument, int cmd)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  char buf3[MAX_INPUT_LENGTH];

  if (!IS_NPC(ch) && ch->pcdata->wizinvis)
     sprintf(buf3, "someone");
  else
     sprintf(buf3, GET_SHORT(ch));

  for (; *argument == ' '; argument++);

  if(!(*argument))
    send_to_char("It's not gonna look that impressive...\n\r", ch);
  else {
    if (cmd == 9) 
      sprintf(buf2, "$4$BYou thunder '%s'$R", argument);
    else 
      sprintf(buf2, "$7$BYou bellow '%s'$R", argument);
    act(buf2, ch, 0, 0, TO_CHAR, 0);

    for (i = descriptor_list; i; i = i->next)
      if (i->character != ch && !i->connected) {
        if (!IS_NPC(ch) && ch->pcdata->wizinvis && i->character->level < ch->pcdata->wizinvis)
          sprintf(buf3, "Someone");
        else
           sprintf(buf3, GET_SHORT(ch));

        if (cmd == 9) {
           sprintf(buf1, "$B$4%s thunders '%s'$R\n\r",buf3, argument);
        } else {
           sprintf(buf1, "$7$B%s bellows '%s'$R\r\n", buf3, argument);
        }

        send_to_char(buf1, i->character);
     }
  } 
  return eSUCCESS; 
}

int do_incognito(struct char_data *ch, char *argument, int cmd)
{
  if(IS_MOB(ch))
    return eFAILURE;

  if(ch->pcdata->incognito == TRUE) {
    send_to_char("Incognito off.\n\r", ch);
    ch->pcdata->incognito = FALSE;
  }
  else {
    send_to_char("Incognito on.  Even while invis, anyone in your room can "
                 "see you.\n\r", ch);
    ch->pcdata->incognito = TRUE;
  }
  return eSUCCESS;
}

int do_load(struct char_data *ch, char *arg, int cmd)
{
  char type[MAX_INPUT_LENGTH];
  char name[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];

  char *c;
  int x, number =0, num = 0,cnt = 1;

  char *types[] = {
    "mobile",
    "object",
  };

  if(IS_NPC(ch))
    return eFAILURE;

  if(!has_skill(ch, COMMAND_LOAD)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }

  half_chop(arg, type, arg2);

  if(!*type || !*arg2) {
    send_to_char("Usage:  load <mob|obj> <name|vnum>\n\r", ch);
    return eFAILURE;
  }

  half_chop (arg2, name, arg3);

  if (arg3) cnt = atoi(arg3);

  if(cnt > 50)
  {
     send_to_char("Sorry, you can only load at most 50 of something at a time.\r\n", ch);
     return eFAILURE;
  }

  c = name;

  for(x = 0; x <= 2; x++) {
     if(x == 2) {
       send_to_char("Type must mobile or object.\n\r", ch);
       return eFAILURE;
     }
     if(is_abbrev(type, types[x]))
       break; 
  }

  switch(x) {
    default:
      send_to_char("Problem...fuck up in do_load.\n\r", ch);
      log("Default in do_load...should NOT happen.", ANGEL, LOG_BUG);
      return eFAILURE;
    case 0 :  /* mobile */
      if((number = number_or_name(&c, &num)) == 0)
        return eFAILURE;
      else if(number == -1) {
        if((number = real_mobile(num)) < 0) {
          send_to_char("No such mobile.\n\r", ch);
          return eFAILURE;
        }
        if(GET_LEVEL(ch) < DEITY && !can_modify_mobile(ch, num)) {
          send_to_char("You may only load mobs inside of your range.\n\r", ch);
          return eFAILURE;
        } 
        do_mload(ch, number, cnt);
        return eSUCCESS;
      }
      if((num = mob_in_index(c, number)) == -1) {
        send_to_char("No such mobile.\n\r", ch);
        return eFAILURE;
      }
      do_mload(ch, num, cnt);
      return eSUCCESS;
    case 1 :  /* object */ 
      if((number = number_or_name(&c, &num)) == 0)
        return eFAILURE;
      else if(number == -1) {
        if((number = real_object(num)) < 0) {
          send_to_char("No such object.\n\r", ch);
          return eFAILURE;
        }
        if((GET_LEVEL(ch) < 108) && 
           IS_SET(((struct obj_data *)(obj_index[number].item))->obj_flags.extra_flags, ITEM_SPECIAL)) 
        {
          send_to_char("Why would you want to load that?\n\r", ch);
          return eFAILURE;
        }
        if(GET_LEVEL(ch) < DEITY && !can_modify_object(ch, num)) {
          send_to_char("You may only load objects inside of your range.\n\r", ch);
          return eFAILURE;
        } 

        do_oload(ch, number, cnt);
        return eSUCCESS;
      }
      if((num = obj_in_index(c, number)) == -1) {
        send_to_char("No such object.\n\r", ch);
        return eFAILURE;
      }
      if((GET_LEVEL(ch) < IMP) && 
         IS_SET(((struct obj_data *)
         (obj_index[num].item))->obj_flags.extra_flags,
         ITEM_SPECIAL)) {
        send_to_char("Why would you want to load that?\n\r", ch);
        return eFAILURE;
      }
      do_oload(ch, num, cnt);
      return eSUCCESS;
  }
  return eSUCCESS;
}

int do_purge(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  char name[100], buf[300];

  if(IS_NPC(ch))
    return eFAILURE;

  one_argument(argument, name);

  if(*name) { /* argument supplied. destroy single object or char */
    if((vict = get_char_room_vis(ch, name)) && (GET_LEVEL(ch) > G_POWER)) 
{ 
      if(!IS_NPC(vict) && (GET_LEVEL(ch)<=GET_LEVEL(vict))) {
        sprintf(buf, "%s is surrounded with scorching flames but is"
                " unharmed.\n\r", GET_SHORT(vict)); 
        send_to_char(buf,ch);
        act("$n tried to purge you.", ch, 0, vict, TO_VICT, 0);
        return eFAILURE;
      }

      act("$n disintegrates $N.",  ch, 0, vict, TO_ROOM, NOTVICT);
      act("You disintegrate $N.",  ch, 0, vict, TO_CHAR, 0);

      if(vict->desc) { 
        close_socket( vict->desc);
        vict->desc = NULL;
      }

      extract_char(vict, TRUE);
    }
    else if((obj = get_obj_in_list_vis(ch, name,
            world[ch->in_room].contents)) != NULL) { 
      act("$n purges $p.",  ch, obj, 0, TO_ROOM, 0);
      act("You purge $p.",  ch, obj, 0, TO_CHAR, 0);
      extract_obj(obj);
    }
    else { 
      send_to_char( "You can't find it to purge!\n\r", ch );
      return eFAILURE;
    }
  }
  else {  /* no argument. clean out the room */
    if(IS_NPC(ch)) { 
      send_to_char("Don't... You would kill yourself too.\n\r", ch);
      return eFAILURE;
    }

    act("$n gestures... the room is filled with scorching flames!",
        ch, 0, 0, TO_ROOM, 0);
    send_to_char("You gesture...the room is filled with scorching "
                 "flames!\n\r", ch);

    for(vict = world[ch->in_room].people; vict; vict = next_v) { 
       next_v = vict->next_in_room;
       if(IS_NPC(vict))
         extract_char(vict, TRUE);
    }

    for(obj = world[ch->in_room].contents; obj; obj = next_o) { 
       next_o = obj->next_content;
       extract_obj(obj);
    }
  }
  save_corpses();
  return eSUCCESS;
}

int show_zone_commands(struct char_data *ch, int i, int start = 0)
{
  char buf[MAX_STRING_LENGTH];
  extern char * zone_bits[];
  extern char * zone_modes[];
  extern char *equipment_types[];
  extern int top_of_zonet;
  void sprintbit( long vektor, char *names[], char *result );
  int k = 0;
  int num_to_show;

  if(i > top_of_zonet) {
    send_to_char("There is no such zone.\r\n", ch);
    return eFAILURE;
  }

  sprintf(buf, "$3Name$R: %s\r\n"
               "$3Starts$R:   %6d $3Ends$R:  %13d\r\n"
               "$3Lifetime$R: %6d $3Age$R:   %13d     $3Left$R:   %6d\r\n" 
               "$3PC'sInZone$R: %4d $3Mode$R: %-18s $3Flags$R: ",
                    zone_table[i].name, 
                    (i ? (zone_table[i - 1].top + 1) : 0), 
                    zone_table[i].top,
                    zone_table[i].lifespan,
                    zone_table[i].age,  
                    zone_table[i].lifespan - zone_table[i].age,
                    zone_table[i].players,
                    zone_modes[zone_table[i].reset_mode]);
  send_to_char(buf, ch);
  sprintbit(zone_table[i].zone_flags, zone_bits, buf);
  send_to_char(buf, ch);
  sprintf(buf,"\r\n"
               "$3MobsLastPop$R: %3d $3DeathCounter$R: %6d     $3ReduceCounter$R: %d\r\n"
               "$3DiedThisTick$R: %d\r\n",
                    zone_table[i].num_mob_on_repop,
                    zone_table[i].death_counter,
                    zone_table[i].counter_mod,
                    zone_table[i].died_this_tick);
  send_to_char(buf, ch);
  send_to_char("\r\n", ch);

  if(zone_table[i].cmd[0].command == 'S')
  {
    send_to_char("This zone has no zone commands.\r\n", ch);
    return eFAILURE;
  }

  for(k = 0; zone_table[i].cmd[k].command != 'S'; k++);

  if(k < start)
  {
    sprintf(buf, "Last command in this zone is %d.\r\n", k);
    send_to_char(buf, ch);
    return eFAILURE;
  }

  if(IS_SET(ch->pcdata->toggles, PLR_PAGER))
     num_to_show = 50;
  else num_to_show = 20;

  // show zone cmds
  for(int j = start; (j < start+num_to_show) && (zone_table[i].cmd[j].command != 'S'); j++)
  {
    // show command # and if_flag
    // note that we show the command as cmd+1.  This is so we don't have a
    // command 0 from the user's perspective.
    if(zone_table[i].cmd[j].command == '*')
    {
      sprintf(buf, "[%3d] Comment: ", j+1);
    }
    else switch(zone_table[i].cmd[j].if_flag) {
      case 0: sprintf(buf, "[%3d] Always ", j+1); break;
      case 1: sprintf(buf, "[%3d] $B$2OnTrue$R ", j+1); break;
      case 2: sprintf(buf, "[%3d] $4OnFals$R ", j+1); break;
      case 3: sprintf(buf, "[%3d] $B$5OnBoot$R ", j+1); break;
      case 4: sprintf(buf, "[%3d] $B$2Ls$1Mb$2Tr$R ", j+1); break;
      case 5: sprintf(buf, "[%3d] $B$4Ls$1Mb$4Fl$R ", j+1); break;
      case 6: sprintf(buf, "[%3d] $B$2Ls$7Ob$2Tr$R ", j+1); break;
      case 7: sprintf(buf, "[%3d] $B$4Ls$7Ob$4Fl$R ", j+1); break;
      case 8: sprintf(buf, "[%3d] $B$2Ls$R%%%%$B$2Tr$R ", j+1); break;
      case 9: sprintf(buf, "[%3d] $B$4Ls$R%%%%$B$4Fl$R ", j+1); break;
      default: sprintf(buf, "[%3d] $B$4ERROR(%d)$R", j+1, zone_table[i].cmd[j].if_flag); break;
    }
   int virt;
    #define ZCMD zone_table[i].cmd[j]
    switch(zone_table[i].cmd[j].command) {
    case 'M':
      virt = ZCMD.active?mob_index[ZCMD.arg1].virt:ZCMD.arg1;
      sprintf(buf, "%s $B$1Load mob  [%5d] ", buf, virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) in room ");
      else sprintf(buf, "%s(if< [%3d]) in room ", buf, zone_table[i].cmd[j].arg2);
      sprintf(buf, "%s[%5d].$R\r\n", buf,zone_table[i].cmd[j].arg3);
      break;
    case 'O':
      virt = ZCMD.active?obj_index[ZCMD.arg1].virt:ZCMD.arg1;
      sprintf(buf, "%s $BLoad obj  [%5d] ", buf, virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) in room ");
      else sprintf(buf, "%s(if< [%3d]) in room ", buf, zone_table[i].cmd[j].arg2);
//      sprintf(buf, "%s[%5d].$R\r\n", buf, 
//world[zone_table[i].cmd[j].arg3].number);
      sprintf(buf, "%s[%5d].$R\r\n", buf,zone_table[i].cmd[j].arg3);
      break;
    case 'P':
      virt = ZCMD.active?obj_index[ZCMD.arg1].virt:ZCMD.arg1;
      sprintf(buf, "%s $5Place obj [%5d] ", buf, virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) in objt ");
      else sprintf(buf, "%s(if< [%3d]) in objt ", buf, zone_table[i].cmd[j].arg2);
      virt = ZCMD.active?obj_index[ZCMD.arg3].virt:ZCMD.arg3;
      sprintf(buf, "%s[%5d] (in last created).$R\r\n", buf, virt);
      break;
    case 'G':
      virt = ZCMD.active?obj_index[ZCMD.arg1].virt:ZCMD.arg1;
      sprintf(buf, "%s $6Place obj [%5d] ", buf, virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) on last mob loaded.$R\r\n");
      else sprintf(buf, "%s(if< [%3d]) on last mob loaded.$R\r\n", buf, zone_table[i].cmd[j].arg2);
      break;
    case 'E':
      virt = ZCMD.active?obj_index[ZCMD.arg1].virt:ZCMD.arg1;
      sprintf(buf, "%s $2Equip obj [%5d] ", buf, virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) on last mob on ");
      else sprintf(buf, "%s(if< [%3d]) on last mob on ", buf, zone_table[i].cmd[j].arg2);
      if(zone_table[i].cmd[j].arg3 > MAX_WEAR-1 ||
         zone_table[i].cmd[j].arg3 < 0)
         sprintf(buf, "%s[%d](InvalidArg3).$R\r\n", buf, zone_table[i].cmd[j].arg3);
      else sprintf(buf, "%s[%d](%s).$R\r\n", buf, zone_table[i].cmd[j].arg3,
           equipment_types[zone_table[i].cmd[j].arg3]);
      break;
    case 'D':
      sprintf(buf, "%s $3Room [%5d] Dir: [%d]", buf,
        zone_table[i].cmd[j].arg1,
        zone_table[i].cmd[j].arg2);

        switch(zone_table[i].cmd[j].arg3) {
        case 0:
          strcat(buf, "Unlock/Open$R\r\n");
          break;
        case 1:
          strcat(buf, "Unlock/Close$R\r\n");
          break;
        case 2:
          strcat(buf, "Lock/Close$R\r\n");
          break;
        default:
          strcat(buf, "ERROR: Unknown$R\r\n");
          break;
        }
      break;
    case '%':
      sprintf(buf, "%s Consider myself true on %d times out of %d.\r\n", buf,
        zone_table[i].cmd[j].arg1,
        zone_table[i].cmd[j].arg2);
      break;
    case 'J':
      sprintf(buf, "%s Temp Command. [%d] [%d] [%d]\r\n", buf,
                      zone_table[i].cmd[j].arg1,
                      zone_table[i].cmd[j].arg2,
                      zone_table[i].cmd[j].arg3);
      break;
    case '*':
      sprintf(buf, "%s %s\r\n", buf, 
                   zone_table[i].cmd[j].comment ? zone_table[i].cmd[j].comment : "Empty Comment");
      break;
    case 'K':
      sprintf(buf, "%s Skip next [%d] commands.\r\n", buf,
                   zone_table[i].cmd[j].arg1);
      break;
    case 'X':
      {
         char xstrone[]   = "Set all if-flags to 'unsure' state.";
         char xstrtwo[]   = "Set mob if-flag to 'unsure' state.";
         char xstrthree[] = "Set obj if-flag to 'unsure' state.";
         char xstrfour[]  = "Set %% if-flag to 'unsure' state.";
         char xstrerror[] = "Illegal value in arg1.";
         char * xresultstr;

         switch(zone_table[i].cmd[j].arg1) {
           case 0:
             xresultstr = xstrone;
             break;
           case 1:
             xresultstr = xstrtwo;
             break;
           case 2:
             xresultstr = xstrthree;
             break;
           case 3:
             xresultstr = xstrfour;
             break;
           default:
             xresultstr = xstrerror;
             break;
         }

         sprintf(buf, "%s [%d] %s\r\n", buf, 
                   zone_table[i].cmd[j].arg1, xresultstr);
      }
      break;
    default: 
      sprintf(buf, "Illegal Command: %c %d %d %d %d\r\n", 
                      zone_table[i].cmd[j].command,
                      zone_table[i].cmd[j].if_flag,
                      zone_table[i].cmd[j].arg1,
                      zone_table[i].cmd[j].arg2,
                      zone_table[i].cmd[j].arg3);
      break;
    } // switch

    if(zone_table[i].cmd[j].comment && zone_table[i].cmd[j].command != '*')
      sprintf(buf, "%s       %s\r\n", buf, zone_table[i].cmd[j].comment);

    send_to_char(buf, ch);
  } // for

  send_to_char("\r\nUse zedit to see the rest of the commands if they were truncated.\r\n", ch);
  return eSUCCESS;
}

char *str_nospace(char *stri)
{
  char test[512]; // Why not
  int i = 0;
  if (!stri) return "";
  while (*(stri+i))
  {
     if (*(stri+i) == ' ')
       test[i] = '_';
     else
       test[i] = *(stri+i);
     i++;
  }
  test[i] = '\0';
  return test;
}

int find_file(world_file_list_item *itm, int high)
{
  int i;
  world_file_list_item *tmp;
  for (i = 0,tmp=itm;tmp;tmp = tmp->next,i++)
  if (tmp->lastnum/100 == high/100) return i;
  return -1;
}

int do_show(struct char_data *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH], buf[200];
  char beginrange[MAX_INPUT_LENGTH];
  char endrange[MAX_INPUT_LENGTH];
  char type[MAX_INPUT_LENGTH];
  world_file_list_item * curr = NULL;
  int i;
  int nr;
  int count = 0;
  int begin, end;

  extern world_file_list_item * world_file_list;
  extern world_file_list_item *   mob_file_list;
  extern world_file_list_item *   obj_file_list;
  extern char *dirs[];
  extern int top_of_mobt;
  extern int top_of_objt;
  extern int top_of_world;
 
//  half_chop(argument, type, name);
  argument = one_argument(argument,type);

  //argument = one_argument(argument,name);
  
  int has_range = has_skill(ch, COMMAND_RANGE);

  if(!*type) {
    send_to_char("Format: show <type> <name>.\n\r"
                 "Types:\r\n"
                 "  keydoorcombo\r\n"
                 "  mob\r\n"
                 "  obj\r\n"
                 "  room\r\n"
                 "  zone\r\n"
                 "  zone all\r\n",ch); 
    if(has_range)
      send_to_char("  rfiles\r\n"
                   "  mfiles\r\n"
                   "  ofiles\r\n"
		   "  search\r\n"
		   " msearch\r\n"
		   " rsearch\r\n"
		   " counts\r\n", ch);
    return eFAILURE;
  }

  if(is_abbrev(type,"mobile")) 
  {
    argument = one_argument(argument,name);
    if(!*name) {
       send_to_char("Format:  show mob <keyword>\r\n"
                    "                  <number>\r\n"
                    "                  <beginrange> <endrange>\r\n", ch);
       return eFAILURE;
    }

    if(isdigit(*name)) {
//       half_chop(name, beginrange, endrange);
	strcpy(beginrange, name);
//        beginrange = name;
        argument = one_argument(argument,endrange);
        if(!*endrange)
         strcpy(endrange, "-1");

       if(!check_range_valid_and_convert(begin, beginrange, 0, 100000) ||
          !check_range_valid_and_convert(end, endrange, -1, 100000))
       {
         send_to_char("The begin and end ranges must be valid numbers.\r\n", ch);
         return eFAILURE;
       }
       if(end != -1 && end < begin) {  // swap um
         i = end;
         end = begin;
         begin = i;
       }

       *buf = '\0';
       send_to_char("[#  ] [MOB #] MOB'S DESCRIPTION\n\n\r", ch);

       if(end == -1) {
         if((nr = real_mobile(begin)) >= 0) {
           sprintf(buf, "[  1] [%5d] %s\n\r", begin,
                   ((struct char_data *)(mob_index[nr].item))->short_desc);
           send_to_char(buf, ch);
         }
       }
       else {
         for(i = begin;i < mob_index[top_of_mobt].virt && i <= end; i++) {
            if((nr = real_mobile(i)) < 0)
              continue;

           count++;
           sprintf(buf, "[%3d] [%5d] %s\n\r", count, i, 
              ((struct char_data *)(mob_index[nr].item))->short_desc);
           send_to_char(buf, ch);

           if(count > 200) {
             send_to_char("Maximum number of searchable items hit.  Search ended.\r\n", ch);
             break;
           }
         } 
       } 
    }
    else {
       *buf = '\0';
       send_to_char("[#  ] [MOB #] MOB'S DESCRIPTION\n\n\r", ch);
 
       for(i = 0; (i < mob_index[top_of_mobt].virt); i++) 
       {
          if((nr = real_mobile(i)) < 0)
            continue;

          if(isname(name,
             ((struct char_data *)(mob_index[nr].item))->name)) 
          {
            count++;
            sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, i, ((struct char_data*)(mob_index[nr].item))->level,
                 ((struct char_data *)(mob_index[nr].item))->short_desc);
            send_to_char(buf, ch);

            if(count > 200) {
              send_to_char("Maximum number of searchable items hit.  Search ended.\r\n", ch);
              break;
            }
          }
       }
    }
    if(!*buf)
      send_to_char("Couldn't find any MOBS by that NAME.\n\r", ch);
  } /* "mobile" */
  else if (is_abbrev(type, "counts") && has_range)
  {
     extern int total_rooms;
     csendf(ch, "$3Rooms$R: %d\r\n$3Mobiles$R: %d\r\n$3Objects$R: %d\r\n",
	     total_rooms, top_of_mobt, top_of_objt);
     return eSUCCESS;
  }
  else if (is_abbrev(type,"object")) 
  {
    argument = one_argument(argument,name);
     if(!*name) {
       send_to_char("Format:  show obj <keyword>\r\n"
                    "                  <number>\r\n"
                    "                  <beginrange> <endrange>\r\n", ch);
       return eFAILURE;
    }

    if(isdigit(*name)) {
 //      half_chop(name, beginrange, endrange);
    argument = one_argument(argument,endrange);
	//beginrange = name;
	strcpy(beginrange,name);
        if(!*endrange)
         strcpy(endrange, "-1");

       if(!check_range_valid_and_convert(begin, beginrange, 0, 100000) ||
          !check_range_valid_and_convert(end, endrange, -1, 100000))
       {
         send_to_char("The begin and end ranges must be valid numbers.\r\n", ch);
         return eFAILURE;
       }
       if(end != -1 && end < begin) {  // swap um
         i = end;
         end = begin;
         begin = i;
       }

       *buf = '\0';
       send_to_char("[#  ] [OBJ #] OBJECT'S DESCRIPTION\n\n\r", ch);


       if(end == -1) {
         if((nr = real_object(begin)) >= 0) {
           sprintf(buf, "[  1] [%5d] %s\n\r", begin,
                   ((struct obj_data *)(obj_index[nr].item))->short_description);
           send_to_char(buf, ch);
         }
       }
       else {
         for(i = begin;i < obj_index[top_of_objt].virt && i <= end; i++) {
            if((nr = real_object(i)) < 0)
              continue;

           count++;
           sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, i, ((struct obj_data *)(obj_index[nr].item))->obj_flags.eq_level,
              ((struct obj_data *)(obj_index[nr].item))->short_description);
           send_to_char(buf, ch);

           if(count > 200) {
             send_to_char("Maximum number of searchable items hit.  Search ended.\r\n", ch);
             break;
           }
         } 
       } 
    }
    else {
       *buf = '\0';
       send_to_char("[#  ] [OBJ #] OBJECT'S DESCRIPTION\n\n\r", ch);

       for(i = 0; ( i < obj_index[top_of_objt].virt); i++) 
       {
          if((nr = real_object(i)) < 0)
            continue;

          if(isname(name, ((struct obj_data *)(obj_index[nr].item))->name)) 
          {
            count++;
            sprintf(buf, "[%3d] [%5d] %s\n\r", count, i,
                ((struct obj_data *)(obj_index[nr].item))->short_description);
            send_to_char(buf, ch);
          }

          if(count > 200) {
             send_to_char("Maximum number of searchable items hit.  Search ended.\r\n", ch);
             break;
          }
       }
    }
    if(!*buf)
      send_to_char("Couldn't find any OBJECTS by that NAME.\n\r", ch);
  } /* "object" */
  else if (is_abbrev(type,"room")) 
  {
    argument = one_argument(argument,name);
     if(!*name) {
       send_to_char("Format:  show room <beginrange> <endrange>\r\n", ch);
       return eFAILURE;
    }

    if(isdigit(*name)) {
 //      half_chop(name, beginrange, endrange);
      argument = one_argument(argument,endrange);
//	beginrange = name;
	strcpy(beginrange, name);
        if(!*endrange)
         strcpy(endrange, "-1");

       if(!check_range_valid_and_convert(begin, beginrange, 0, 100000) ||
          !check_range_valid_and_convert(end, endrange, -1, 100000))
       {
         send_to_char("The begin and end ranges must be valid numbers.\r\n", ch);
         return eFAILURE;
       }
       if(end != -1 && end < begin) {  // swap um
         i = end;
         end = begin;
         begin = i;
       }

       *buf = '\0';
       send_to_char("[#  ] [ROOM#] ROOM'S NAME\n\n\r", ch);

       if(end == -1) {
         if(world_array[begin]) {
           sprintf(buf, "[  1] [%5d] %s\n\r", begin, world[begin].name);
           send_to_char(buf, ch);
         }
       }
       else {
         for(i = begin; i < top_of_world && i <= end; i++) {
	   if (!world_array[i]) continue;
           count++;
           sprintf(buf, "[%3d] [%5d] %s\n\r", count, i, world[i].name);
           send_to_char(buf, ch);

           if(count > 200) {
             send_to_char("Maximum number of searchable items hit.  Search ended.\r\n", ch);
             break;
           }
         } 
       } 
    }
    if(!*buf)
      send_to_char("Couldn't find any ROOMS in that range.\n\r", ch);
  } /* "object" */
  else if (is_abbrev(type, "zone"))
  {
    argument = one_argument(argument,name);
     if(!*name) {
       send_to_char("Show which zone? (# or 'all')\r\n", ch);
       return eFAILURE;
    }

    if(!isdigit(*name)) /* show them all */
    {
      send_to_char("Num Name\r\n"
                   "--- ------------------------------------------------------\r\n", ch);
      for(i = 0; i <= top_of_zone_table; i++)
      {
        sprintf(buf, "%3d %5d-%-5d %s\r\n",
                   i, zone_table[i].bottom_rnum, zone_table[i].top_rnum,zone_table[i].name);
        send_to_char(buf, ch);
      }
    }
    else /* was a digit */
    {
       i = atoi(name);
       if((!i && *name != '0') || i < 0) {
          send_to_char("Which zone was that?\r\n", ch);
          return eFAILURE;
       }
       show_zone_commands(ch, i);

    } // else was a digit
  } // zone
  else if (is_abbrev(type, "rsearch") && has_range)
  {
     char arg1[MAX_INPUT_LENGTH];
     int zon, bits = 0, sector = 0;
     extern char *room_bits[], *sector_types[];
     argument = one_argument(argument,arg1);
     if (!is_number(arg1))
     {
	send_to_char("Syntax: show rsearch <zone#> <sectorname/roomflag>\r\n",ch);
	return eSUCCESS;
     }
     zon = atoi(arg1);
//     room_data  
//   zone
//   sector_type
// room_flags
       while ( (argument = one_argument(argument,arg1))!=NULL)
       {
	  if (arg1[0] == '\0') break;
	  int i;
	  for (i = 0; room_bits[i][0] != '\n'; i++)
	    if (!str_cmp(arg1, room_bits[i])) {
		SET_BIT(bits, 1<<i);
		continue;
	    }
	   for (i = 0; sector_types[i][0] != '\n'; i++)
	     if (!str_cmp(arg1,sector_types[i])) {
		sector = i-1;
		continue;
	    }
	   send_to_char("Unknown room-flag or sector type.\r\n",ch);
       }
	if (!bits && !sector)
	{
	  send_to_char("Syntax: show rsearch <zone number> <flags/sector type\r\n",ch);
	  return eSUCCESS;
	}
        if (zon > top_of_zone_table)
        {
	  send_to_char("Unknown zone.\r\n",ch);
 	  return eFAILURE;
         }
	char buf[MAX_INPUT_LENGTH];
       for (i = zone_table[zon].bottom_rnum; i < zone_table[zon].top_rnum;i++)
       {
	 if (!world_array[i]) continue;
	 if (bits)
  	   if (!IS_SET(world[i].room_flags,bits))
	 	continue;
	  if (sector)
	    if (world[i].sector_type != sector)
		continue;
	 sprintf(buf,"[%3d] %s\r\n", i, world[i].name);
	 send_to_char(buf,ch);
       }
  }
  else if (is_abbrev(type, "msearch") && has_range)
  {  // Mobile search.
    char arg1[MAX_STRING_LENGTH];
    int act = 0, clas = 0, levlow = -555, levhigh = -555, affect = 0, immune = 0, race = -1, align = 0;
    extern char *action_bits[];
    extern struct race_shit race_info[];
    extern char *isr_bits[];
    extern char *affected_bits[];
    extern char *pc_clss_types2[];
    //int its;
//    if (
    bool fo = FALSE;
    while ( ( argument = one_argument(argument, arg1) ) )
    {
       int i;
       if (strlen(arg1) < 2) break;
	fo = TRUE;
       for (i = 0; *pc_clss_types2[i] != '\n' ; i++)
        if (!str_cmp(pc_clss_types2[i],arg1))
        {
          clas = i;
          goto thisLoop;
        }
       for (i = 0; *isr_bits[i] != '\n' ; i++)
        if (!str_cmp(str_nospace(isr_bits[i]),arg1))
        {
          SET_BIT(immune, 1<<i);
          goto thisLoop;
        }
       for (i = 0; *action_bits[i] != '\n' ; i++)
        if (!str_cmp(str_nospace(action_bits[i]),arg1))
        {
          SET_BIT(act, 1<<i);
          goto thisLoop;
        }
       for (i = 0; *affected_bits[i] != '\n' ; i++)
        if (!str_cmp(str_nospace(affected_bits[i]),arg1))
        {
          SET_BIT(affect, 1<<i);
          goto thisLoop;
        }
       for (i = 0; i <= MAX_RACE; i++)
        if (!str_cmp(str_nospace(race_info[i].singular_name),arg1))
        {
          race = i;
          goto thisLoop;
        }
	if (!str_cmp(arg1,"evil"))
	  align = 3;
	else if (!str_cmp(arg1,"good"))
	  align = 1;
	else if (!str_cmp(arg1,"neutral"))
	  align = 2;
	if (!str_cmp(arg1,"level"))
	{
	  argument = one_argument(argument,arg1);
	  if (is_number(arg1))
	    levlow = atoi(arg1);
	  argument = one_argument(argument,arg1);
	  if (is_number(arg1))
	    levhigh = atoi(arg1);
	  if (levhigh ==-555 || levlow==-555)
	  {
	    send_to_char("Incorrect level requirement.\r\n",ch);
	    return eFAILURE;
	  }
	}
       thisLoop:
	continue;
     }
     if (!fo)
     {
	int z,o=0;
	for (z = 0; *action_bits[z] != '\n';z++)
	{
	   o++;
	   send_to_char(str_nospace(action_bits[z]),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
	for (z = 0; *isr_bits[z] != '\n';z++)
	{
	   o++;
	   send_to_char(str_nospace(isr_bits[z]),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
	for (z = 0; *affected_bits[z] != '\n';z++)
	{
	   o++;
	   send_to_char(str_nospace(affected_bits[z]),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
	for (z = 0; *pc_clss_types2[z] != '\n';z++)
	{
	   o++;
	   send_to_char(pc_clss_types2[z],ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
      for (i = 0; i <= MAX_RACE; i++)
	{
	   o++;
	   send_to_char(str_nospace(race_info[i].singular_name),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
           send_to_char("level",ch);
           if (o%7==0)
             send_to_char("\r\n",ch);
           else
             send_to_char(" ", ch);

           send_to_char("good",ch);
           if (o%7==0)
             send_to_char("\r\n",ch);
           else
             send_to_char(" ", ch);
            send_to_char("evil",ch);
           if (o%7==0)
             send_to_char("\r\n",ch);
           else
             send_to_char(" ", ch);
            send_to_char("neutral",ch);
           if (o%7==0)
             send_to_char("\r\n",ch);
           else
             send_to_char(" ", ch);
 
	return eSUCCESS;
     }
     int c,nr;
	if (!act && !clas && !levlow && !levhigh && !affect && !immune && !race && !align)
	{
	   send_to_char("No valid search supplied.\r\n",ch);
	   return eFAILURE;
	}
     for (c=0;c < mob_index[top_of_mobt].virt;c++)
     {
      if ((nr = real_mobile(c)) < 0)	
           continue;
      if(race > -1)
       if (((struct char_data *)(mob_index[nr].item))->race != race)
         continue;
      if (align) {
	if (align == 1 && ((struct char_data *)(mob_index[nr].item))->alignment < 350)
	  continue;
	else if (align == 2 && (((struct char_data*)(mob_index[nr].item))->alignment < -350 || ((struct char_data*)(mob_index[nr].item))->alignment > 350))
	continue;
	else if (align == 3 && ((struct char_data *)(mob_index[nr].item))->alignment > -350)
	continue;
      }
      if (immune)
	if (!IS_SET(((struct char_data *)(mob_index[nr].item))->immune, immune))
	 continue;
      if (clas)
        if (((struct char_data *)(mob_index[nr].item))->c_class != clas)
          continue;
      if (levlow!=-555)
	if (((struct char_data *)(mob_index[nr].item))->level < levlow)
	  continue;
      if (levhigh!=-555)
	if (((struct char_data *)(mob_index[nr].item))->level > levhigh)
	  continue;
      if(act)
	for (i = 0; i < 31; i++)
           if (IS_SET(act,1<<i))
      if (!ISSET(((struct char_data *)(mob_index[nr].item))->mobdata->actflags, 1<<i))
         goto eheh;
      if(affect)
	for (i = 0; i < 31; i++)
           if (IS_SET(affect,1<<i))
      		if (!ISSET(((struct char_data *)(mob_index[nr].item))->affected_by, 1<<i))
        		goto eheh;
      count++;
      if (count > 200)
      {
        send_to_char("Limit reached.\r\n",ch);
        break;
      }
           sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, c, ((struct 
char_data *)(mob_index[nr].item))->level,
              ((struct char_data *)(mob_index[nr].item))->short_desc);
      send_to_char(buf,ch);
     eheh:
	continue;
     }
  }
  else if (is_abbrev(type, "search") && has_range)
  {  // Object search.
    char arg1[MAX_STRING_LENGTH];
    int affect = 0, size =0, extra = 0, more = 0, wear = 0,type =0;
    int levlow = -555, levhigh = -555,dam = 0,lweight = -555, hweight = -555;
    int any = 0;
    extern char *wear_bits[];
    extern char *extra_bits[];
    extern char *more_obj_bits[];
    extern char *size_bitfields[];
    extern char *apply_types[];
    extern char *item_types[];
    extern char *strs_damage_types[];
    bool fo = FALSE;
    int its;
    while ( ( argument = one_argument(argument, arg1) ) )
    {
       int i;
       if (strlen(arg1) < 2) break;
	fo = TRUE;
       for (i = 0; *wear_bits[i] != '\n' ; i++)
	if (!str_cmp(str_nospace(wear_bits[i]),arg1))
	{
	  SET_BIT(wear, 1<<i);
	  goto endy;
	}
	for (i=0; *item_types[i] != '\n';i++)
	{
	  if (!str_cmp(str_nospace(item_types[i]),arg1))
	  {
	     type = i;
	     goto endy;
	  }
	}
       for (i = 0; *strs_damage_types[i] != '\n' ; i++)
        if (!str_cmp(str_nospace(strs_damage_types[i]),arg1))
        {
	  dam = i;
          goto endy;
        }
       for (i = 0; *extra_bits[i] != '\n' ; i++)
        if (!str_cmp(str_nospace(extra_bits[i]),arg1))
        {
	if (!str_cmp(extra_bits[i], "ANY_CLASS"))
	  any = i;
	else
          SET_BIT(extra, 1<<i);
          goto endy;
        }
       for (i = 0; *more_obj_bits[i] != '\n' ; i++)
        if (!str_cmp(str_nospace(more_obj_bits[i]),arg1))
        {
          SET_BIT(more, 1<<i);
          goto endy;
        }
       for (i = 0; *size_bitfields[i] != '\n' ; i++)
        if (!str_cmp(str_nospace(size_bitfields[i]),arg1))
        {
          SET_BIT(size, 1<<i);
          goto endy;
        }
       for (i = 0; *apply_types[i] != '\n' ; i++)
        if (!str_cmp(str_nospace(apply_types[i]),arg1))
        {
  	    affect = i;
            goto endy;
        }
       if (!str_cmp(arg1,"olevel"))
       {
         argument = one_argument(argument,arg1);
         if (is_number(arg1))
           levlow = atoi(arg1);

         argument = one_argument(argument,arg1);
         if (is_number(arg1))
           levhigh = atoi(arg1);

         if (levhigh == -555 || levlow == -555)
         {
           send_to_char("Incorrect level requirement.\r\n",ch);
           return eFAILURE;
         }
       }
       if (!str_cmp(arg1,"oweight"))
       {
         argument = one_argument(argument,arg1);
         if (is_number(arg1))
           lweight = atoi(arg1);

         argument = one_argument(argument,arg1);
         if (is_number(arg1))
           hweight = atoi(arg1);

         if (lweight == -555 || hweight == -555)
         {
           send_to_char("Incorrect weight requirement.\r\n",ch);
           return eFAILURE;
         }
      }
       csendf(ch, "Unknown type: %s.\r\n", arg1);
       endy:
	continue;
     }
     int c,nr,aff;
//     csendf(ch,"%d %d %d %d %d", more, extra, wear, size, affect);
     bool found = FALSE;
    int o = 0, z;
    if (!fo)
    {
	for (z = 0; *wear_bits[z] != '\n';z++)
	{
	   o++;
	   send_to_char(str_nospace(wear_bits[z]),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
	for (z = 0; *extra_bits[z] != '\n';z++)
	{
	   o++;
	   send_to_char(str_nospace(extra_bits[z]),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
        for (z = 0; *strs_damage_types[z] != '\n';z++)
        {
           o++;
           send_to_char(str_nospace(strs_damage_types[z]),ch);
           if (o%7==0)
             send_to_char("\r\n",ch);
           else
             send_to_char(" ", ch);
        }

        for (z = 0; *more_obj_bits[z] != '\n';z++)
        {
           o++;
           send_to_char(str_nospace(more_obj_bits[z]),ch);
           if (o%7==0)
             send_to_char("\r\n",ch);
           else
             send_to_char(" ", ch);
        }
	for (z = 0; *item_types[z] != '\n';z++)
	{
	   o++;
	   send_to_char(str_nospace(item_types[z]),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
	for (z = 0; *size_bitfields[z] != '\n';z++)
	{
	   o++;
	   send_to_char(str_nospace(size_bitfields[z]),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
	for (z = 0; *apply_types[z] != '\n';z++)
	{
	   o++;
	   send_to_char(str_nospace(apply_types[z]),ch);
	   if (o%7==0)
	     send_to_char("\r\n",ch);
	   else
	     send_to_char(" ", ch);
	}
           send_to_char("oweight",ch);
           if (o%7==0)
             send_to_char("\r\n",ch);
           else
             send_to_char(" ", ch);

           send_to_char("olevel",ch);
           if (o%7==0)
             send_to_char("\r\n",ch);
           else
             send_to_char(" ", ch);

	return eSUCCESS;
    }

     for (c=0;c < obj_index[top_of_objt].virt;c++)
     {
       found = FALSE;
      if ((nr = real_object(c)) < 0)
           continue;
      if(wear)
	for (i = 0; i < 20; i++)
	  if (IS_SET(wear, 1 << i))
      if (!IS_SET(((struct obj_data *)(obj_index[nr].item))->obj_flags.wear_flags, 1<<i))
	goto endLoop;
	if (type)
	  if (((struct obj_data *)(obj_index[nr].item))->obj_flags.type_flag != type)
	    continue;
     if (lweight!=-555)
      if (((struct obj_data *)(obj_index[nr].item))->obj_flags.weight < lweight)
         continue;
     if (hweight!=-555)
      if (((struct obj_data *)(obj_index[nr].item))->obj_flags.weight > hweight)
         continue;

     if (levhigh!=-555)
      if (((struct obj_data *)(obj_index[nr].item))->obj_flags.eq_level > levhigh)
         continue;
     if (levlow!=-555)
      if (((struct obj_data *)(obj_index[nr].item))->obj_flags.eq_level < levlow)
         continue;
      if (size)
       for (i = 0; i < 10; i++)
	if (IS_SET(size, 1<<i))
      if (!IS_SET(((struct obj_data *)(obj_index[nr].item))->obj_flags.size, 1<<i))
	goto endLoop;
if (((struct obj_data *)(obj_index[nr].item))->obj_flags.type_flag == ITEM_WEAPON) {
int get_weapon_damage_type(struct obj_data * wielded);
its = get_weapon_damage_type(((struct obj_data *)(obj_index[nr].item)));
	}
     if (dam && dam != (its-1000))
  	  continue;
      if(extra)
        for (i = 0; i < 30; i++)
	  if (IS_SET(extra,1<<i))
      if (!IS_SET(((struct obj_data *)(obj_index[nr].item))->obj_flags.extra_flags, 1<<i)
	&& !(any && IS_SET(((struct obj_data *)(obj_index[nr].item))->obj_flags.extra_flags, 1<<any)))
	goto endLoop;

      if (more)
	for (i = 0; i < 10; i++)
	  if (IS_SET(more,1<<i))
      if (!IS_SET(((struct obj_data *)(obj_index[nr].item))->obj_flags.more_flags, 1<<i))
	goto endLoop;
//      int aff,total = 0;
  //    bool found = FALSE;
      for (aff = 0; aff < ((struct obj_data *)(obj_index[nr].item))->num_affects;aff++)
	 if (affect== ((struct obj_data *)(obj_index[nr].item))->affected[aff].location)
	   found = TRUE;
     if (affect)
        if (!found)
          continue;
      count++;
      if (count > 200)
      {
	send_to_char("Limit reached.\r\n",ch);
	break;
      }
           sprintf(buf, "[%3d] [%5d] [%2d] %s\n\r", count, c, ((struct
obj_data *)(obj_index[nr].item))->obj_flags.eq_level,
              ((struct obj_data *)(obj_index[nr].item))->short_description);
           send_to_char(buf, ch);
       endLoop:
	continue;
     }
  }
  else if (is_abbrev(type, "rfiles") && has_range)
  {
    curr = world_file_list;
    i = 0;
    send_to_char("ID ) Filename                       Begin  End\r\n"
                 "----------------------------------------------------------\r\n", ch);
    while(curr) {
      sprintf(buf, "%3d) %-30s %-6ld %-6ld %1s%1s%1s %s\r\n", i++, curr->filename, 
        curr->firstnum, curr->lastnum,
        IS_SET(curr->flags, WORLD_FILE_IN_PROGRESS) ? "$B$1*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_READY)       ? "$B$5*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_APPROVED)    ? "$B$2*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_MODIFIED) ? "MODIFIED" : "");
      send_to_char(buf, ch);
      curr = curr->next;
    }
  }
  else if (is_abbrev(type, "mfiles") && has_range)
  {
    curr = mob_file_list;
    i = 0;
    send_to_char("ID ) Filename                       Begin  End\r\n"
                 "----------------------------------------------------------\r\n", ch);
    while(curr) {
      sprintf(buf, "%3d) %-30s %-6ld %-6ld %1s%1s%1s %s\r\n", i++, curr->filename, 
        curr->firstnum, curr->lastnum,  
        IS_SET(curr->flags, WORLD_FILE_IN_PROGRESS) ? "$B$1*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_READY)       ? "$B$5*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_APPROVED)    ? "$B$2*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_MODIFIED) ? "MODIFIED" : "");
      send_to_char(buf, ch);
      curr = curr->next;
    }
  }
  else if (is_abbrev(type, "ofiles") && has_range)
  {
    curr = obj_file_list;
    i = 0;
    send_to_char("ID ) Filename                       Begin  End\r\n"
                 "----------------------------------------------------------\r\n", ch);
    while(curr) {
      sprintf(buf, "%3d) %-30s %-6ld %-6ld %1s%1s%1s %s\r\n", i++, curr->filename, 
        curr->firstnum, curr->lastnum,  
        IS_SET(curr->flags, WORLD_FILE_IN_PROGRESS) ? "$B$1*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_READY)       ? "$B$5*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_APPROVED)    ? "$B$2*$R" : " ",
        IS_SET(curr->flags, WORLD_FILE_MODIFIED) ? "MODIFIED" : "");
      send_to_char(buf, ch);
      curr = curr->next;
    }
  }
  else if (is_abbrev(type, "keydoorcombo"))
  {
    if(!*name) {
       send_to_char("Show which key? (# of key)\r\n", ch);
       return eFAILURE;
    }

    count = atoi(name);
    if((!count && *name != '0') || count < 0) {
       send_to_char("Which key was that?\r\n", ch);
       return eFAILURE;
    }

    csendf(ch, "$3Doors in game that use key %d$R:\r\n\r\n", count);
    for(i = 0; i < top_of_world; i++)
       for(nr = 0; nr < MAX_DIRS; nr++)
         if(world_array[i] && world_array[i]->dir_option[nr])
         {
           if(IS_SET(world_array[i]->dir_option[nr]->exit_info, EX_ISDOOR) &&
              world_array[i]->dir_option[nr]->key == count)
           {
             csendf(ch, " $3Room$R: %5d $3Dir$R: %5s $3Key$R: %d\r\n", 
                    world_array[i]->number, dirs[nr], world_array[i]->dir_option[nr]->key);
           }
         }
  }
  else send_to_char("Illegal type.  Type just 'show' for legal types.\r\n",ch);
  return eSUCCESS;
}

int do_trans(struct char_data *ch, char *argument, int cmd)
{
    struct descriptor_data *i;
    struct char_data *victim;
    char buf[100];
    int target;

    if (IS_NPC(ch))
        return eFAILURE;

    one_argument(argument,buf);
    if (!*buf)
        send_to_char("Whom do you wish to transfer?\n\r",ch);
    else if (str_cmp("all", buf)) 
    {
       if (!(victim = get_char_vis(ch,buf)))
          send_to_char("No-one by that name around.\n\r",ch);
       else {
          if (world[ch->in_room].number == 25 && !isname(GET_NAME(ch), "Pirahna"))
          {
            send_to_char ("Damn! That is rude! This ain't your place. :P\n\r", ch);
            return eFAILURE;
          }
          act("$n disappears in a mushroom cloud.", victim, 0, 0, TO_ROOM, 0);
          target = ch->in_room;
          csendf(ch, "Moving %s from %d to %d.\n\r", GET_NAME(victim),
               world[victim->in_room].number, world[target].number);
          move_char(victim, target);
          act("$n arrives from a puff of smoke.", victim, 0, 0, TO_ROOM, 0);
          act("$n has transferred you!",ch,0,victim,TO_VICT, 0);
          do_look(victim,"",15);
          send_to_char("Ok.\n\r",ch);
       }
    } else { /* Trans All */
       for (i = descriptor_list; i; i = i->next)
          if (i->character != ch && !i->connected) 
          {
             victim = i->character;
             act("$n disappears in a mushroom cloud.", victim, 0, 0, TO_ROOM, 0);
             target = ch->in_room;
             move_char(victim, target);
             act("$n arrives from a puff of smoke.", victim, 0, 0, TO_ROOM, 0);
             act("$n has transferred you!",ch,0,victim,TO_VICT, 0);
             do_look(victim,"",15);
          }

       send_to_char("Ok.\n\r",ch);
    }
    return eSUCCESS;
}

int do_teleport(struct char_data *ch, char *argument, int cmd)
{
   struct char_data *victim, *target_mob, *pers;
   char person[MAX_INPUT_LENGTH], room[MAX_INPUT_LENGTH];
   int target;
   int loop;
   //extern int top_of_world;

   if (IS_NPC(ch)) return eFAILURE;

   half_chop(argument, person, room);

   if (!*person) {
      send_to_char("Who do you wish to teleport?\n\r", ch);
      return eFAILURE;
    } /* if */

   if (!*room) {
      send_to_char("Where do you wish to send ths person?\n\r", ch);
      return eFAILURE;
    } /* if */

   if (!(victim = get_char_vis(ch, person))) {
      send_to_char("No-one by that name around.\n\r", ch);
      return eFAILURE;
    } /* if */

   if (isdigit(*room)) {
      target = atoi(&room[0]);
      if((*room != '0' && target == 0) || !world_array[target]) {
         send_to_char("No room exists with that number.\n\r" ,ch);
         return eFAILURE;
      }
//      for (loop = 0; loop <= top_of_world; loop++) {
//         if (world[loop].number == target) {
//            target = (int16)loop;
//            break;
//      } else if (loop == top_of_world) {
//            send_to_char("No room exists with that number.\n\r", ch);
//            return eFAILURE;
//      } /* if */
//       } /* for */
    } else if ( ( target_mob = get_char_vis(ch, room) ) != NULL ) {
      target = target_mob->in_room;
    } else {
      send_to_char("No such target (person) can be found.\n\r", ch);
      return eFAILURE;
    } /* if */

    if (world[target].number == 25 && !isname (GET_NAME(victim), "Pirahna"))
      {
      send_to_char ("Sorry, you need permission first!\n\r", ch);
      return eFAILURE;
      }

   if (IS_SET(world[target].room_flags, PRIVATE)) {
      for (loop = 0, pers = world[target].people; pers;
           pers = pers->next_in_room, loop++);
      if (loop > 1) {
          send_to_char(
            "There's a private conversation going on in that room\n\r",
            ch);
         return eFAILURE;
      } /* if */
   } /* if */

   if (IS_SET(world[target].room_flags, IMP_ONLY) && GET_LEVEL(ch) < IMP) {
      send_to_char("No.\n\r", ch);
      return eFAILURE;
   }

   if (IS_SET(world[target].room_flags, CLAN_ROOM) && 
       GET_LEVEL(ch) < DEITY) {
      send_to_char("No.\n\r", ch);
      return eFAILURE;
   }

   act("$n disappears in a puff of smoke.",  victim, 0, 0, TO_ROOM, 0);
   csendf(ch, "Moving %s from %d to %d.\n\r", GET_NAME(victim),
	  world[victim->in_room].number, world[target].number);
   move_char(victim, target);
   act("$n arrives from a puff of smoke.",  victim, 0, 0, TO_ROOM, 0);
   act("$n has teleported you!",  ch, 0, (char *)victim, TO_VICT, 0);
   do_look(victim, "", 15);
   send_to_char("Teleport completed.\n\r", ch);

   return eSUCCESS;
} /* do_teleport */

int do_gtrans(struct char_data *ch, char *argument, int cmd)
{
    // struct descriptor_data *i;
    struct char_data *victim;
    char buf[100];
    int target;
    struct follow_type *k, *next_dude;

    if (IS_NPC(ch))
        return eFAILURE;

    one_argument(argument,buf);
    if (!*buf) {
        send_to_char("Whom is the group leader you wish to transfer?\n\r",ch);
        return eFAILURE;
    }

    if (!(victim = get_char_vis(ch,buf))) {
            send_to_char("No-one by that name around.\n\r",ch);
            return eFAILURE;
        }
        else 
        {
            act("$n disappears in a mushroom cloud.",
                victim, 0, 0, TO_ROOM, 0);
            target = ch->in_room;
	    csendf(ch, "Moving %s from %d to %d.\n\r", GET_NAME(victim),
	           world[victim->in_room].number, world[target].number);
	    move_char(victim, target);
            act("$n arrives from a puff of smoke.",
                victim, 0, 0, TO_ROOM, 0);
            act("$n has transferred you!",ch,0,victim,TO_VICT, 0);
            do_look(victim,"",15);

            if (victim->followers)
               for(k = victim->followers; k; k = next_dude) 
               {
                  next_dude = k->next;
                  if(!IS_NPC(k->follower) && IS_AFFECTED(k->follower, AFF_GROUP))
                  {
                     act("$n disappears in a mushroom cloud.",
                       victim, 0, 0, TO_ROOM, 0);
                     target = ch->in_room;
                     csendf(ch, "Moving %s from %d to %d.\n\r", GET_NAME(k->follower),
                        world[k->follower->in_room].number, world[target].number);
                     move_char(k->follower, target);
                     act("$n arrives from a puff of smoke.",
                     k->follower, 0, 0, TO_ROOM, 0);
                     act("$n has transferred you!",ch,0,k->follower,TO_VICT, 0);
                     do_look(k->follower,"",15);
                  }
               } /* for */
           send_to_char("Ok.\n\r",ch);
       } /* else */
  return eSUCCESS;
}

char *oprog_type_to_name(int type)
{
  switch (type)
  {
     case ALL_GREET_PROG: return "all_greet_prog";
     case WEAPON_PROG: return "weapon_prog";
     case ARMOUR_PROG: return "armour_prog";
     case LOAD_PROG: return "load_prog";
     case COMMAND_PROG: return "command_prog";
     case ACT_PROG: return "act_prog";
     case ARAND_PROG: return "arand_prog";
     case CATCH_PROG: return "catch_prog";
     case SPEECH_PROG: return "speech_prog";
     case RAND_PROG: return "rand_prog";
     default: return "ERROR_PROG";
  }
}

void opstat(char_data *ch, int vnum)
{
  int num = real_object(vnum);
  OBJ_DATA *obj;
  char buf[MAX_STRING_LENGTH];
  if (num < 0)
  {
    send_to_char("Error, non-existant object.\r\n",ch);
    return;
  }
  obj = (OBJ_DATA*)obj_index[num].item;
  sprintf(buf,"$3Object$R: %s   $3Vnum$R: %d.\r\n",
	 obj->name, vnum);
  send_to_char(buf,ch);
  if ( obj_index[num].progtypes == 0)
  {
     send_to_char("This object has no special procedures.\r\n",ch);
     return;
  }
  send_to_char("\r\n",ch);
  MPROG_DATA *mprg;
   int i;
   char buf2[MAX_STRING_LENGTH];
    for ( mprg = obj_index[num].mobprogs, i = 1; mprg != NULL;
         i++, mprg = mprg->next )
    {
      sprintf( buf, "$3%d$R>$3$B", i);
      send_to_char( buf, ch );
      send_to_char(oprog_type_to_name( mprg->type ), ch);
      send_to_char("$R ", ch);
      sprintf( buf, "$B$5%s$R\n\r", mprg->arglist);
      send_to_char(buf, ch);
      sprintf( buf, "%s\n\r", mprg->comlist );
      double_dollars(buf2, buf);
      send_to_char( buf2, ch );
    }
}

int do_opstat(char_data *ch, char *argument, int cmd)
{
  int vnum = -1;
  if(!has_skill(ch, COMMAND_OPSTAT)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }
  if (isdigit(*argument))
  {
     vnum = atoi(argument);
  } else vnum = ch->pcdata->last_obj_edit;
  opstat(ch,vnum);
  return eSUCCESS;
}

void update_objprog_bits(int num)
{
    MPROG_DATA * prog = obj_index[num].mobprogs;
    obj_index[num].progtypes = 0;

    while(prog) {
      SET_BIT(obj_index[num].progtypes, prog->type);
      prog = prog->next;
    }
}


int do_opedit(char_data *ch, char *argument, int cmd)
{
  int num = -1,vnum = -1,i=-1,a=-1;
  char arg[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg);
  if (IS_NPC(ch)) return eFAILURE;
  if (isdigit(*arg))
  {
    vnum = atoi(arg);
    argument = one_argument(argument, arg);
  } else {
    vnum = obj_index[ch->pcdata->last_obj_edit].virt;

  }

  if ((num = real_object(vnum)) < 0)
  {
      send_to_char("No such object.\r\n",ch);
      return eFAILURE;
  }
  if (!can_modify_object(ch, vnum))
  {
     send_to_char("You are unable to work creation outside your range.\r\n",ch);
     return eFAILURE;
  }
  ch->pcdata->last_obj_edit = num;
/*  if (!*arg)
  {
    opstat(ch, vnum);
    return eSUCCESS;
  }*/
  MPROG_DATA *prog, *currprog;
  if (!str_cmp(arg, "add"))
  {
     argument = one_argument(argument, arg);
     if (!*arg)
     {
	send_to_char("$3Syntax$R: opedit [num] add new\r\n"
		     "This creates a new object proc.\r\n",ch);
	return eFAILURE;
     }
#ifdef LEAK_CHECK
        prog = (MPROG_DATA *) calloc(1, sizeof(MPROG_DATA));
#else
        prog = (MPROG_DATA *) dc_alloc(1, sizeof(MPROG_DATA));
#endif
        prog->type = ALL_GREET_PROG;
        prog->arglist = strdup("80");
        prog->comlist = strdup("say This is my new obj prog!\n\r");
        prog->next = NULL;

        if((currprog = obj_index[num].mobprogs)) {
          while(currprog->next)
            currprog = currprog->next;
          currprog->next = prog;
        }
        else
          obj_index[num].mobprogs = prog;
	update_objprog_bits(num);
	send_to_char("New obj proc created.\r\n",ch);
	return eSUCCESS;
  } else if (!str_cmp(arg, "remove"))
  {
    argument = one_argument(argument,arg);
    int a = -1;
    if (!*arg || !isdigit(*arg))
    {
	send_to_char("$3Syntax$R: opedit [obj_num] remove <prog>\r\n"
			"This removes an object procedure completly\r\n",ch);
	return eFAILURE;
    }
    a = atoi(arg);
    prog = NULL;
    for(i = 1, currprog = obj_index[num].mobprogs;
          currprog && i != a;
          i++, prog = currprog, currprog = currprog->next)
        ;
    if (!currprog)
    {
      send_to_char("Invalid proc number.\r\n",ch);
	return eFAILURE;
    }
    if(prog)
      prog->next = currprog->next;
    else obj_index[num].mobprogs = currprog->next;

        currprog->type = 0;
        dc_free(currprog->arglist);
        dc_free(currprog->comlist);
        dc_free(currprog);

        update_objprog_bits(num);

        send_to_char("Program deleted.\r\n", ch);
	return eSUCCESS;
  } else if (!str_cmp(arg, "type"))
  {
     argument = one_argument(argument, arg);
     if (!*arg || !argument || !*argument || !isdigit(*arg) || !isdigit(*(1+argument)))
     {
	send_to_char("$3Syntax$R: opedit [obj_num] type <prog> <type>\r\n",ch);
	send_to_char("$3Valid types are$R:\r\n",ch);
	char buf[MAX_STRING_LENGTH];
	for (i = 0; *obj_types[i] != '\n'; i++)
	{
	  sprintf(buf, " %2d - %15s\r\n",
		i+1, obj_types[i]);
	  send_to_char(buf,ch);
	}
	return eFAILURE;
     }
     int a = atoi(arg);
     for(i = 1, currprog = obj_index[num].mobprogs;
       currprog && i != a;
       i++, currprog = currprog->next)
        ;

     if(!currprog) {
       send_to_char("Invalid prog number.\r\n", ch);
       return eFAILURE;
     }
    switch (atoi(argument+1))
    {
	case 1: a = ACT_PROG; break;
	case 2: a = SPEECH_PROG; break;
	case 3: a = RAND_PROG; break;
	case 4: a = ALL_GREET_PROG; break;
	case 5: a = CATCH_PROG; break;
	case 6: a = ARAND_PROG; break;
	case 7: a = LOAD_PROG; break;
	case 8: a = COMMAND_PROG; break;
	case 9: a = WEAPON_PROG; break;
	case 10: a = ARMOUR_PROG; break;
	default: send_to_char("Invalid progtype.\r\n",ch); return eFAILURE;
    }
    currprog->type = a;
    update_objprog_bits(num);
    send_to_char("Proc type changed.\r\n",ch);
    return eSUCCESS;
  } else if (!str_cmp(arg, "arglist"))
  {
//    char arg1[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
//    argument = one_argument(argument, arg1);
    if (!*arg || !argument || !*argument || !isdigit(*arg))
    {
	send_to_char("$3Syntax$R: opedit [obj_num] arglist <prog> <new arglist>\r\n",ch);
	return eFAILURE;
    }
    a = atoi(arg);
        for(i = 1, currprog = obj_index[num].mobprogs;
            currprog && i != a;
            i++, currprog = currprog->next)
          ;

        if(!currprog) {
          send_to_char("Invalid prog number.\r\n", ch);
          return eFAILURE;
        }
        dc_free(currprog->arglist);
        currprog->arglist = strdup(argument+1);

        send_to_char("Arglist changed.\r\n", ch);
 	return eSUCCESS;
  } else if (!str_cmp(arg,"command"))
  {
    argument = one_argument(argument, arg);
    if (!*arg || !isdigit(*arg))
    {
	send_to_char("$3Syntax$R: opedit [obj_num] command <prog>\r\n",ch);
	return eFAILURE;
    }
     a = atoi(arg);
        for(i = 1, currprog = obj_index[num].mobprogs;
            currprog && i != a;
            i++, currprog = currprog->next)
          ;

        if(!currprog) { // intval was too high
          send_to_char("Invalid prog number.\r\n", ch);
          return eFAILURE;
        }
        ch->desc->backstr = NULL;
     send_to_char("        Write your help entry and stay within the line.(/s saves /h for help)\r\n"
                     "|--------------------------------------------------------------------------------|\r\n", ch);

        if (currprog->comlist) {
          ch->desc->backstr = str_dup(currprog->comlist);
          send_to_char(ch->desc->backstr, ch);
        }
        ch->desc->connected = CON_EDIT_MPROG;
        ch->desc->strnew = &(currprog->comlist);
        ch->desc->max_str = MAX_MESSAGE_LENGTH;
    	return eSUCCESS;
  } else if (!str_cmp(arg, "list"))
  {
   opstat(ch, vnum);
   return eSUCCESS;
  }
  send_to_char("$3Syntax$R: opedit [obj_num] [field] [arg]\r\n"
		"Edit a field with no args for help on that field.\r\n\r\n"
		"The field must be one of the following:\r\n"
		"\tadd\tremove\ttype\targlist\r\n\tcommand\tlist\r\n\r\n",ch);
 char buf[MAX_STRING_LENGTH];
  sprintf(buf,"$3Current object set to: %d\r\n",ch->pcdata->last_obj_edit);
  send_to_char(buf,ch);
  return eSUCCESS;
}
