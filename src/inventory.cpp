/************************************************************************
| Description:  This file contains implementation of inventory-management
|   commands: get, give, put, etc..
|
| Authors: DikuMUD, Pirahna, Staylor, Urizen, Rahz, Zaphod, Shane, Jhhudso, Heaven1 and others
*/
extern "C"
{
  #include <ctype.h>
}
#include <queue>
#include <fmt/format.h>

#include "connect.h"
#include "character.h"
#include "obj.h"
#include "mobile.h"
#include "room.h"
#include "structs.h"
#include "utility.h"
#include "player.h"
#include "levels.h"
#include "interp.h"
#include "handler.h"
#include "db.h"
#include "act.h"
#include <string.h>
#include "returnvals.h"
#include "spells.h"
#include "clan.h"
#include "arena.h"
#include "inventory.h"

/* extern variables */

extern CWorld world;
extern struct index_data *obj_index; 
extern struct index_data *mob_index;
extern struct obj_data *object_list;
extern int rev_dir[];

/* extern functions */
void save_corpses(void);
char *fname(char *namelist);
struct obj_data *create_money( int amount );
int palm  (struct char_data *ch, struct obj_data *obj_object, struct obj_data *sub_object, bool has_consent);
void special_log(char *arg);
struct obj_data * bring_type_to_front(char_data * ch, int item_type);
bool search_container_for_item(obj_data * obj, int item_number);
bool search_container_for_vnum(obj_data * obj, int vnum);

/* procedures related to get */
void get(struct char_data *ch, struct obj_data *obj_object, struct obj_data *sub_object, bool has_consent, int cmd)
{
    string buffer;
   
    if(!sub_object || sub_object->carried_by != ch) 
    {
     if (IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && IS_NPC(ch)) {
	  send_to_char("You cannot get that item.\r\n",ch);
	  return;
	}
      // we only have to check for uniqueness if the container is not on the character
      // or if there is no container
      if(IS_SET(obj_object->obj_flags.more_flags, ITEM_UNIQUE)) {
        if(search_char_for_item(ch, obj_object->item_number, false)) {
           send_to_char("The item's uniqueness prevents it!\r\n", ch);
           return;
        }
      }
      if(contents_cause_unique_problem(obj_object, ch)) {
         send_to_char("Something inside the item is unique and prevents it!\r\n", ch);
         return;
      }
    }

    if( ( IS_NPC(ch) || affected_by_spell(ch, OBJ_CHAMPFLAG_TIMER) )
        && obj_index[obj_object->item_number].virt == CHAMPION_ITEM) {
       send_to_char("No champion flag for you, two years!\r\n", ch);
       return;
    }

  if (sub_object)
  {
    buffer = fmt::format("{}_consent", GET_NAME(ch));
    if (has_consent && obj_object->obj_flags.type_flag != ITEM_MONEY)
    {
      if ((cmd == CMD_LOOT && isname("lootable", sub_object->name)) && !isname(buffer, sub_object->name))
      {
        SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);;
        struct affected_type pthiefaf;
        WAIT_STATE(ch, PULSE_VIOLENCE*2);

        char log_buf[MAX_STRING_LENGTH];
        sprintf(log_buf, "%s looted %s[%d] from %s", GET_NAME(ch), obj_object->short_description, obj_index[obj_object->item_number].virt, sub_object->name);
        log(log_buf, ANGEL, LOG_MORTAL);

        send_to_char("You suddenly feel very guilty...shame on you stealing from the dead!\r\n", ch);

        pthiefaf.type = FUCK_PTHIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = 0;
        pthiefaf.location = APPLY_NONE;
        pthiefaf.bitvector = -1;

        if (affected_by_spell(ch, FUCK_PTHIEF))
        {
          affect_from_char(ch, FUCK_PTHIEF);
          affect_to_char(ch, &pthiefaf);
        } else
          affect_to_char(ch, &pthiefaf);

      }
    } else if (has_consent && obj_object->obj_flags.type_flag == ITEM_MONEY && !isname(buffer, sub_object->name))
    {
      if (cmd == CMD_LOOT && isname("lootable", sub_object->name))
      {
        struct affected_type pthiefaf;

        pthiefaf.type = FUCK_GTHIEF;
        pthiefaf.duration = 10;
        pthiefaf.modifier = 0;
        pthiefaf.location = APPLY_NONE;
        pthiefaf.bitvector = -1;
        WAIT_STATE(ch, PULSE_VIOLENCE);
        send_to_char("You suddenly feel very guilty...shame on you stealing from the dead!\r\n", ch);

        char log_buf[MAX_STRING_LENGTH];
        sprintf(log_buf, "%s looted %d coins from %s", GET_NAME(ch), obj_object->obj_flags.value[0], sub_object->name);
        log(log_buf, ANGEL, LOG_MORTAL);

        if (affected_by_spell(ch, FUCK_GTHIEF))
        {
          affect_from_char(ch, FUCK_GTHIEF);
          affect_to_char(ch, &pthiefaf);
        } else
          affect_to_char(ch, &pthiefaf);

      }
    }

        if (sub_object->in_room && obj_object->obj_flags.type_flag != ITEM_MONEY && sub_object->carried_by != ch)
	{ // Logging gold gets from corpses would just be too much.
  	    sprintf(log_buf, "%s gets %s[%d] from %s[%d]", 
		    GET_NAME(ch),
		    obj_object->name,
		    obj_index[obj_object->item_number].virt,
		    sub_object->name,
		    obj_index[sub_object->item_number].virt);
	    log(log_buf, 110, LOG_OBJECTS);
 	    for(OBJ_DATA *loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
              logf(IMP, LOG_OBJECTS, "The %s[%d] contained %s[%d]",
                          obj_object->short_description,
		          obj_index[obj_object->item_number].virt,
                          loop_obj->short_description,
                          obj_index[loop_obj->item_number].virt);
	}
	move_obj(obj_object, ch);
	if (sub_object->carried_by == ch) {
	    act("You get $p from $P.", ch, obj_object, sub_object, TO_CHAR, 0);
	    act("$n gets $p from $s $P.", ch, obj_object, sub_object, TO_ROOM, INVIS_NULL);
	} else {
	    act("You get $p from $P.",  ch, obj_object, sub_object, TO_CHAR, INVIS_NULL);
	    act("$n gets $p from $P.",  ch, obj_object, sub_object, TO_ROOM,
	      INVIS_NULL);
	}
    } else {
	move_obj(obj_object, ch);
	act("You get $p.", ch, obj_object, 0, TO_CHAR, 0);
	act("$n gets $p.", ch, obj_object, 0, TO_ROOM, INVIS_NULL);
        if (obj_object->obj_flags.type_flag != ITEM_MONEY)
        {
            sprintf(log_buf, "%s gets %s[%d] from room %d", GET_NAME(ch), obj_object->name, obj_index[obj_object->item_number].virt,
                ch->in_room);
            log(log_buf, IMP, LOG_OBJECTS);
            for(OBJ_DATA *loop_obj = obj_object->contains; loop_obj; loop_obj = loop_obj->next_content)
              logf(IMP, LOG_OBJECTS, "The %s contained %s[%d]",
                          obj_object->short_description,
                          loop_obj->short_description,
                          obj_index[loop_obj->item_number].virt);
        }

        if(obj_index[obj_object->item_number].virt == CHAMPION_ITEM) {
           SETBIT(ch->affected_by, AFF_CHAMPION);
           buffer = fmt::format("\r\n##{} has just picked up the Champion flag!\r\n", GET_NAME(ch));
           send_info(buffer);
        }
    }
    if (sub_object && sub_object->obj_flags.value[3] == 1 && 
           isname("pc",sub_object->name))
        do_save(ch, "", 666);


    if((obj_object->obj_flags.type_flag == ITEM_MONEY) && 
	(obj_object->obj_flags.value[0]>=1)) {
	obj_from_char(obj_object);

	buffer = fmt::format("There was {} coins.",
		obj_object->obj_flags.value[0]);
        if(IS_MOB(ch) || !IS_SET(ch->pcdata->toggles, PLR_BRIEF))
	{
	  send_to_char(buffer, ch);
	  send_to_char("\r\n",ch);
	}
	extern zone_data *zone_table;
	bool tax = FALSE;

        if (zone_table[world[ch->in_room].zone].clanowner > 0 && ch->clan != 
		zone_table[world[ch->in_room].zone].clanowner)
	{
	         int cgold = (int)((float)(obj_object->obj_flags.value[0]) * 0.1);
		 obj_object->obj_flags.value[0] -= cgold;
		zone_table[world[ch->in_room].zone].gold += cgold;
	if(!IS_MOB(ch) && IS_SET(ch->pcdata->toggles, PLR_BRIEF))
	{
		tax = TRUE;
		buffer = fmt::format("{} Bounty: {}", buffer, cgold);
		zone_table[world[ch->in_room].zone].gold += cgold;
	}
	else csendf(ch, "Clan %s collects %d bounty, leaving %d for you.\r\n",get_clan(zone_table[world[ch->in_room].zone].clanowner)->name,cgold,
			obj_object->obj_flags.value[0]);
	}
//	if (sub_object && sub_object->obj_flags.value[3] == 1 && 
//           !isname("pc",sub_object->name) && ch->clan 
//            && get_clan(ch)->tax && !IS_SET(GET_TOGGLES(ch), PLR_NOTAX))
	if (((sub_object && sub_object->obj_flags.value[3] == 1 && 
           !isname("pc",sub_object->name)) || !sub_object) && ch->clan 
            && get_clan(ch)->tax && !IS_SET(GET_TOGGLES(ch), PLR_NOTAX))
	{
	  int cgold = (int)((float)(obj_object->obj_flags.value[0]) * (float)((float)(get_clan(ch)->tax)/100.0));
	  obj_object->obj_flags.value[0] -= cgold;
	  GET_GOLD(ch) += obj_object->obj_flags.value[0];
          get_clan(ch)->cdeposit(cgold);
	if(!IS_MOB(ch) && IS_SET(ch->pcdata->toggles, PLR_BRIEF))
        {
		tax = TRUE;
		buffer = fmt::format("{} ClanTax: {}", buffer, cgold);
        }
	else {
	  csendf(ch,"Your clan taxes you %d gold, leaving %d gold for you.\r\n",cgold, obj_object->obj_flags.value[0]);
			}
	  save_clans();
	} else
	GET_GOLD(ch) += obj_object->obj_flags.value[0];

  // If a mob gets gold, we disable its ability to receive a gold bonus. This keeps
  // the mob from turning into an interest bearing savings account. :)
  if (IS_NPC(ch)) {
    SETBIT(ch->mobdata->actflags, ACT_NO_GOLD_BONUS);
  }

    if (tax)
    {
      buffer = fmt::format("{}. {} gold remaining.\r\n", buffer, obj_object->obj_flags.value[0]);
    } else
    {
      buffer = fmt::format("{}\r\n", buffer);
    }

        if(!IS_MOB(ch) && IS_SET(ch->pcdata->toggles, PLR_BRIEF))
  	  send_to_char(buffer,ch);
	extract_obj(obj_object);
    }

    save_corpses();
}

