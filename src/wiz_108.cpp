/********************************
| Level 108 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"
#include "interp.h"
#include "utility.h"
#include "levels.h"
#include "mobile.h"
#include "player.h"
#include "handler.h"
#include "returnvals.h"
#include "spells.h"
#include <string>
#include "game_portal.h"
#include "const.h"

using namespace std;

int get_number(char **name);


int do_zoneexits(struct char_data *ch, char *argument, int cmd)
{
//  try
 // {
    char buf[MAX_STRING_LENGTH];
    string output = "";
    struct room_direction_data *curExits;
    int curZone = GET_ZONE(ch);
    int curRoom = ch->in_room;
    OBJ_DATA *portal;
    int i, dir;
    int low, high;
    int last_good = curRoom;

    if(!can_modify_room(ch, ch->in_room)) 
    {
      send_to_char("You are unable to do this outside of your range.\n\r", ch);
      return eFAILURE;
    }

    csendf(ch, "Searching Zone: %d - %s\r\n", curZone, zone_table[world[curRoom].zone].name);
    for(low = curRoom; low > 0; low--)
    {
      if(!world_array[low-1]) continue;
      last_good = low;
      if(world[low-1].zone != curZone) break;
    }
    low = last_good;
    last_good = curRoom;
    for(high = curRoom; high < top_of_world; high++)
    {
      if(!world_array[high+1]) continue;
      last_good = high;
      if(world[high+1].zone != curZone) break;
    }
    high = last_good;
    
    for(i = low; i < high; i++)
    {
      if(!world_array[i]) continue;
      for(dir = 0; dir < 6; dir++)
      {
        if ((curExits = world[i].dir_option[dir]) != 0)
        {
          if(curExits->to_room > 0 && world[curExits->to_room].zone != curZone)
          {
            sprintf(buf, "Room %5d - %5s to Room %5d, zone %3d (%s)\r\n",
                     i, dirs[dir], curExits->to_room, world[curExits->to_room].zone,
                     zone_table[world[curExits->to_room].zone].name);

            output += buf;
          }
        }
      }
      for(portal = world[i].contents; portal; portal = portal->next_content)
      {
        if(portal->obj_flags.type_flag == ITEM_CLIMBABLE) 
        {
          if(portal->obj_flags.value[0] < 0)
          {
            sprintf(buf, "Room %5d - climb to Room %5d (ERROR)\r\n",
                     i, real_room(portal->obj_flags.value[0]));

            output += buf;
          }
          else if(!world_array[portal->obj_flags.value[0]])
          {
            sprintf(buf, "Room %5d - climb to Room %5d (DOES NOT EXIST)\r\n",
                     i, real_room(portal->obj_flags.value[0]));

            output += buf;
          }
          else if (world[real_room(portal->obj_flags.value[0])].zone != curZone)
          {
            sprintf(buf, "Room %5d - climb to Room %5d, zone %3d (%s)\r\n",
                     i, real_room(portal->obj_flags.value[0]), 
                     world[real_room(portal->obj_flags.value[0])].zone,
                     zone_table[world[real_room(portal->obj_flags.value[0])].zone].name);

            output += buf;
          }
        }

        if (portal->obj_flags.type_flag == ITEM_PORTAL
            && !IS_SET(portal->obj_flags.value[3], PORTAL_NO_ENTER)
            && (portal->obj_flags.value[1] == 1 || portal->obj_flags.value[1] == 2)
           ) 
        {
          if (real_room(portal->obj_flags.value[0]) < 0) 
          {
            sprintf(buf, "Room %5d - enter to Room %5d (ERROR)\r\n",
                     i,real_room(portal->obj_flags.value[0]));

            output += buf;
          }
          else if (!world_array[real_room(portal->obj_flags.value[0])]) 
          {
            sprintf(buf, "Room %5d - enter to Room %5d (DOES NOT EXIST)\r\n",
                     i,real_room(portal->obj_flags.value[0]));

            output += buf;
          }
          else if (world[real_room(portal->obj_flags.value[0])].zone != curZone)
          {
            sprintf(buf, "Room %5d - enter to Room %5d, zone %3d (%s)\r\n",
                     i, real_room(portal->obj_flags.value[0]), 
                     world[real_room(portal->obj_flags.value[0])].zone,
                     zone_table[world[real_room(portal->obj_flags.value[0])].zone].name);

            output += buf;
          } 
        }
      }

      for(portal = object_list; portal; portal = portal->next) 
      {
        if((portal->obj_flags.type_flag == ITEM_PORTAL) 
            && (portal->obj_flags.value[1] == 1 || (portal->obj_flags.value[1] == 2)) 
            && (portal->in_room > -1) 
            && !IS_SET(portal->obj_flags.value[3], PORTAL_NO_LEAVE)) 
        {
          if((portal->obj_flags.value[0] == world[i].number) 
              ||(portal->obj_flags.value[2] == world[i].zone)) 
          {
            if(world[real_room(portal->in_room)].zone != curZone)
            {
              sprintf(buf, "Room %5d - leave to Room %5d, zone %3d (%s)\r\n",
                     i, real_room(portal->in_room), 
                     world[real_room(portal->in_room)].zone,
                     zone_table[world[real_room(portal->in_room)].zone].name);

              output += buf;
            }

          }
        }
      }
    }

  send_to_char(output.c_str(), ch);
 // }
 // catch(char *errmsg)
 // {
 //   csendf(ch, "Error encountered while finding zone exits:\r\n%s\r\n", errmsg);
 //   send_to_char("Ask Rubicon if it needs fixed...\r\n", ch);
 //   return eFAILURE;
  //}

  return eSUCCESS;
}


int do_purloin(struct char_data *ch, char *argument, int cmd)
{
   char bufName[200], *pBuf;
   struct obj_data *k;
   int j, nIndex = 0;

   if(!has_skill(ch, COMMAND_PURLOIN)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
   }

   one_argument(argument, bufName);
   
   // if the string is NULL, return.  Else assign pBuf to point to it.
   if (*(pBuf = bufName) == '\0') {
      send_to_char("Retrieves any item in the game and puts it in your inventory.\n\r"
        "Works well in combination with the 'find obj' command.\n\r"
        "Usage: purloin [number.]name\n\r", ch);
      return eFAILURE;
      }

   // pass pBuf's address to get_number.  It returns the index
   // specified by the player, or 1 (0 if an error).  It also
   // sets pBuf so it points to the name.
   if ( ( nIndex = get_number(&pBuf) ) == 0 ) {
      send_to_char("get_number failed.  bad input?\n\r", ch); 
      return eFAILURE;
      }

   for (k = object_list, j = 1; k && (j <= nIndex); k = k->next) {
      if (!(isname(pBuf, k->name)))
         continue;
      if (!CAN_SEE_OBJ(ch, k))
         continue;

      if(k->carried_by && !CAN_SEE(ch, k->carried_by)) {
         continue;
         }
      else if(k->equipped_by && !CAN_SEE(ch, k->equipped_by)) {
         continue;
         }
      else if (k->in_obj) {
         if (k->in_obj->carried_by && !CAN_SEE(ch, k->in_obj->carried_by)) {
            continue;
            }
         else if(k->in_obj->equipped_by && !CAN_SEE(ch, k->in_obj->equipped_by)) {
            continue;
            }
         }

      if (j == nIndex) {
         struct char_data * vict = NULL;
         if(k->carried_by) {
            vict = k->carried_by;
            }
         else if(k->equipped_by) {
            int iEq;
            vict = k->equipped_by;
            for (iEq = 0; iEq < MAX_WEAR; iEq++) {
                if (vict->equipment[iEq] == k) {
                   obj_to_char(unequip_char(vict, iEq), vict);
                   break;
                   }
                } // for
            }
         else if (k->in_obj) {
            if (k->in_obj->carried_by) {
               vict = k->in_obj->carried_by;
               }
            else if(k->in_obj->equipped_by) {
               vict = k->in_obj->equipped_by;
               }
            }
         if (vict != NULL) {
            csendf(ch, "You purloin %s from %s.\n\r",
                   k->short_description, GET_NAME(vict));
            logf (GET_LEVEL(ch), LOG_GOD, "%s purloins %s from %s",
              GET_NAME(ch), k->short_description, GET_NAME(vict));
            }
         else {
            csendf(ch, "You purloin %s.\n\r", k->short_description);
            }
         move_obj(k, ch);
         return eSUCCESS;
         }
      j++;
      }

   send_to_char("Sorry, couldn't find it or something.\n\r", ch);
   return eSUCCESS;
}

int do_set(struct char_data *ch, char *argument, int cmd)
{
//   renamed the command "setup" so don't need this anymore
//    void do_mortal_set(struct char_data *ch, char *argument, int cmd);
//    
//    if(GET_LEVEL(ch) < IMMORTAL || IS_NPC(ch)) {
//      do_mortal_set(ch, argument, cmd);
//      return;
//    }

    void add_to_bard_list(char_data * ch);

    /* from spell_parser.c */

    const char *values[] = {
        "age","sex","class","level","height","weight","str","stradd",
        "int","wis","dex","con","gold","exp","mana","hit","move",
        "sessions","alignment","thirst","drunk","full","race",
        "bank", "platinum", "ki", "clan", "saves_base", "hpmeta",
	"manameta","movemeta", "armor", "profession", "\n"
    };
    struct char_data *vict;
    char name[100], buf2[100], buf[100], help[MAX_STRING_LENGTH];
    int skill, value, i, x;

    if (IS_NPC(ch)) return eFAILURE;

    if(!has_skill(ch, COMMAND_SET)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
    }

    argument = one_argument(argument, name);
    if (!*name) // no arguments. print an informative text
    {
        send_to_char(
        "Usage:\n\rset <name> <field> <value>\n\r", ch);

        strcpy(help, "\n\rField being one of the following:\n\r");
        display_string_list(values, ch);
/*        for (i = 1; *values[i] != '\n'; i++)
        {
            sprintf(help + strlen(help), "%18s", values[i]);
            if (!(i % 4))
            {
                strcat(help, "\n\r");
                send_to_char(help, ch);
                *help = '\0';
            }
        }
        if (*help)
            send_to_char(help, ch);
        send_to_char("\n\r", ch);*/
        return eFAILURE;
    }
    if (!(vict = get_char_vis(ch, name)))
    {
        send_to_char("No living thing by that name.\n\r", ch);
        return eFAILURE;
    }

    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
        send_to_char("Get real! You ain't that big.\n\r",ch);
        if (!IS_NPC(vict))  {
           sprintf(buf2, "%s just tried to set: %s\n\r", GET_NAME(ch), buf);
           send_to_char(buf2, vict);
        }
        return eFAILURE;
    }

    if (!IS_NPC(vict) && (GET_LEVEL(vict) == IMP) && (GET_NAME(vict) != GET_NAME(ch))) {
        send_to_char("Forget it dweeb.\n\r",ch);
        return eFAILURE;
    }

    argument = one_argument(argument, buf);
    if (!*buf)
    {
                send_to_char("A field was expected.\n\r", ch);
                return eFAILURE;
    }

        skill = old_search_block(buf, 0, strlen(buf), values, 1);
        if(skill < 0)
        {
                send_to_char("That value not recognized.\n\r",ch);
                return eFAILURE;
        }
        argument = one_argument(argument, buf); /* update argument */
        skill--;
        sprintf(buf2,
                "%s sets %s's %s to %s.",
                GET_NAME(ch),GET_NAME(vict),values[skill],buf);
        switch (skill) {
            case 0: /* age */
            {
                if(IS_MOB(vict)) {
                   send_to_char("Can't set a mob's age.\r\n", ch);
                   return eFAILURE;
                }
                value = atoi(buf);
                log(buf2, IMP, LOG_GOD);
                /* set age of victim */
                vict->pcdata->time.birth = 
                    time(0) - (long)value*(long)SECS_PER_MUD_YEAR;
            };
            break;
            case 1: /* sex */
            {
                if (str_cmp(buf,"m") && str_cmp(buf,"f") && str_cmp(buf,"n"))
                {
                    send_to_char("Sex must be 'm','f' or 'n'.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set sex of victim */
                switch(*buf) {
                    case 'm':vict->sex = SEX_MALE;   break;
                    case 'f':vict->sex = SEX_FEMALE; break;
                    case 'n':vict->sex = SEX_NEUTRAL;break;
                }
            }
            break;
            case 2: /* class */
            {
                if (str_cmp(buf,"m") && str_cmp(buf,"c") && 
                    str_cmp(buf,"w") && str_cmp(buf,"t") &&
                    str_cmp(buf,"a") && str_cmp(buf,"p") &&
                    str_cmp(buf,"b") && str_cmp(buf,"k") &&
                    str_cmp(buf,"r") && str_cmp(buf,"d") &&
                    str_cmp(buf,"u"))


                {
                  send_to_char("Class must be 'm','c','w','t','a','p','b',"
                                "'r', 'k'(monk), 'd'(bard), or 'u'(druid). \n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set class of victim */
                switch(*buf) {
                    case 'm':vict->c_class = CLASS_MAGIC_USER; break;
                    case 'c':vict->c_class = CLASS_CLERIC;     break;
                    case 'w':vict->c_class = CLASS_WARRIOR;    break;
                    case 't':vict->c_class = CLASS_THIEF;      break;
                    case 'a':vict->c_class = CLASS_ANTI_PAL;   break;
                    case 'p':vict->c_class = CLASS_PALADIN;    break;
                    case 'b':vict->c_class = CLASS_BARBARIAN;  break;
                    case 'k':vict->c_class = CLASS_MONK;       break;
                    case 'r':vict->c_class = CLASS_RANGER;     break;
                    case 'd':vict->c_class = CLASS_BARD;       
                             add_to_bard_list(vict);           break;
                    case 'u':vict->c_class = CLASS_DRUID;      break; 
                }
            }
            break;
            case 3: /* level */
            {
                value = atoi(buf);
                if(value > MAX_MORTAL && value < MIN_GOD)
                {
                  send_to_char("That level doesn't exist!\n\r", ch);
                  return eFAILURE;
                }
                if(((value < 0) || (value > MAX_MORTAL)) && GET_LEVEL(ch) < OVERSEER) {
                    send_to_char(
                        "Level must be between 0 and 101.\n\r", ch );
                    return eFAILURE;
                }
                /* why the fuck was ths missing? -Sadus */
                if(!IS_NPC(vict) && value > GET_LEVEL(ch)) {
                  send_to_char("That level is higher than you!\n\r", ch);
                  return eFAILURE;
                } 
                log(buf2, IMP, LOG_GOD);
                /* set level of victim */
                vict->level = value;
                update_wizlist(vict);
            }
            break;
            case 4: /* height */
            {
                value = atoi(buf);
                log(buf2, IMP, LOG_GOD);
                /* set height of victim */
                vict->height = value;
            }       
            break;
            case 5: /* weight */
            {
                value = atoi(buf);
                log(buf2, IMP, LOG_GOD);
                /* set weight of victim */
                vict->weight = value;
            }
            break;
            case 6: /* str */
            {
                value = atoi(buf);
                if ((value <= 0) || (value > 30))
                {
                    send_to_char("Strength must be more than 0\n\r", ch);
                    send_to_char("and less than 26.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original strength of victim */
                vict->raw_str = value;
            }
            break;
            case 7: /* stradd */
            {
                send_to_char( "Strength addition not supported.\n\r", ch );
            }
            break;
            case 8: /* int */
            {
                value = atoi(buf);
                if ((value <= 0) || (value > 30))
                {
                    send_to_char("Intelligence must be more than 0\n\r", ch);
                    send_to_char("and less than 26.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original INT of victim */
                vict->raw_intel = value;
              redo_mana (vict);
              affect_total(vict);
            }
            break;
            case 9: /* wis */
            {
                value = atoi(buf);
                if ((value <= 0) || (value > 30))
                {
                    send_to_char("Wisdom must be more than 0\n\r", ch);
                    send_to_char("and less than 26.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original WIS of victim */
                vict->raw_wis = value;
            }
            break;
            case 10: /* dex */
            {
                value = atoi(buf);
                if ((value <= 0) || (value > 30))
                {
                    send_to_char("Dexterity must be more than 0\n\r", ch);
                    send_to_char("and less than 26.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original DEX of victim */
                vict->raw_dex = value;
            }
            break;
            case 11: /* con */
            {
                value = atoi(buf);
                if ((value <= 0) || (value > 30))
                {
                    send_to_char("Constitution must be more than 0\n\r", ch);
                    send_to_char("and less than 26.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original CON of victim */
                vict->raw_con = value;
                   redo_hitpoints(vict);
            }
            break;
            case 12: /* gold */
            {
                value = atoi(buf);
                log(buf2, IMP, LOG_GOD);
                /* set original gold of victim */
                vict->gold = value;
            }
            break;
            case 13: /* exp */
            {
              int64 val;
              val = atoll(buf);
              int64 before_exp = vict->exp;
              vict->exp = val;
              logf(GET_LEVEL(ch), LOG_GOD, "%s sets %s's exp from %lld to %lld.",
                   GET_NAME(ch), GET_NAME(vict), before_exp, vict->exp);
            }
            break;
            case 14: /* mana */
            {
                value = atoi(buf);
                log(buf2, IMP, LOG_GOD);
                /* set original mana of victim */
                vict->raw_mana = value;
                  redo_mana (vict);

            }
            break;
            case 15: /* hit */
            {
                value = atoi(buf);
                log(buf2, IMP, LOG_GOD);
                /* set original hit of victim */
                vict->raw_hit = value;
                redo_hitpoints(vict);

            }
            break;
            case 16: /* move */
            {
                value = atoi(buf);
                log(buf2, IMP, LOG_GOD);
                /* set original move of victim */
                vict->raw_move = value;
            }
            break;
            case 17: /* sessions */
            {
                if(IS_MOB(vict)) {
                  send_to_char("Can't set a mob's pracs...\r\n", ch);
                  return eFAILURE;
                }
                value = atoi(buf);
                log(buf2, IMP, LOG_GOD);
                /* set original sessions of victim */
                vict->pcdata->practices = value;
            }
            break;
            case 18: /* alignment */
            {
                value = atoi(buf);
                if ((value < -1000) || (value > 1000))
                {
                    send_to_char("Alignment must be more than -1000\n\r", ch);
                    send_to_char("and less than 1000.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original alignment of victim */
                vict->alignment = value;
            }
            break;
            case 19: /* thirst */
            {
                value = atoi(buf);
                if ((value < -1) || (value > 100))
                {
                    send_to_char("Thirst must be more than -2\n\r", ch);
                    send_to_char("and less than 101.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original thirst of victim */
                vict->conditions[THIRST] = value;
            }
            break;
            case 20: /* drunk */
            {
                value = atoi(buf);
                if ((value < -1) || (value > 100))
                {
                    send_to_char("Drunk must be more than -2\n\r", ch);
                    send_to_char("and less than 101.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original drunk of victim */
                vict->conditions[DRUNK] = value;
            }
            break;
            case 21: /* full */
            {
                value = atoi(buf);
                if ((value < -1) || (value > 100))
                {
                    send_to_char("Full must be more than -2\n\r", ch);
                    send_to_char("and less than 101.\n\r", ch);
                    return eFAILURE;
                }
                log(buf2, IMP, LOG_GOD);
                /* set original full of victim */
                vict->conditions[FULL] = value;
            }
            break;
            case 22: /* race */
            {
                for(x = 0; x <= 31; x++) {
                  if(x == 31) {
                    send_to_char("No such race.\n\r", ch);
                    return eFAILURE;
                  }
                  if(isname(race_info[x].singular_name, buf)) {
                    GET_RACE(vict) = x;
                    vict->immune = race_info[(int)GET_RACE(vict)].immune;
                    vict->suscept = race_info[(int)GET_RACE(vict)].suscept;
                    vict->resist = race_info[(int)GET_RACE(vict)].resist;
                    break;
                  }
                }
                log(buf2, IMP, LOG_GOD);
            }
            break;
            case 23:  /* bank */
            {
              GET_BANK(vict) = atoi(buf);
              log(buf2, GET_LEVEL(ch), LOG_GOD); 
            }
            break;
            case 24:  /* platinum */
            {
              if (GET_LEVEL(ch) == IMP)
              {
                uint32 before_plat = GET_PLATINUM(vict);
                GET_PLATINUM(vict) = atoi(buf);
                logf(IMP, LOG_GOD, "%s sets %s's platinum from %u to %u.",
                     GET_NAME(ch), GET_NAME(vict), before_plat, GET_PLATINUM(vict));
              }
            }
            break;
            case 25: /* ki */
            {
                vict->raw_ki = atoi(buf);
                log(buf2, GET_LEVEL(ch), LOG_GOD);
            }
            break;
            case 26: /* clan number */
            {
              vict->clan = atoi(buf);
              log(buf2, GET_LEVEL(ch), LOG_BUG);
            } 
            break;
            case 27: // saves 
            {
              one_argument(argument, buf2);
              if(!*buf || !*buf2) {
                send_to_char("Syntax: set <vict> saves <0-5> <num>\r\n", ch);
                return eFAILURE;
              }

              if(IS_NPC(vict)) {
                send_to_char("You cannot set saves_bases on mobs.\r\n", ch);
                return eFAILURE; 
              }

              if(!check_range_valid_and_convert(i, buf, 0, 5)) {
                send_to_char("Save type be from 0 to 5.\r\n", ch);
                return eFAILURE;
              }

              if(!check_range_valid_and_convert(value, buf2, 0, 5)) {
                send_to_char("Value must be from -10 to 100.\r\n", ch);
                return eFAILURE;
              }
              vict->pcdata->saves_mods[i] = value;
            }
            break;
	case 28:
	{
              GET_HP_METAS(vict) = atoi(buf);
              log(buf2, GET_LEVEL(ch), LOG_GOD); 
	}
        break;
	case 29:
	{
              GET_MANA_METAS(vict) = atoi(buf);
              log(buf2, GET_LEVEL(ch), LOG_GOD); 
	}
        break;
	case 30:
	{
              GET_MOVE_METAS(vict) = atoi(buf);
              log(buf2, GET_LEVEL(ch), LOG_GOD); 
	}
        break;
	case 31:
	{
	      vict->armor = atoi(buf);
              log(buf2, GET_LEVEL(ch), LOG_GOD); 
	}
	break;
	case 32:
	{
	      if(!*buf) {
                csendf(ch, "Syntax: set <vict> profession <0-%d>\r\n", MAX_PROFESSIONS);
                return eFAILURE;
              }

              if(IS_NPC(vict)) {
                send_to_char("You cannot set profession on mobs.\r\n", ch);
                return eFAILURE; 
              }

              if(!check_range_valid_and_convert(value, buf, 0, MAX_PROFESSIONS)) {
                csendf(ch, "Save type be from 0 to %d.\r\n", MAX_PROFESSIONS);
                return eFAILURE;
              }

	      vict->pcdata->profession = value;
	      log(buf2, GET_LEVEL(ch), LOG_GOD);
	}
	break;
	
        }
    
  send_to_char("Ok.\n\r", ch);
  affect_total(vict); 
  return eSUCCESS;
}


