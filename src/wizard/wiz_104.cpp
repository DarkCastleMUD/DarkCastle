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

extern struct room_data ** world_array;

int do_thunder(struct char_data *ch, char *argument, int cmd)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct descriptor_data *i;

  for (; *argument == ' '; argument++);

  if(!(*argument))
    send_to_char("It's not gonna look that impressive...\n\r", ch);
  else {
    if(cmd == 9) {
      sprintf(buf1, "$B$4%s thunders '%s'$R\n\r",GET_SHORT(ch), argument);
      sprintf(buf2, "$4$BYou thunder '%s'$R", argument);
    }
    else { 
      sprintf(buf1, "$7$B$n bellows '%s'$R", argument);
      sprintf(buf2, "$7$BYou bellow '%s'$R", argument);
    }

    act(buf2, ch, 0, 0, TO_CHAR, 0);

    for (i = descriptor_list; i; i = i->next)
      if (i->character != ch && !i->connected)
        if(cmd == 9) 
          colorCharSend(buf1, i->character);
        else  
          act(buf1, ch, 0, i->character, TO_VICT, 0);
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
        if(GET_LEVEL(ch) < DEITY && !can_modify_mobile(ch, number)) {
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
        if((GET_LEVEL(ch) < IMP) && 
           IS_SET(((struct obj_data *)(obj_index[number].item))->obj_flags.extra_flags, ITEM_SPECIAL)) 
        {
          send_to_char("Why would you want to load that?\n\r", ch);
          return eFAILURE;
        }
        if(GET_LEVEL(ch) < DEITY && !can_modify_object(ch, number)) {
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
    send_to_char("There is no zone that high.\r\n", ch);
    return eFAILURE;
  }

  sprintf(buf, "$3Name$R: %s\r\n$3Starts$R:   %6d   $3Ends$R:  %6d\r\n"
               "$3Lifetime$R: %6d   $3Age$R:   %6d   $3Left$R:   %6d\r\n" 
               "$3PC'sInZone$R: %6d $3Mode$R: %-12s $3Flags$R: ", 
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
  send_to_char("\r\n\r\n", ch);

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

    switch(zone_table[i].cmd[j].command) {
    case 'M':
      sprintf(buf, "%s $B$1Load mob  [%5d] ", buf, mob_index[zone_table[i].cmd[j].arg1].virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) in room ");
      else sprintf(buf, "%s(if< [%3d]) in room ", buf, zone_table[i].cmd[j].arg2);
      sprintf(buf, "%s[%5d].$R\r\n", buf, world[zone_table[i].cmd[j].arg3].number);
      break;
    case 'O':
      sprintf(buf, "%s $BLoad obj  [%5d] ", buf, obj_index[zone_table[i].cmd[j].arg1].virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) in room ");
      else sprintf(buf, "%s(if< [%3d]) in room ", buf, zone_table[i].cmd[j].arg2);
      sprintf(buf, "%s[%5d].$R\r\n", buf, world[zone_table[i].cmd[j].arg3].number);
      break;
    case 'P':
      sprintf(buf, "%s $5Place obj [%5d] ", buf, obj_index[zone_table[i].cmd[j].arg1].virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) in objt ");
      else sprintf(buf, "%s(if< [%3d]) in objt ", buf, zone_table[i].cmd[j].arg2);
      sprintf(buf, "%s[%5d] (in last created).$R\r\n", buf, obj_index[zone_table[i].cmd[j].arg3].virt);
      break;
    case 'G':
      sprintf(buf, "%s $6Place obj [%5d] ", buf, obj_index[zone_table[i].cmd[j].arg1].virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) on last mob loaded.$R\r\n");
      else sprintf(buf, "%s(if< [%3d]) on last mob loaded.$R\r\n", buf, zone_table[i].cmd[j].arg2);
      break;
    case 'E':
      sprintf(buf, "%s $2Equip obj [%5d] ", buf, obj_index[zone_table[i].cmd[j].arg1].virt);
      if(zone_table[i].cmd[j].arg2 == -1)
        strcat(buf, "(  always ) on last mob on ");
      else sprintf(buf, "%s(if< [%3d]) on last mob on ", buf, zone_table[i].cmd[j].arg2);
      if(zone_table[i].cmd[j].arg3 > WEAR_MAX ||
         zone_table[i].cmd[j].arg3 < 0)
         sprintf(buf, "%s[%d](InvalidArg3).$R\r\n", buf, zone_table[i].cmd[j].arg3);
      else sprintf(buf, "%s[%d](%s).$R\r\n", buf, zone_table[i].cmd[j].arg3,
           equipment_types[zone_table[i].cmd[j].arg3]);
      break;
    case 'D':
      sprintf(buf, "%s $3Room [%5d] Dir: [%d]", buf,
        world[zone_table[i].cmd[j].arg1].number,
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

  half_chop(argument, type, name);

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
      send_to_char("  wfiles\r\n"
                   "  mfiles\r\n"
                   "  ofiles\r\n", ch);
    return eFAILURE;
  }

  if(is_abbrev(type,"mobile")) 
  {
    if(!*name) {
       send_to_char("Format:  show mob <keyword>\r\n"
                    "                  <number>\r\n"
                    "                  <beginrange> <endrange>\r\n", ch);
       return eFAILURE;
    }

    if(isdigit(*name)) {
       half_chop(name, beginrange, endrange);
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
    if(!*buf)
      send_to_char("Couldn't find any MOBS by that NAME.\n\r", ch);
  } /* "mobile" */
  else if (is_abbrev(type,"object")) 
  {
    if(!*name) {
       send_to_char("Format:  show obj <keyword>\r\n"
                    "                  <number>\r\n"
                    "                  <beginrange> <endrange>\r\n", ch);
       return eFAILURE;
    }

    if(isdigit(*name)) {
       half_chop(name, beginrange, endrange);
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
           sprintf(buf, "[%3d] [%5d] %s\n\r", count, i, 
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
    if(!*name) {
       send_to_char("Format:  show room <beginrange> <endrange>\r\n", ch);
       return eFAILURE;
    }

    if(isdigit(*name)) {
       half_chop(name, beginrange, endrange);
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
            if(!world_array[i])
              continue;

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
        sprintf(buf, "%3d %s\r\n",
                   i, zone_table[i].name);
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
  else if (is_abbrev(type, "wfiles") && has_range)
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
    else if (str_cmp("all", buf)) {
        if (!(victim = get_char_vis(ch,buf)))
            send_to_char("No-one by that name around.\n\r",ch);
        else {
    if (world[ch->in_room].number == 25 && !isname(GET_NAME(ch), "Pirahna"))
      {
      send_to_char ("Damn! that is rude! This ain't your place. :P\n\r", ch);
      return eFAILURE;
      }
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
            send_to_char("Ok.\n\r",ch);
        }
    } else { /* Trans All */
    for (i = descriptor_list; i; i = i->next)
            if (i->character != ch && !i->connected) {
                victim = i->character;
                act("$n disappears in a mushroom cloud.",
                     victim, 0, 0, TO_ROOM, 0);
                target = ch->in_room;
		move_char(victim, target);
                act("$n arrives from a puff of smoke.",
                     victim, 0, 0, TO_ROOM, 0);
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
   extern int top_of_world;

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
//            target = (sh_int)loop;
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