// return eSUCCESS if item was picked up
// return eFAILURE if not
// TODO - currently this is designed with icky if-logic.  While it allows you to put
// code at the end that would affect any attempted 'get' it looks really nasty and
// is never utilized.  Restructure it so it is clear.  Pay proper attention to 'saving'
// however so as not to introduce a potential dupe-bug.
int do_get(struct char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  struct obj_data *sub_object;
  struct obj_data *obj_object;
  struct obj_data *next_obj;
  bool found = FALSE;
  bool fail = FALSE;
  bool has_consent = FALSE;
  int type = 3;
  bool alldot = FALSE;
  bool inventorycontainer = FALSE, blindlag = FALSE;
  char allbuf[MAX_STRING_LENGTH];

  argument_interpreter(argument, arg1, arg2);

  /* get type */
  if (!*arg1)
  {
    type = 0;
  }
  if (*arg1 && !*arg2)
  {
    alldot = FALSE;
    allbuf[0] = '\0';
    if ((str_cmp(arg1, "all") != 0) &&
        (sscanf(arg1, "all.%s", allbuf) != 0))
    {
      strcpy(arg1, "all");
      alldot = TRUE;
    }
    if (!str_cmp(arg1, "all"))
    {
      type = 1;
    }
    else
    {
      type = 2;
    }
  }
  if (*arg1 && *arg2)
  {
    alldot = FALSE;
    allbuf[0] = '\0';
    if ((str_cmp(arg1, "all") != 0) &&
        (sscanf(arg1, "all.%s", allbuf) != 0))
    {
      strcpy(arg1, "all");
      alldot = TRUE;
    }
    if (!str_cmp(arg1, "all"))
    {
      if (!str_cmp(arg2, "all"))
      {
        type = 3;
      }
      else
      {
        type = 4;
      }
    }
    else
    {
      if (!str_cmp(arg2, "all"))
      {
        type = 5;
      }
      else
      {
        type = 6;
      }
    }
  }

  if ((cmd == 10) && (GET_CLASS(ch) != CLASS_THIEF) &&
      (GET_LEVEL(ch) < IMMORTAL))
  {
    send_to_char("I bet you think you're a thief.\r\n", ch);
    return eFAILURE;
  }

  if (cmd == 10 && type != 2 && type != 6 && type != 0)
  {
    send_to_char("You can only palm objects that are in the same room, "
                 "one at a time.\r\n",
                 ch);
    return eFAILURE;
  }

  if (cmd == CMD_LOOT && type != 0 && type != 6)
  {
    send_to_char("You can only loot 1 item from a non-consented corpse.\r\n", ch);
    return eFAILURE;
  }

  switch (type)
  {
  /* get */
  case 0:
  {
    switch (cmd)
    {
    case 10:
      send_to_char("Palm what?\r\n", ch);
      break;
    case CMD_LOOT:
      send_to_char("Loot what?\r\n", ch);
      break;
    default:
      send_to_char("Get what?\r\n", ch);
    }
  }
  break;
  /* get all */
  case 1:
  {
    if (ch->in_room == real_room(3099))
    {
      send_to_char("Not in the donation room.\r\n", ch);
      return eFAILURE;
    }
    sub_object = 0;
    found = FALSE;
    fail = FALSE;
    for (obj_object = world[ch->in_room].contents;
         obj_object;
         obj_object = next_obj)
    {
      next_obj = obj_object->next_content;

      /* IF all.obj, only get those named "obj" */
      if (alldot && !isname(allbuf, obj_object->name))
        continue;

      // Can't pick up NO_NOTICE items with 'get all'  only 'all.X' or 'X'
      if (!alldot && IS_SET(obj_object->obj_flags.more_flags, ITEM_NONOTICE) && GET_LEVEL(ch) < IMMORTAL)
        continue;

      // Ignore NO_TRADE items on a 'get all'
      if (IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && GET_LEVEL(ch) < IMMORTAL)
      {
        csendf(ch, "The %s appears to be NO_TRADE so you don't pick it up.\r\n", obj_object->short_description);
        continue;
      }
      if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
          obj_object->obj_flags.value[0] > 10000 &&
          GET_LEVEL(ch) < 5)
      {
        send_to_char("You cannot pick up that much money!\r\n", ch);
        continue;
      }

      if (obj_object->obj_flags.eq_level > 9 && GET_LEVEL(ch) < 5)
      {
        csendf(ch, "%s is too powerful for you to possess.\r\n", obj_object->short_description);
        continue;
      }

      if (IS_SET(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
          !isname(GET_NAME(ch), obj_object->name) && GET_LEVEL(ch) < IMP)
      {
        csendf(ch, "The %s appears to be SPECIAL. Only its rightful owner can take it.\r\n", obj_object->short_description);
        continue;
      }

      // PC corpse
      if ((obj_object->obj_flags.value[3] == 1 && isname("pc", obj_object->name)) || isname("thiefcorpse", obj_object->name))
      {
        sprintf(buffer, "%s_consent", GET_NAME(ch));
        if ((isname("thiefcorpse", obj_object->name) &&
             !isname(GET_NAME(ch), obj_object->name)) ||
            isname(GET_NAME(ch), obj_object->name) || GET_LEVEL(ch) >= OVERSEER)
          has_consent = TRUE;
        if (!has_consent && !isname(GET_NAME(ch), obj_object->name))
        {
          if (GET_LEVEL(ch) < OVERSEER)
          {
            send_to_char("You don't have consent to take the corpse.\r\n", ch);
            continue;
          }
        }
        if (has_consent && contains_no_trade_item(obj_object))
        {
          if (GET_LEVEL(ch) < OVERSEER)
          {
            send_to_char("This item contains no_trade items that cannot be picked up.\r\n", ch);
            has_consent = FALSE; // bugfix, could loot without consent
            continue;
          }
        }
        if (GET_LEVEL(ch) < OVERSEER)
          has_consent = FALSE; // reset it for the next item:P
        else
          has_consent = TRUE; // reset it for the next item:P
      }

      if (CAN_SEE_OBJ(ch, obj_object) && GET_LEVEL(ch) < IMMORTAL)
      {
        // Don't bother checking this if item is gold coins.
        if ((IS_CARRYING_N(ch) + 1) > CAN_CARRY_N(ch) &&
            !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && GET_LEVEL(ch) < IMMORTAL))
        {
          sprintf(buffer, "%s : You can't carry that many items.\r\n", fname(obj_object->name));
          send_to_char(buffer, ch);
          fail = TRUE;
        }
        else if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) > CAN_CARRY_W(ch) && GET_LEVEL(ch) < IMMORTAL && GET_ITEM_TYPE(obj_object) != ITEM_MONEY)
        {
          sprintf(buffer, "%s : You can't carry that much weight.\r\n", fname(obj_object->name));
          send_to_char(buffer, ch);
          fail = TRUE;
        }
        else if (CAN_WEAR(obj_object, ITEM_TAKE))
        {
          get(ch, obj_object, sub_object, 0, cmd);
          found = TRUE;
        }
        else
        {
          send_to_char("You can't take that.\r\n", ch);
          fail = TRUE;
        }
      }
      else if (CAN_SEE_OBJ(ch, obj_object) && GET_LEVEL(ch) >= IMMORTAL && CAN_WEAR(obj_object, ITEM_TAKE))
      {
        get(ch, obj_object, sub_object, 0, cmd);
        found = TRUE;
      }
    } // of for loop
    if (found)
    {
      //		send_to_char("OK.\r\n", ch);
      do_save(ch, "", 666);
    }
    else
    {
      if (!fail)
        send_to_char("You see nothing here.\r\n", ch);
    }
  }
  break;
  /* get ??? */
  case 2:
  {
    sub_object = 0;
    found = FALSE;
    fail = FALSE;
    obj_object = get_obj_in_list_vis(ch, arg1,
                                     world[ch->in_room].contents);
    if (obj_object)
    {
      if (obj_object->obj_flags.type_flag == ITEM_CONTAINER &&
          obj_object->obj_flags.value[3] == 1 &&
          isname("pc", obj_object->name))
      {
        sprintf(buffer, "%s_consent", GET_NAME(ch));
        if (isname(GET_NAME(ch), obj_object->name))
          has_consent = TRUE;
        if (!has_consent && !isname(GET_NAME(ch), obj_object->name))
        {
          send_to_char("You don't have consent to take the "
                       "corpse.\r\n",
                       ch);
          return eFAILURE;
        }
        has_consent = FALSE; // reset it
      }

      if (IS_SET(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
          !isname(GET_NAME(ch), obj_object->name) && GET_LEVEL(ch) < IMP)
      {
        csendf(ch, "The %s appears to be SPECIAL. Only its rightful owner can take it.\r\n", obj_object->short_description);
      }
      else if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
               !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && GET_LEVEL(ch) < IMMORTAL))
      {
        sprintf(buffer, "%s : You can't carry that many items.\r\n", fname(obj_object->name));
        send_to_char(buffer, ch);
        fail = TRUE;
      }
      else if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) > CAN_CARRY_W(ch) &&
               GET_LEVEL(ch) < IMMORTAL && GET_ITEM_TYPE(obj_object) != ITEM_MONEY)
      {
        sprintf(buffer, "%s : You can't carry that much weight.\r\n", fname(obj_object->name));
        send_to_char(buffer, ch);
        fail = TRUE;
      }
      else if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
               obj_object->obj_flags.value[0] > 10000 &&
               GET_LEVEL(ch) < 5)
      {
        send_to_char("You cannot pick up that much money!\r\n", ch);
        fail = TRUE;
      }

      else if (obj_object->obj_flags.eq_level > 19 && GET_LEVEL(ch) < 5)
      {
        if (ch->in_room != real_room(3099))
        {
          csendf(ch, "%s is too powerful for you to possess.\r\n", obj_object->short_description);
          fail = TRUE;
        }
        else
        {
          csendf(ch, "The aura of the donation room allows you to pick up %s.\r\n", obj_object->short_description);
          get(ch, obj_object, sub_object, 0, cmd);
          do_save(ch, "", 666);
          found = TRUE;
        }
      }
      else if (CAN_WEAR(obj_object, ITEM_TAKE))
      {
        if (cmd == 10)
          palm(ch, obj_object, sub_object, 0);
        else
          get(ch, obj_object, sub_object, 0, cmd);

        do_save(ch, "", 666);
        found = TRUE;
      }
      else
      {
        send_to_char("You can't take that.\r\n", ch);
        fail = TRUE;
      }
    }
    else
    {
      sprintf(buffer, "You do not see a %s here.\r\n", arg1);
      send_to_char(buffer, ch);
      fail = TRUE;
    }
  }
  break;
  /* get all all */
  case 3:
  {
    send_to_char("You must be joking?!\r\n", ch);
  }
  break;
  /* get all ??? */
  case 4:
  {
    found = FALSE;
    fail = FALSE;
    sub_object = get_obj_in_list_vis(ch, arg2,
                                     world[ch->in_room].contents);
    if (!sub_object)
    {
      sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying);
      inventorycontainer = TRUE;
    }

    if (sub_object)
    {
      if (sub_object->obj_flags.type_flag == ITEM_CONTAINER &&
          ((sub_object->obj_flags.value[3] == 1 &&
            isname("pc", sub_object->name)) ||
           isname("thiefcorpse", sub_object->name)))
      {
        sprintf(buffer, "%s_consent", GET_NAME(ch));
        if ((isname("thiefcorpse", sub_object->name) && !isname(GET_NAME(ch), sub_object->name)) || isname(buffer, sub_object->name) || GET_LEVEL(ch) > 105)
          has_consent = TRUE;
        if (!has_consent && !isname(GET_NAME(ch), sub_object->name))
        {
          send_to_char("You don't have consent to touch the corpse.\r\n", ch);
          return eFAILURE;
        }
      }
      if (ARE_CONTAINERS(sub_object))
      {
        if (IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED))
        {
          sprintf(buffer, "The %s is closed.\r\n", fname(sub_object->name));
          send_to_char(buffer, ch);
          return eFAILURE;
        }
        for (obj_object = sub_object->contains;
             obj_object;
             obj_object = next_obj)
        {
          next_obj = obj_object->next_content;
          if (GET_ITEM_TYPE(obj_object) == ITEM_CONTAINER && contains_no_trade_item(obj_object))
          {
            csendf(ch, "%s : It seems magically attached to the corpse.\r\n",
                   fname(obj_object->name));
            continue;
          } /*
		   struct obj_data *temp,*next_contentthing;
		   for (temp = obj_object->contains;temp;temp = next_contentthing)
		   {
			next_contentthing = temp->next_content;
			if(IS_SET(temp->obj_flags.more_flags, ITEM_NO_TRADE))
			{
			csendf(ch, "Whoa!  The %s inside the %s poofed into thin air!\r\n", temp->short_description,obj_object->short_description);
			extract_obj(temp);
			}
		   }*/
            //		}
          /* IF all.obj, only get those named "obj" */
          if (alldot && !isname(allbuf, obj_object->name))
          {
            continue;
          }

          // Ignore NO_TRADE items on a 'get all'
          if (IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && GET_LEVEL(ch) < 100)
          {
            csendf(ch, "The %s appears to be NO_TRADE so you don't pick it up.\r\n", obj_object->short_description);
            continue;
          }

          if (IS_SET(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
              !isname(GET_NAME(ch), obj_object->name) && GET_LEVEL(ch) < IMP)
          {
            csendf(ch, "The %s appears to be SPECIAL. Only its rightful owner can take it.\r\n", obj_object->short_description);
            continue;
          }

          if (CAN_SEE_OBJ(ch, obj_object))
          {
            if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && GET_LEVEL(ch) < IMMORTAL))
            {
              sprintf(buffer, "%s : You can't carry that many items.\r\n", fname(obj_object->name));
              send_to_char(buffer, ch);
              fail = TRUE;
            }
            else
            {
              if (inventorycontainer ||
                  (IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < CAN_CARRY_W(ch) ||
                  GET_LEVEL(ch) > IMMORTAL || GET_ITEM_TYPE(obj_object) == ITEM_MONEY)
              {
                if (has_consent && IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE))
                {
                  // if I have consent and i'm touching the corpse, then I shouldn't be able
                  // to pick up no_trade items because it is someone else's corpse.  If I am
                  // the other of the corpse, has_consent will be false.
                  if (GET_LEVEL(ch) < IMMORTAL)
                  {
                    if (isname(obj_object->name, "thiefcorpse"))
                    {
                      csendf(ch, "Whoa!  The %s poofed into thin air!\r\n", obj_object->short_description);
                      extract_obj(obj_object);
                      continue;
                    }
                    csendf(ch, "%s : It seems magically attached to the corpse.\r\n", fname(obj_object->name));
                    continue;
                  }
                }
                if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
                    obj_object->obj_flags.value[0] > 10000 &&
                    GET_LEVEL(ch) < 5)
                {
                  send_to_char("You cannot pick up that much money!\r\n", ch);
                  continue;
                }

                if (sub_object->carried_by != ch && obj_object->obj_flags.eq_level > 9 && GET_LEVEL(ch) < 5)
                {
                  csendf(ch, "%s is too powerful for you to possess.\r\n", obj_object->short_description);
                  continue;
                }

                if (CAN_WEAR(obj_object, ITEM_TAKE))
                {
                  get(ch, obj_object, sub_object, 0, cmd);
                  found = TRUE;
                }
                else
                {
                  send_to_char("You can't take that.\r\n", ch);
                  fail = TRUE;
                }
              }
              else
              {
                sprintf(buffer,
                        "%s : You can't carry that much weight.\r\n",
                        fname(obj_object->name));
                send_to_char(buffer, ch);
                fail = TRUE;
              }
            }
          }
        }
        if (!found && !fail)
        {
          sprintf(buffer, "You do not see anything in the %s.\r\n",
                  fname(sub_object->name));
          send_to_char(buffer, ch);
          fail = TRUE;
        }
      }
      else
      {
        sprintf(buffer, "The %s is not a container.\r\n",
                fname(sub_object->name));
        send_to_char(buffer, ch);
        fail = TRUE;
      }
    }
    else
    {
      sprintf(buffer, "You do not see or have the %s.\r\n", arg2);
      send_to_char(buffer, ch);
      fail = TRUE;
    }
  }
  break;
  case 5:
  {
    send_to_char(
        "You can't take a thing from more than one container.\r\n", ch);
  }
  break;
  case 6:
  { // get ??? ???
    found = FALSE;
    fail = FALSE;
    sub_object = get_obj_in_list_vis(ch, arg2,
                                     world[ch->in_room].contents);
    if (!sub_object)
    {
      if (cmd == CMD_LOOT)
      {
        send_to_char("You can only loot 1 item from a non-consented corpse.\r\n", ch);
        return eFAILURE;
      }

      sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying, TRUE);
      inventorycontainer = TRUE;
    }
    if (sub_object)
    {
      if (sub_object->obj_flags.type_flag == ITEM_CONTAINER &&
          ((sub_object->obj_flags.value[3] == 1 &&
            isname("pc", sub_object->name)) ||
           isname("thiefcorpse", sub_object->name)))
      {
        sprintf(buffer, "%s_consent", GET_NAME(ch));

        if ((cmd != CMD_LOOT && (isname("thiefcorpse", sub_object->name) && !isname(GET_NAME(ch), sub_object->name))) || isname(buffer, sub_object->name))
          has_consent = TRUE;
        if (!isname(GET_NAME(ch), sub_object->name) && (cmd == CMD_LOOT && isname("lootable", sub_object->name)) && !IS_SET(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED) && !IS_SET(world[ch->in_room].room_flags, SAFE) && GET_LEVEL(ch) >= 50)
          has_consent = TRUE;
        if (!has_consent && !isname(GET_NAME(ch), sub_object->name))
        {
          send_to_char("You don't have consent to touch the "
                       "corpse.\r\n",
                       ch);
          return eFAILURE;
        }
      }
      if (ARE_CONTAINERS(sub_object))
      {
        if (IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED))
        {
          sprintf(buffer, "The %s is closed.\r\n", fname(sub_object->name));
          send_to_char(buffer, ch);
          return eFAILURE;
        }
        obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains);
        if (!obj_object && IS_AFFECTED(ch, AFF_BLIND) && has_skill(ch, SKILL_BLINDFIGHTING))
        {
          obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains, TRUE);
          blindlag = TRUE;
        }
        if (obj_object)
        {
          if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
              obj_object->obj_flags.value[0] > 10000 &&
              GET_LEVEL(ch) < 5)
          {
            send_to_char("You cannot pick up that much money!\r\n", ch);
            fail = TRUE;
          }

          else if (sub_object->carried_by != ch && obj_object->obj_flags.eq_level > 9 && GET_LEVEL(ch) < 5)
          {
            csendf(ch, "%s is too powerful for you to possess.\r\n", obj_object->short_description);
            fail = TRUE;
          }

          else if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                   !(GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && GET_LEVEL(ch) < IMMORTAL))
          {
            sprintf(buffer, "%s : You can't carry that many items.\r\n", fname(obj_object->name));
            send_to_char(buffer, ch);
            fail = TRUE;
          }
          else if (inventorycontainer ||
                   (IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < CAN_CARRY_W(ch) || GET_ITEM_TYPE(obj_object) == ITEM_MONEY)
          {
            if (has_consent && (IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) ||
                                contains_no_trade_item(obj_object)))
            {
              // if I have consent and i'm touching the corpse, then I shouldn't be able
              // to pick up no_trade items because it is someone else's corpse.  If I am
              // the other of the corpse, has_consent will be false.
              if (GET_LEVEL(ch) < IMMORTAL)
              {
                if (isname("thiefcorpse", sub_object->name) || (cmd == CMD_LOOT && isname("lootable", sub_object->name)))
                {
                  csendf(ch, "Whoa!  The %s poofed into thin air!\r\n", obj_object->short_description);

                  char log_buf[MAX_STRING_LENGTH];
                  sprintf(log_buf, "%s poofed %s[%d] from %s[%d]",
                          GET_NAME(ch),
                          obj_object->short_description,
                          obj_index[obj_object->item_number].virt,
                          sub_object->name,
                          obj_index[sub_object->item_number].virt);
                  log(log_buf, ANGEL, LOG_MORTAL);

                  extract_obj(obj_object);
                  fail = TRUE;
                  sprintf(buffer, "%s_consent", GET_NAME(ch));

                  if ((cmd == CMD_LOOT && isname("lootable", sub_object->name)) && !isname(buffer, sub_object->name))
                  {
                    SET_BIT(sub_object->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED);
                    struct affected_type pthiefaf;

                    pthiefaf.type = FUCK_PTHIEF;
                    pthiefaf.duration = 10;
                    pthiefaf.modifier = 0;
                    pthiefaf.location = APPLY_NONE;
                    pthiefaf.bitvector = -1;

                    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                    send_to_char("You suddenly feel very guilty...shame on you stealing from the dead!\r\n", ch);
                    if (affected_by_spell(ch, FUCK_PTHIEF))
                    {
                      affect_from_char(ch, FUCK_PTHIEF);
                      affect_to_char(ch, &pthiefaf);
                    }
                    else
                      affect_to_char(ch, &pthiefaf);
                  }
                }
                else
                {
                  csendf(ch, "%s : It seems magically attached to the corpse.\r\n", fname(obj_object->name));
                  fail = TRUE;
                }
              }
            }
            else if (CAN_WEAR(obj_object, ITEM_TAKE))
            {
              if (cmd == 10)
                palm(ch, obj_object, sub_object, has_consent);
              else
                get(ch, obj_object, sub_object, has_consent, cmd);
              found = TRUE;
              if (blindlag)
                WAIT_STATE(ch, PULSE_VIOLENCE);
            }
            else
            {
              send_to_char("You can't take that.\r\n", ch);
              fail = TRUE;
            }
          }
          else
          {
            sprintf(buffer, "%s : You can't carry that much weight.\r\n", fname(obj_object->name));
            send_to_char(buffer, ch);
            fail = TRUE;
          }
        }
        else
        {
          sprintf(buffer, "The %s does not contain the %s.\r\n",
                  fname(sub_object->name), arg1);
          send_to_char(buffer, ch);
          fail = TRUE;
        }
      }
      else
      {
        sprintf(buffer,
                "The %s is not a container.\r\n", fname(sub_object->name));
        send_to_char(buffer, ch);
        fail = TRUE;
      }
    }
    else
    {
      sprintf(buffer, "You do not see or have the %s.\r\n", arg2);
      send_to_char(buffer, ch);
      fail = TRUE;
    }
  }
  break;
  }
  if (fail)
    return eFAILURE;
  do_save(ch, "", 666);
  return eSUCCESS;
}

int do_consent(struct char_data *ch, char *arg, int cmd)
{
  char buf[MAX_INPUT_LENGTH+1], buf2[MAX_STRING_LENGTH+1];
  struct obj_data *obj;
  struct char_data *vict;

  while(isspace(*arg))
    ++arg;

  one_argument(arg, buf);
  
  if(!*buf) {
    send_to_char("Give WHO consent to touch your rotting carcass?\r\n", ch);
    return eFAILURE;
  }
 
  if(!(vict = get_char_vis(ch, buf))) {
    csendf(ch, "Consent whom?  You can't see any %s.\r\n", buf);
    return eFAILURE;
  }

  if(vict == ch) {
    send_to_char("Silly, you don't need to consent yourself!\r\n", ch);
    return eFAILURE;
  }
  if (GET_LEVEL(vict) < 10)
  {
    send_to_char("That person is too low level to be consented.\r\n",ch);
    return eFAILURE;
  }

  // prevent consenting of NPCs
  if (IS_NPC(vict)) {
    send_to_char("Now what business would THAT thing have with your mortal remains?\r\n", ch);
    return eFAILURE;
  }

  for(obj = object_list; obj; obj = obj->next) {
     if(obj->obj_flags.type_flag != ITEM_CONTAINER || obj->obj_flags.value[3] != 1 || !obj->name)
       continue;
     
     if(!isname(GET_NAME(ch), obj->name))
       // corpse isn't owned by the consenting player
       continue;
       
     // check to see if this player is already consented for the corpse
     sprintf(buf2, "%s_consent", buf);
     if (isname(buf2, obj->name))
        // keep looking; there might be other corpses not yet consented
        continue;

     // check for buffer overflow before adding the new name to the list
     if ((strlen(obj->name) + strlen(buf) + strlen(" _consent")) > (MAX_STRING_LENGTH / 2)) {
       send_to_char("Don't you think there are enough perverts molesting "
                    "your\r\nmaggot-ridden corpse already?\r\n", ch);
       return eFAILURE;
     }
     sprintf(buf2, "%s %s_consent", obj->name, buf);
     obj->name = str_hsh(buf2);
  }
  
  sprintf(buf2, "All corpses in the game which belong to you can now be "
          "molested by\r\nanyone named %s.\r\n", buf);
  send_to_char(buf2, ch);
  return eSUCCESS;
}

int contents_cause_unique_problem(obj_data * obj, char_data * vict)
{
  int lastnum = -1;

  for(obj_data * inside = obj->contains; inside; inside = inside->next_content)
  {
    if(inside->item_number < 0) // skip -1 items
       continue;
    if(lastnum == inside->item_number) // items are in order.  If we've already checked
       continue;                       // this item, don't do it again. 

    if( IS_SET(inside->obj_flags.more_flags, ITEM_UNIQUE) &&
        search_char_for_item(vict, inside->item_number, false))
       return TRUE;
    lastnum = inside->item_number;
  }
  return FALSE;
}

int contains_no_trade_item(obj_data * obj)
{
  obj_data * inside = obj->contains;

  while(inside)
  {
    if(IS_SET(inside->obj_flags.more_flags, ITEM_NO_TRADE))
      return TRUE;
    inside = inside->next_content;
  }

  return FALSE;
}

int do_drop(struct char_data *ch, char *argument, int cmd)
{
  char arg[MAX_STRING_LENGTH];
  int amount;
  char buffer[MAX_STRING_LENGTH];
  struct obj_data *tmp_object;
  struct obj_data *next_obj;
  bool test = FALSE, blindlag = FALSE;
  char alldot[MAX_STRING_LENGTH];

  alldot[0] = '\0';

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  argument = one_argument(argument, arg);

  if(is_number(arg)) {
    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\r\n", ch);
      return eFAILURE;
    }

/*    if(strlen(arg) > 7) {
      send_to_char("Number field too big.\r\n", ch);
      return eFAILURE;
    }*/
    amount = atoi(arg);
    argument=one_argument(argument,arg);
    if(str_cmp("coins",arg) && str_cmp("coin",arg) && str_cmp("gold", arg)) {
      send_to_char("Sorry, you can't do that (yet)...\r\n", ch);
      return eFAILURE;
    }
    if(amount < 0) {
      send_to_char("Sorry, you can't do that!\r\n",ch);
      return eFAILURE;
    }
    if(GET_GOLD(ch) < (uint32)amount) {
      send_to_char("You haven't got that many coins!\r\n",ch);
      return eFAILURE;
    }
    send_to_char("OK.\r\n",ch);
    if(amount==0)
      return eSUCCESS;
	
    act("$n drops some gold.", ch, 0, 0, TO_ROOM, 0);
    tmp_object = create_money(amount);
    obj_to_room(tmp_object, ch->in_room);
    GET_GOLD(ch) -= amount;
    if(GET_LEVEL(ch) >= IMMORTAL)
    {
	sprintf(buffer, "%s dropped %d coins.", GET_NAME(ch), amount);
	special_log(buffer);
    }

    do_save(ch,"",666);
    return eSUCCESS;
  }

  if(*arg) {
    if(!str_cmp(arg,"all") || sscanf(arg,"all.%s",alldot) != 0) {
      for(tmp_object = ch->carrying; tmp_object; tmp_object = next_obj) {
         next_obj = tmp_object->next_content;

         if(alldot[0] != '\0' && !isname(alldot, tmp_object->name)) continue;

         if(IS_SET(tmp_object->obj_flags.extra_flags, ITEM_SPECIAL))
           continue;

         if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) {
            send_to_char("Your criminal acts prohibit it.\r\n", ch);
            return eFAILURE;
         }
         if(IS_SET(tmp_object->obj_flags.more_flags, ITEM_NO_TRADE)) 
           continue;
         if(contains_no_trade_item(tmp_object))
           continue;
         if(!IS_SET(tmp_object->obj_flags.extra_flags, ITEM_NODROP) ||
               GET_LEVEL(ch) >= IMMORTAL) {
            if(IS_SET(tmp_object->obj_flags.extra_flags, ITEM_NODROP))
               send_to_char("(This item is cursed, BTW.)\r\n", ch);
            if(CAN_SEE_OBJ(ch, tmp_object)) {
               sprintf(buffer, "You drop the %s.\r\n", fname(tmp_object->name));
               send_to_char(buffer, ch);
            }
            else if(CAN_SEE_OBJ(ch, tmp_object, TRUE)) {
               sprintf(buffer, "You drop the %s.\r\n", fname(tmp_object->name));
               send_to_char(buffer, ch);
               blindlag = TRUE;
            }
            else
               send_to_char("You drop something.\r\n", ch);

            if (tmp_object->obj_flags.type_flag != ITEM_MONEY)
            {
              sprintf(log_buf, "%s drops %s[%d] in room %d", GET_NAME(ch), tmp_object->name, obj_index[tmp_object->item_number].virt,ch->in_room);
              log(log_buf, IMP, LOG_OBJECTS);
              for(OBJ_DATA *loop_obj = tmp_object->contains; loop_obj; loop_obj = loop_obj->next_content)
                logf(IMP, LOG_OBJECTS, "The %s contained %s[%d]",
                          tmp_object->short_description,
                          loop_obj->short_description,
                          obj_index[loop_obj->item_number].virt);
	    }
	    
            act("$n drops $p.", ch, tmp_object, 0, TO_ROOM, INVIS_NULL);
            move_obj(tmp_object, ch->in_room);
            test = TRUE;
            if(blindlag) WAIT_STATE(ch, PULSE_VIOLENCE);
         }
         else {
            if(CAN_SEE_OBJ(ch, tmp_object, TRUE)) {
               sprintf(buffer, "You can't drop the %s, it must be CURSED!\r\n", fname(tmp_object->name));
               send_to_char(buffer, ch);
               test = TRUE;
            }
         }
      } /* for */

      if(!test)
        send_to_char("You do not seem to have anything.\r\n", ch);

    } /* if strcmp "all" */

    else {
      tmp_object = get_obj_in_list_vis(ch, arg, ch->carrying);
      if(tmp_object) {

      if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) {
        send_to_char("Your criminal acts prohibit it.\r\n", ch);
        return eFAILURE;
      }
      if(IS_SET(tmp_object->obj_flags.more_flags, ITEM_NO_TRADE) && GET_LEVEL(ch) < IMMORTAL) {
        send_to_char("It seems magically attached to you.\r\n", ch);
        return eFAILURE;
      }
      if(contains_no_trade_item(tmp_object)) {
        send_to_char("Something inside it seems magically attached to you.\r\n", ch);
        return eFAILURE;
      }

        if(IS_SET(tmp_object->obj_flags.extra_flags, ITEM_SPECIAL)) {
	  send_to_char("Don't be a dork.\r\n", ch);
	  return eFAILURE;
	}
        else if(!IS_SET(tmp_object->obj_flags.extra_flags, ITEM_NODROP) ||
            GET_LEVEL(ch) >= IMMORTAL) {
          if(IS_SET(tmp_object->obj_flags.extra_flags, ITEM_NODROP))
            send_to_char("(This item is cursed, BTW.)\r\n", ch);
          sprintf(buffer, "You drop the %s.\r\n", fname(tmp_object->name));
          send_to_char(buffer, ch);
          act("$n drops $p.", ch, tmp_object, 0, TO_ROOM, INVIS_NULL);
          if (tmp_object->obj_flags.type_flag != ITEM_MONEY)
          {
            sprintf(log_buf, "%s drops %s[%d] in room %d", GET_NAME(ch), tmp_object->name, obj_index[tmp_object->item_number].virt,ch->in_room);
            log(log_buf, IMP, LOG_OBJECTS);
            for(OBJ_DATA *loop_obj = tmp_object->contains; loop_obj; loop_obj = loop_obj->next_content)
              logf(IMP, LOG_OBJECTS, "The %s contained %s[%d]",
                          tmp_object->short_description,
                          loop_obj->short_description,
                          obj_index[loop_obj->item_number].virt);
          }


          move_obj(tmp_object, ch->in_room);
          return eSUCCESS;
        }
        else
	  send_to_char("You can't drop it, it must be CURSED!\r\n", ch);
      }
      else
        send_to_char("You do not have that item.\r\n", ch);

    }
    do_save(ch, "", 666);
  }
  else
    send_to_char("Drop what?\r\n", ch);
  return eFAILURE;
}

void do_putalldot(struct char_data *ch, char *name, char *target, int cmd)
{
  struct obj_data *tmp_object;
  struct obj_data *next_object;
  char buf[200];
  bool found = FALSE;

 /* If "put all.object bag", get all carried items
  * named "object", and put each into the bag.
  */

  for(tmp_object = ch->carrying; tmp_object; tmp_object = next_object) {
     next_object = tmp_object->next_content;
     if(!name && CAN_SEE_OBJ(ch, tmp_object)) {
       sprintf(buf, "%s %s", fname(tmp_object->name), target);
       buf[99] = 0;
       found = TRUE;
       do_put(ch, buf, cmd);
     }
     else if(isname(name, tmp_object->name) && CAN_SEE_OBJ(ch, tmp_object)) {
       sprintf(buf, "%s %s", name, target);
       buf[99] = 0;
       found = TRUE;
       do_put(ch, buf, cmd);
     }
  }

  if(!found)
    send_to_char("You don't have one.\r\n", ch);
}

int weight_in(struct obj_data *obj)
{// Sheldon backpack. Damn procs. 
  int w = 0;
  struct obj_data *obj2;
  for (obj2 = obj->contains; obj2; obj2 = obj2->next_content)
     w += obj2->obj_flags.weight;
  return w;
}

int do_put(struct char_data *ch, char *argument, int cmd)
{
  char buffer[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  struct obj_data *obj_object;
  struct obj_data *sub_object;
  struct char_data *tmp_char;
  int bits;
  char allbuf[MAX_STRING_LENGTH];

  if (IS_SET(world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  argument_interpreter(argument, arg1, arg2);

  if (*arg1)
  {
    if (*arg2)
    {
      if (!(get_obj_in_list_vis(ch, arg2, ch->carrying)) && !(get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents)))
      {
        sprintf(buffer, "You don't have a %s.\r\n", arg2);
        send_to_char(buffer, ch);
        return 1;
      }
      allbuf[0] = '\0';
      if (!str_cmp(arg1, "all"))
      {
        do_putalldot(ch, 0, arg2, cmd);
        return eSUCCESS;
      }
      else if (sscanf(arg1, "all.%s", allbuf) != 0)
      {
        do_putalldot(ch, allbuf, arg2, cmd);
        return eSUCCESS;
      }
      obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);

      if (obj_object)
      {
        if (IS_SET(obj_object->obj_flags.extra_flags, ITEM_NODROP))
        {
          if (GET_LEVEL(ch) < IMMORTAL)
          {
            send_to_char("You are unable to! That item must be CURSED!\r\n", ch);
            return eFAILURE;
          }
          else
            send_to_char("(This item is cursed, BTW.)\r\n", ch);
        }
        if (obj_index[obj_object->item_number].virt == CHAMPION_ITEM)
        {
          send_to_char("You must display this flag for all to see!\r\n", ch);
          return eFAILURE;
        }
        if (IS_SET(obj_object->obj_flags.extra_flags, ITEM_NEWBIE))
        {
          send_to_char("The protective enchantment this item holds cannot be held within this container.\r\n", ch);
          return eFAILURE;
        }
        if (ARE_CONTAINERS(obj_object))
        {
          send_to_char("You can't put that in there.\r\n", ch);
          return eFAILURE;
        }

        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM,
                            ch, &tmp_char, &sub_object);
        if (sub_object)
        {
          if (ARE_CONTAINERS(sub_object))
          {
            // Keyrings can only hold keys
            if (GET_ITEM_TYPE(sub_object) == ITEM_KEYRING && GET_ITEM_TYPE(obj_object) != ITEM_KEY)
            {
              csendf(ch, "You can't put %s on a keyring.\r\n", GET_OBJ_SHORT(obj_object));
              return eFAILURE;
            }

            // Altars can only hold totems
            if (GET_ITEM_TYPE(sub_object) == ITEM_ALTAR && GET_ITEM_TYPE(obj_object) != ITEM_TOTEM)
            {
              send_to_char("You cannot put that in an altar.\r\n", ch);
              return eFAILURE;
            }

            if (!IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED))
            {
              // Can't put an item in itself
              if (obj_object == sub_object)
              {
                send_to_char("You attempt to fold it into itself, but fail.\r\n", ch);
                return eFAILURE;
              }

              // Can't put godload in non-godload
              if (IS_SPECIAL(obj_object) && NOT_SPECIAL(sub_object))
              {
                send_to_char("Are you crazy?!  Someone could steal it!\r\n", ch);
                return eFAILURE;
              }

              // Can't put NO_TRADE item in someone else's container/altar/totem
              if (IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) &&
                  sub_object->carried_by != ch)
              {
                send_to_char("You can't trade that item.\r\n", ch);
                return eFAILURE;
              }

              if (IS_SET(obj_object->obj_flags.more_flags, ITEM_UNIQUE) && (sub_object->carried_by != ch) && search_container_for_item(sub_object, obj_object->item_number))
              {
                send_to_char("The object's uniqueness prevents it!\r\n", ch);
                return eFAILURE;
              }

              if (((sub_object->obj_flags.weight) +
                   (obj_object->obj_flags.weight)) <=
                      (sub_object->obj_flags.value[0]) &&
                  (obj_index[sub_object->item_number].virt != 536 ||
                   weight_in(sub_object) + obj_object->obj_flags.weight <= 200))
              {
                if (bits == FIND_OBJ_INV)
                {
                  obj_from_char(obj_object);
                  /* make up for above line */
                  if (obj_index[sub_object->item_number].virt != 536)
                    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj_object);
                  obj_to_obj(obj_object, sub_object);
                }
                else
                {
                  move_obj(obj_object, sub_object);
                }

                if (GET_ITEM_TYPE(sub_object) == ITEM_KEYRING)
                {
                  act("$n attaches $p to the $P.", ch, obj_object, sub_object, TO_ROOM, INVIS_NULL);
                  act("You attach $p to the $P.", ch, obj_object, sub_object, TO_CHAR, 0);
                  logf(IMP, LOG_OBJECTS, "%s attaches %s[%d] to %s[%d]",
                      ch->name,
                      obj_object->short_description,
                      obj_index[obj_object->item_number].virt,
                      sub_object->short_description,
                      obj_index[sub_object->item_number].virt);
                }
                else
                {
                  act("$n puts $p in $P.", ch, obj_object, sub_object, TO_ROOM, INVIS_NULL);
                  act("You put $p in $P.", ch, obj_object, sub_object, TO_CHAR, 0);
                  logf(IMP, LOG_OBJECTS, "%s puts %s[%d] in %s[%d]",
                      ch->name,
                      obj_object->short_description,
                      obj_index[obj_object->item_number].virt,
                      sub_object->short_description,
                      obj_index[sub_object->item_number].virt);
                }

                return eSUCCESS;
              }
              else
              {
                send_to_char("It won't fit.\r\n", ch);
              }
            }
            else
              send_to_char("It seems to be closed.\r\n", ch);
          }
          else
          {
            sprintf(buffer, "The %s is not a container.\r\n", fname(sub_object->name));
            send_to_char(buffer, ch);
          }
        }
        else
        {
          sprintf(buffer, "You dont have the %s.\r\n", arg2);
          send_to_char(buffer, ch);
        }
      }
      else
      {
        sprintf(buffer, "You dont have the %s.\r\n", arg1);
        send_to_char(buffer, ch);
      }
    } /* if arg2 */
    else
    {
      sprintf(buffer, "Put %s in what?\r\n", arg1);
      send_to_char(buffer, ch);
    }
  } /* if arg1 */
  else
  {
    send_to_char("Put what in what?\r\n", ch);
  }
  return eFAILURE;
}

void do_givealldot(CHAR_DATA *ch, char *name, char *target, int cmd)
{
  struct obj_data *tmp_object;
  struct obj_data *next_object;
  char buf[200];
  bool found = FALSE;

  for(tmp_object = ch->carrying; tmp_object; tmp_object = next_object) {
     next_object = tmp_object->next_content;
     if(!name && CAN_SEE_OBJ(ch, tmp_object)) {
       sprintf(buf, "%s %s", fname(tmp_object->name), target);
       buf[99] = 0;
       found = TRUE;
       do_give(ch, buf, cmd);
     }
     else if(isname(name, tmp_object->name) && CAN_SEE_OBJ(ch, tmp_object)) {
       sprintf(buf, "%s %s", name, target);
       buf[99] = 0;
       found = TRUE;
       do_give(ch, buf, cmd);
     }
  }

  if(!found)
    send_to_char("You don't have one.\r\n", ch);
}

int do_give(struct char_data *ch, char *argument, int cmd)
{
  char obj_name[MAX_INPUT_LENGTH+1], vict_name[MAX_INPUT_LENGTH+1], buf[200];
  char arg[80], allbuf[80];
  long long amount;
  int retval;
  extern int top_of_world;
  struct char_data *vict;
  struct obj_data *obj;

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n",
                  ch);
    return eFAILURE;
  }

  if (affected_by_spell(ch, FUCK_PTHIEF))
  {
    send_to_char("Your criminal actions prohibit it.\r\n",ch);
    return eFAILURE;
  }
  argument = one_argument(argument, obj_name);

  if(is_number(obj_name)) { 
    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\r\n", ch);
      return eFAILURE;
    }
/*
    if(strlen(obj_name) > 7) {
      send_to_char("Number field too large.\r\n", ch);
      return eFAILURE;
    }*/
    amount = atoll(obj_name);
    argument = one_argument(argument, arg);
    if(str_cmp("gold", arg) && str_cmp("coins",arg) && str_cmp("coin",arg)) { 
      send_to_char("Sorry, you can't do that (yet)...\r\n",ch);
      return eFAILURE;
    }
    if(amount < 0) { 
      send_to_char("Sorry, you can't do that!\r\n",ch);
      return eFAILURE;
    }
    if(GET_GOLD(ch) < amount && GET_LEVEL(ch) < DEITY) 
    {
      send_to_char("You haven't got that many coins!\r\n",ch);
      return eFAILURE;
    }
    argument = one_argument(argument, vict_name);
    if(!*vict_name) { 
      send_to_char("To whom?\r\n",ch);
      return eFAILURE;
    }

      if (!(vict = get_char_room_vis(ch, vict_name))) 
      {
        send_to_char("To whom?\r\n",ch);
        return eFAILURE;
      }
    
      
      if(ch == vict) {
         send_to_char("Umm okay, you give it to yourself.\r\n", ch);
         return eFAILURE;
      }
/*
      if(GET_GOLD(vict) > 2000000000) {
         send_to_char("They can't hold that much gold!\r\n", ch);
         return eFAILURE;
      }
*/
      csendf(ch, "You give %lld coin%s to %s.\r\n", amount,
             amount == 1 ? "" : "s", GET_SHORT(vict));

        sprintf(buf, "%s gives %lld coin%s to %s", GET_NAME(ch), amount,
                pluralize(amount), GET_NAME(vict));
        log(buf, IMP, LOG_OBJECTS);
      
      sprintf(buf, "%s gives you %lld gold coin%s.", PERS(ch, vict), amount,
               amount == 1 ? "" : "s");
      act(buf, ch, 0, vict, TO_VICT, INVIS_NULL);
      act("$n gives some gold to $N.", ch, 0, vict,TO_ROOM,INVIS_NULL|NOTVICT);
      
      GET_GOLD(ch) -= amount;

      if(IS_NPC(ch) && (!IS_AFFECTED(ch, AFF_CHARM) || GET_LEVEL(ch) > 50)) 
      {
         sprintf(buf, "%s (mob) giving gold to %s.", GET_NAME(ch),
                 GET_NAME(vict));
         special_log(buf);
      }

      if(GET_GOLD(ch) < 0) 
      {
         GET_GOLD(ch) = 0;
         send_to_char("Warning:  You are giving out more gold than you had.\r\n", ch);
         if(GET_LEVEL(ch) < IMP) {
           sprintf(buf, "%s gives %lld coins to %s (negative!)", GET_NAME(ch), 
                   amount, GET_NAME(vict));
           special_log(buf);
         }
      }
      GET_GOLD(vict) += amount;

      // If a mob is given gold, we disable its ability to receive a gold bonus. This keeps
      // the mob from turning into an interest bearing savings account. :)
      if (IS_NPC(vict)) {
        SETBIT(vict->mobdata->actflags, ACT_NO_GOLD_BONUS);
      }


      do_save(ch, "", 10);
      do_save(vict, "", 10);
      // bribe trigger automatically removes any gold given to mob
      mprog_bribe_trigger( vict, ch, amount );

      return eSUCCESS;
    }

    argument = one_argument(argument, vict_name);

    if (!*obj_name || !*vict_name)
    {
	send_to_char("Give what to whom?\r\n", ch);
	return eFAILURE;
    }

    if(!strcmp(vict_name, "follower"))
    {
      bool found = false;
      struct follow_type *k;
      int org_room = ch->in_room;
      if(ch->followers)
        for (k = ch->followers; k && k != (follow_type *)0x95959595; k = k->next) 
        {
          if (org_room == k->follower->in_room)
            if (IS_AFFECTED(k->follower, AFF_CHARM)) 
            {
              vict = k->follower;
              found = true;
	    }
        }

      if(!found)
      {
	send_to_char("Nobody here are loyal subjects of yours!\r\n", ch);
        return eFAILURE;
      }
    }
    else
    {
      if (!(vict = get_char_room_vis(ch, vict_name)))
      {
	send_to_char("No one by that name around here.\r\n", ch);
	return eFAILURE;
      }
    }

    if (ch == vict)
    {
       send_to_char("Why give yourself stuff?\r\n", ch);
       return eFAILURE;
    }

    if(!str_cmp(obj_name, "all")) {
       do_givealldot(ch, 0, vict_name, cmd);
       return eSUCCESS;
    }
    else if(sscanf(obj_name, "all.%s", allbuf) != 0) {
       do_givealldot(ch, allbuf, vict_name, cmd);
       return eSUCCESS;
    }

    if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
    {
	send_to_char("You do not seem to have anything like that.\r\n",
	   ch);
	return eFAILURE;
    }
    if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) && GET_LEVEL(ch) < OVERSEER) {
      send_to_char("That sure would be a fucking stupid thing to do.\r\n", ch);
      return eFAILURE;
    }

    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) {
      send_to_char("Your criminal acts prohibit it.\r\n", ch);
      return eFAILURE;
    }

    if (IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP))
    {
      if(GET_LEVEL(ch) < DEITY)
      {
	send_to_char("You can't let go of it! Yeech!!\r\n", ch);
	return eFAILURE;
      }
      else
        send_to_char("This item is NODROP btw.\r\n", ch);
    }

    // You can give no_trade items to mobs for quest purposes.  It's taken care of later
    if(!IS_NPC(vict) && IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) &&
       (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)))
    {
      if(GET_LEVEL(ch) > IMMORTAL)
         send_to_char("That was a NO_TRADE item btw....\r\n", ch);
      else {
        send_to_char("It seems magically attached to you.\r\n", ch);
        return eFAILURE;
      }
    }
    if(contains_no_trade_item(obj)) {
      if(GET_LEVEL(ch) > IMMORTAL)
         send_to_char("That was a NO_TRADE item btw....\r\n", ch);
      else {
        send_to_char("Something inside it seems magically attached to you.\r\n", ch);
        return eFAILURE;
      }
    }

    if (IS_NPC(vict) && (mob_index[vict->mobdata->nr].non_combat_func == shop_keeper || mob_index[vict->mobdata->nr].virt == QUEST_MASTER))
    {
       act("$N graciously refuses your gift.", ch, 0, vict, TO_CHAR, 0);
       return eFAILURE;
    }
    if (IS_NPC(vict) && IS_AFFECTED(vict, AFF_CHARM) && (IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE)  || contains_no_trade_item(obj)))
    {
	send_to_char("The creature doesn't understand what you're trying to do.\r\n",ch);
	return eFAILURE;
    }

    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF) && !vict->desc) {
      send_to_char("Now WHY would a thief give something to a linkdead char..?\r\n", ch);
      return eFAILURE;
    }

    if ((1+IS_CARRYING_N(vict)) > CAN_CARRY_N(vict))
    {
       if((ch->in_room >= 0 && ch->in_room <= top_of_world) && !strcmp(obj_name, "potato") &&
          IS_SET(world[ch->in_room].room_flags, ARENA) && IS_SET(world[vict->in_room].room_flags, ARENA) &&
          arena.type == POTATO) {
         ;
       } else {
         act("$N seems to have $S hands full.", ch, 0, vict, TO_CHAR, 0);
         return eFAILURE;
       }
    }
    if (obj->obj_flags.weight + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
    {
      if((ch->in_room >= 0 && ch->in_room <= top_of_world) && !strcmp(obj_name, "potato") &&
         IS_SET(world[ch->in_room].room_flags, ARENA) && IS_SET(world[vict->in_room].room_flags, ARENA) &&
         arena.type == POTATO) {
         ;
      } else {
        act("$E can't carry that much weight.", ch, 0, vict, TO_CHAR, 0);
        return eFAILURE;
      } 
    }
    if(IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE)) {
      if(search_char_for_item(vict, obj->item_number, false)) {
         send_to_char("The item's uniqueness prevents it.\r\n", ch);
         csendf(vict, "%s tried to give you an item but was unable.\r\n", GET_NAME(ch));
         return eFAILURE;
      }
    }
    if(contents_cause_unique_problem(obj, vict)) {
      send_to_char("The uniqueness of something inside it prevents it.\r\n", ch);
      csendf(vict, "%s tried to give you an item but was unable.\r\n", GET_NAME(ch));
      return eFAILURE;
    }

    move_obj(obj, vict);
    act("$n gives $p to $N.", ch, obj, vict, TO_ROOM, INVIS_NULL|NOTVICT);
    act("$n gives you $p.", ch, obj, vict, TO_VICT, 0);
    act("You give $p to $N.", ch, obj, vict, TO_CHAR, 0);

    sprintf(buf, "%s gives %s to %s", GET_NAME(ch), obj->name,
                GET_NAME(vict));
    log(buf, IMP, LOG_OBJECTS);
    for(OBJ_DATA *loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
              logf(IMP, LOG_OBJECTS, "The %s[%d] contained %s[%d]", 
                          obj->short_description,
		          obj_index[obj->item_number].virt,
                          loop_obj->short_description,
                          obj_index[loop_obj->item_number].virt);

    if((vict->in_room >= 0 && vict->in_room <= top_of_world) && GET_LEVEL(vict) < IMMORTAL && 
      IS_SET(world[vict->in_room].room_flags, ARENA) && arena.type == POTATO && obj_index[obj->item_number].virt == 393) {
      send_to_char("Here, have some for some potato lag!!\r\n", vict);
      WAIT_STATE(vict, PULSE_VIOLENCE *2);
    }

//    send_to_char("Ok.\r\n", ch);
    do_save(ch,"",10);
    do_save(vict,"",10);
    // if I gave a no_trade item to a mob, the mob needs to destroy it
    // otherwise it defeats the purpose of no_trade:)

    retval = mprog_give_trigger( vict, ch, obj );
bool objExists(OBJ_DATA *obj);
    if(!IS_SET(retval, eEXTRA_VALUE) && IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) && IS_NPC(vict) && 
		objExists(obj))
       extract_obj(obj);

    if(SOMEONE_DIED(retval)) {
      retval = SWAP_CH_VICT(retval);
      return eSUCCESS|retval;
    }

    return eSUCCESS;
}

// Find an item on a character (in inv, or containers in inv (NOT WORN!))
// and try to put it in his inv.  If sucessful, return pointer to the item.
struct obj_data * bring_type_to_front(char_data * ch, int item_type)
{
  struct obj_data *item_carried = NULL;
  struct obj_data *container_item = NULL;
  
  queue <obj_data*> container_queue;
 
  for(item_carried = ch->carrying; item_carried ; item_carried = item_carried->next_content) 
  {
    if(GET_ITEM_TYPE(item_carried) == item_type)
      return item_carried;
    if(GET_ITEM_TYPE(item_carried) == ITEM_CONTAINER && !IS_SET(item_carried->obj_flags.value[1], CONT_CLOSED)) 
	{ // search inside if open
	  container_queue.push(item_carried);
	}

  }

    //  if(GET_ITEM_TYPE(i) == ITEM_CONTAINER) { // search inside if open
  while(container_queue.size() > 0)
  {
      item_carried = container_queue.front();
	  container_queue.pop();
	  for(container_item = item_carried->contains; container_item ; container_item = container_item->next_content) 
	  {
		if(GET_ITEM_TYPE(container_item) == item_type) 
		{
		  get(ch, container_item, item_carried, 0, 9);
		  return container_item;
		}
	  }
  }
  return NULL;
}

// Find an item on a character
struct obj_data * search_char_for_item(char_data * ch, int16 item_number, bool wearonly)
{
  struct obj_data *i = NULL;
  struct obj_data *j = NULL;
  int k;

  for (k=0; k< MAX_WEAR; k++) {
    if (ch->equipment[k])
    {
      if(ch->equipment[k]->item_number == item_number)
        return ch->equipment[k];
      if(GET_ITEM_TYPE(ch->equipment[k]) == ITEM_CONTAINER) { // search inside
        for(j = ch->equipment[k]->contains; j ; j = j->next_content) {
          if(j->item_number == item_number)
            return j;
        }
      }
    }
  }
      if (!wearonly)
  for(i = ch->carrying; i ; i = i->next_content) {
    if(i->item_number == item_number)
      return i;

    // does not support containers inside containers
    if(GET_ITEM_TYPE(i) == ITEM_CONTAINER) { // search inside
      for(j = i->contains; j ; j = j->next_content) {
        if(j->item_number == item_number) 
          return j;
      }
    }
  }
  return NULL;
}

// Find out how many of an item exists on character
int search_char_for_item_count(char_data * ch, int16 item_number, bool wearonly)
{
  struct obj_data *i = NULL;
  struct obj_data *j = NULL;
  int k;
  int count = 0;
  
  for (k=0; k< MAX_WEAR; k++) {
    if (ch->equipment[k]) {
      if(ch->equipment[k]->item_number == item_number)
	count++;
      if(GET_ITEM_TYPE(ch->equipment[k]) == ITEM_CONTAINER) { // search inside
        for(j = ch->equipment[k]->contains; j ; j = j->next_content) {
          if(j->item_number == item_number)
	    count++;
        }
      }
    }
  }
  
  if (!wearonly)
    for(i = ch->carrying; i ; i = i->next_content) {
      if(i->item_number == item_number)
	count++;

      // does not support containers inside containers
      if(GET_ITEM_TYPE(i) == ITEM_CONTAINER) { // search inside
	for(j = i->contains; j ; j = j->next_content) {
	  if(j->item_number == item_number) 
	    count++;
	}
      }
    }

  return count;
}

bool search_container_for_item(obj_data *obj, int item_number)
{
  if (obj == nullptr)
  {
    return false;
  }

  if (NOT_CONTAINERS(obj))
  {
    return false;
  }

  for (obj_data *i = obj->contains; i; i = i->next_content)
  {
    if (IS_KEY(i) && i->item_number == item_number)
    {
      return true;
    }
  }

  return false;
}

bool search_container_for_vnum(obj_data *obj, int vnum)
{
  if (obj == nullptr)
  {
    return false;
  }

  if (NOT_CONTAINERS(obj))
  {
    return false;
  }

  for (obj_data *i = obj->contains; i; i = i->next_content)
  {
    if (IS_KEY(i) && obj_index[i->item_number].virt == vnum)
    {
      return true;
    }
  }

  return false;
}

int find_door(CHAR_DATA *ch, char *type, char *dir)
{
    int door;
    const char *dirs[] =
    {
        "north",
        "east",
        "south",
        "west",
        "up",
        "down",
        "\n"
    };
  
    if (*dir) /* a direction was specified */
    {
        if ((door = search_block(dir, dirs, FALSE)) == -1) /* Partial Match */
        {
            send_to_char("That's not a direction.\r\n", ch);
            return(-1);
        }
     
        if (EXIT(ch, door))
            if (EXIT(ch, door)->keyword)
                if (isname(type, EXIT(ch, door)->keyword))
                    return(door);
                else
                {
                    return(-1);
                }
            else
                return(door);
        else
        {   
            return(-1);
        }
    }
    else // try to locate the keyword
    {
        for (door = 0; door <= 5; door++)
            if (EXIT(ch, door))
                if (EXIT(ch, door)->keyword)
                    if (isname(type, EXIT(ch, door)->keyword))
                        return(door);
                        
        return(-1);
    }
}

/*
in_room == exit->in_room

*/
bool is_bracing(CHAR_DATA *bracee, struct room_direction_data *exit)
{
  //this could happen on a repop of the zone
  if(!IS_SET(exit->exit_info, EX_CLOSED))
    return false;

  //this could happen from some sort of bug
  if(IS_SET(exit->exit_info, EX_BROKEN))
    return false;

  //has to be standing
  if(GET_POS(bracee) < POSITION_STANDING)
    return false;

  //if neither the spot bracee is at, nor remote spot, is equal to teh exit
  if(bracee->brace_at != exit && bracee->brace_exit != exit) 
    return false;

  for(int i = 0; i < 6; i++)  
    if(world[bracee->in_room].dir_option[i] == exit)
      return true;

  if(bracee->in_room == exit->to_room)
  {
    if(exit == bracee->brace_at || exit == bracee->brace_exit)
      return true;
  }

  return false;
}

int do_open(CHAR_DATA *ch, char *argument, int cmd)
{
  bool found = false;
   int door, other_room, retval;
   char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
   struct room_direction_data *back;
   struct obj_data *obj;
   CHAR_DATA *victim;
   CHAR_DATA *next_vict;
            
   int do_fall(CHAR_DATA *ch, short dir);
            
   retval = 0;
         
   argument_interpreter(argument, type, dir);
            
   if (!*type) {
      send_to_char("Open what?\r\n", ch);
      return eFAILURE;
   } else if ((door = find_door(ch, type, dir)) >= 0)
   { 
     found = true;
      if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
         send_to_char("That's impossible, I'm afraid.\r\n", ch);
      else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
         send_to_char("It's already open!\r\n", ch);
      else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
         send_to_char("It seems to be locked.\r\n", ch);  
      else if (IS_SET(EXIT(ch, door)->exit_info, EX_BROKEN)) 
         send_to_char("It's already been broken open!\r\n", ch);  
      else if (EXIT(ch, door)->bracee != NULL)
      {
         if(is_bracing(EXIT(ch, door)->bracee, EXIT(ch, door)))
         {
           if(EXIT(ch, door)->bracee->in_room == ch->in_room)
           {
             csendf(ch, "%s is holding the %s shut.\r\n", 
                          EXIT(ch, door)->bracee->name, fname(EXIT(ch, door)->keyword));
             csendf(EXIT(ch, door)->bracee, "The %s quivers slightly but holds as %s attempts to force their way through.\r\n",
                                     fname(EXIT(ch, door)->keyword), ch);
           }
           else
           {
             csendf(ch, "The %s seems to be barred from the other side.\r\n", fname(EXIT(ch, door)->keyword));   
             csendf(EXIT(ch, door)->bracee, "The %s quivers slightly but holds as someone attempts to force their way through.\r\n", 
                              fname(EXIT(ch, door)->keyword));
           }
         }
         else
         {
           do_brace(EXIT(ch, door)->bracee, "", 0);
           return do_open(ch, argument, cmd);
         }
      }
      else if (IS_SET(EXIT(ch, door)->exit_info, EX_IMM_ONLY) && GET_LEVEL(ch) < IMMORTAL)
         send_to_char("It seems to slither and resist your attempt to touch it.\r\n", ch);
      else
      {   
         REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
            
         if(IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN)) {
            if (EXIT(ch, door)->keyword) {
               act("$n reveals a hidden $F!", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
               csendf(ch, "You reveal a hidden %s!\r\n", fname((char *)EXIT(ch, door)->keyword));
            }
            else {
               act("$n reveals a hidden door!",ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
               send_to_char("You reveal a hidden door!\r\n", ch);
            }
         }  
         else {
           if (EXIT(ch, door)->keyword)
              act("$n opens the $F.", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
				else {
              act("$n opens the door.", ch, 0, 0, TO_ROOM, 0);
				}
              send_to_char("Ok.\r\n", ch);
         }
             
         /* now for opening the OTHER side of the door! */
         if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
            if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
               if (back->to_room == ch->in_room)
               {
                  REMOVE_BIT(back->exit_info, EX_CLOSED);
                  if ((back->keyword) && !IS_SET(world[EXIT(ch, door)->to_room].room_flags, QUIET))
                  {
                     sprintf(buf, "The %s is opened from the other side.\r\n",
                                fname(back->keyword));
                     send_to_room(buf, EXIT(ch, door)->to_room, TRUE);
                  }
                  else
                     send_to_room("The door is opened from the other side.\r\n",
                                EXIT(ch, door)->to_room, TRUE);
               }

         if((IS_SET(world[ch->in_room].room_flags, FALL_DOWN) && (door = 5)) ||
            (IS_SET(world[ch->in_room].room_flags, FALL_UP) && (door = 4)) ||
            (IS_SET(world[ch->in_room].room_flags, FALL_EAST) && (door = 1)) ||
            (IS_SET(world[ch->in_room].room_flags, FALL_WEST) && (door = 3)) ||
            (IS_SET(world[ch->in_room].room_flags, FALL_SOUTH) && (door = 2)) ||
            (IS_SET(world[ch->in_room].room_flags, FALL_NORTH) && (door = 0)))
         {
            int success = 0;

            // opened the door that kept them from falling out
            for(victim = world[ch->in_room].people; victim; victim = next_vict)
            {
               next_vict = victim->next_in_room;
               if(IS_NPC(victim) || IS_AFFECTED(victim, AFF_FLYING))  
                  continue;
               if(!success) {
                  send_to_room("With the door no longer closed for support, this area's strange gravity takes over!\r\n", victim->in_room, TRUE);
                  success = 1;
               }
               if(victim == ch)
                  retval = do_fall(victim, door);
               else do_fall(victim, door);
            }
         }
      }
   }
   else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
   {
     found = true;
      // this is an object
      if (obj->obj_flags.type_flag != ITEM_CONTAINER)
         send_to_char("That's not a container.\r\n", ch);
      else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
         send_to_char("But it's already open!\r\n", ch);
      else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE))
         send_to_char("You can't do that.\r\n", ch);
      else if (IS_SET(obj->obj_flags.value[1], CONT_LOCKED))
         send_to_char("It seems to be locked.\r\n", ch);   
      else
      {   
         REMOVE_BIT(obj->obj_flags.value[1], CONT_CLOSED);
         send_to_char("Ok.\r\n", ch);
         act("$n opens $p.", ch, obj, 0, TO_ROOM, 0);
      }
   }

   if (found == false) {
     csendf(ch, "I see no %s here.\r\n", type);
   }

   // in case ch died or anything
   if(retval)
      return retval;
   return eSUCCESS;
}

int do_close(CHAR_DATA *ch, char *argument, int cmd)
{
  bool found = false;
   int door, other_room;
   char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
   struct room_direction_data *back;
   struct obj_data *obj;
   CHAR_DATA *victim;   
    
   argument_interpreter(argument, type, dir);
    
   if (!*type) {
      send_to_char("Close what?\r\n", ch);
      return eFAILURE;
   } else if ((door = find_door(ch, type, dir)) >= 0)
   {    
     found = true;
      if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
         send_to_char("That's absurd.\r\n", ch);
      else if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
         send_to_char("It's already closed!\r\n", ch);
      else if (IS_SET(EXIT(ch, door)->exit_info, EX_BROKEN))
         send_to_char("It appears to be broken!\r\n", ch);
      else
      {   
         SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
         if (EXIT(ch, door)->keyword)
            act("$n closes the $F.", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
         else
            act("$n closes the door.", ch, 0, 0, TO_ROOM, 0);
         send_to_char("Ok.\r\n", ch);
         /* now for closing the other side, too */
         if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
            if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
               if (back->to_room == ch->in_room)
               {
                  SET_BIT(back->exit_info, EX_CLOSED);
                  if ((back->keyword) &&
                       !IS_SET(world[EXIT(ch, door)->to_room].room_flags, QUIET))
                  {
                     sprintf(buf, "The %s closes quietly.\r\n",
                                   fname(back->keyword));
                     send_to_room(buf, EXIT(ch, door)->to_room, TRUE);
                  }
                  else
                     send_to_room("The door closes quietly.\r\n",
                                       EXIT(ch, door)->to_room, TRUE);
               }
      }
   }
   else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
   {     
     found = true;
      if (obj->obj_flags.type_flag != ITEM_CONTAINER)
         send_to_char("That's not a container.\r\n", ch);
      else if (IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
         send_to_char("But it's already closed!\r\n", ch); 
      else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE))
         send_to_char("That's impossible.\r\n", ch);
      else
      {   
         SET_BIT(obj->obj_flags.value[1], CONT_CLOSED);
         send_to_char("Ok.\r\n", ch);
         act("$n closes $p.", ch, obj, 0, TO_ROOM, 0);
      }
   }

   if (found == false) {
     csendf(ch, "I see no %s here.\r\n", type);
   }

   return eSUCCESS;
}

bool has_key(CHAR_DATA *ch, int key)
{
 //if key vnum is 0, there is no key
  if (key == 0)
  {
    return false;
  }

  obj_data *obj = ch->equipment[HOLD];
  if (obj && IS_KEY(obj))
  {
    if (obj_index[obj->item_number].virt == key)
    {
      return true;
    }
  }

  for (obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (IS_KEY(obj) && obj_index[obj->item_number].virt == key)
    {
      return true;
    }

    if (IS_KEYRING(obj))
    {
      return search_container_for_vnum(obj, key);
    }
  }

  return false;
}

int do_lock(CHAR_DATA *ch, char *argument, int cmd)
{
    int door, other_room;
    char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
    struct room_direction_data *back;
    struct obj_data *obj;
    CHAR_DATA *victim;   
    
    argument_interpreter(argument, type, dir);
    
    if (!*type) {
        send_to_char("Lock what?\r\n", ch);
	return eFAILURE;
    } else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
        ch, &victim, &obj))
    {    
        /* ths is an object */
        
        if (obj->obj_flags.type_flag != ITEM_CONTAINER)
            send_to_char("That's not a container.\r\n", ch);
        else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
            send_to_char("Maybe you should close it first...\r\n", ch);
        else if (obj->obj_flags.value[2] < 0)
            send_to_char("That thing can't be locked.\r\n", ch);
        else if (!has_key(ch, obj->obj_flags.value[2]))
            send_to_char("You don't seem to have the proper key.\r\n", ch);
        else if (IS_SET(obj->obj_flags.value[1], CONT_LOCKED))
            send_to_char("It is locked already.\r\n", ch);
        else
        {   
            SET_BIT(obj->obj_flags.value[1], CONT_LOCKED);
            send_to_char("*Cluck*\r\n", ch);
            act("$n locks $p - 'cluck', it says.", ch, obj, 0, TO_ROOM, 0);
        }
    }
    else if ((door = find_door(ch, type, dir)) >= 0)
    {
        /* a door, perhaps */
        
        if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
            send_to_char("That's absurd.\r\n", ch);
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
            send_to_char("You have to close it first, I'm afraid.\r\n", ch);
        else if (IS_SET(EXIT(ch, door)->exit_info, EX_BROKEN))
            send_to_char("You cannot lock it, it is broken.\r\n", ch);
        else if (EXIT(ch, door)->key < 0)
            send_to_char("There does not seem to be any keyholes.\r\n", ch);
        else if (!has_key(ch, EXIT(ch, door)->key))
            send_to_char("You don't have the proper key.\r\n", ch);
        else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
            send_to_char("It's already locked!\r\n", ch);
        else
        {   
            SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
            if (EXIT(ch, door)->keyword)
                act("$n locks the $F.", ch, 0,  EXIT(ch, door)->keyword,
                    TO_ROOM, 0);
            else
                act("$n locks the door.", ch, 0, 0, TO_ROOM, 0);
            send_to_char("*Click*\r\n", ch);
            /* now for locking the other side, too */
            if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
            if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
                    if (back->to_room == ch->in_room)
                        SET_BIT(back->exit_info, EX_LOCKED);
        }
    }
    else
      send_to_char("You don't see anything like that.\r\n", ch);
   return eSUCCESS;
}
 
 
int do_unlock(CHAR_DATA *ch, char *argument, int cmd)
{
    int door, other_room;
    char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
    struct room_direction_data *back;
    struct obj_data *obj;
    CHAR_DATA *victim;   
    
    argument_interpreter(argument, type, dir);
    
    if (!*type) {
        send_to_char("Unlock what?\r\n", ch);
	return eFAILURE;
    } else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
        ch, &victim, &obj))
    {
        /* ths is an object */
        
        if (obj->obj_flags.type_flag != ITEM_CONTAINER)
            send_to_char("That's not a container.\r\n", ch);
        else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
            send_to_char("Silly - it ain't even closed!\r\n", ch);
        else if (obj->obj_flags.value[2] < 0)
            send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
        else if (!has_key(ch, obj->obj_flags.value[2]))
            send_to_char("You don't seem to have the proper key.\r\n", ch);
        else if (!IS_SET(obj->obj_flags.value[1], CONT_LOCKED))
            send_to_char("Oh.. it wasn't locked, after all.\r\n", ch);
        else
        {   
            REMOVE_BIT(obj->obj_flags.value[1], CONT_LOCKED);
            send_to_char("*Click*\r\n", ch);
            act("$n unlocks $p.", ch, obj, 0, TO_ROOM, 0);
        }
    }
    else if ((door = find_door(ch, type, dir)) >= 0)
    {
        /* it is a door */
        
        if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
            send_to_char("That's absurd.\r\n", ch);
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
            send_to_char("Heck ... it ain't even closed!\r\n", ch);
        else if (EXIT(ch, door)->key < 0)
            send_to_char("You can't seem to spot any keyholes.\r\n", ch);
        else if (!has_key(ch, EXIT(ch, door)->key))
            send_to_char("You do not have the proper key for that.\r\n", ch);
        else if (IS_SET(EXIT(ch, door)->exit_info, EX_BROKEN))
            send_to_char("You cannot unlock it, it is broken!\r\n", ch);
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
            send_to_char("It's already unlocked, it seems.\r\n", ch);
        else
        {   
            REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
            if (EXIT(ch, door)->keyword)
                act("$n unlocks the $F.", ch, 0, EXIT(ch, door)->keyword,
                    TO_ROOM, 0);
            else
                act("$n unlocks the door.", ch, 0, 0, TO_ROOM, 0);
            send_to_char("*click*\r\n", ch);
            /* now for unlocking the other side, too */
            if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
            if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
                    if (back->to_room == ch->in_room)
                        REMOVE_BIT(back->exit_info, EX_LOCKED);
        }
    }
    else
      send_to_char("You don't see anything like that.\r\n", ch);
    return eSUCCESS;
}

