/************************************************************************
| $Id: inventory.cpp,v 1.54 2005/05/03 22:37:52 shane Exp $
| inventory.C
| Description:  This file contains implementation of inventory-management
|   commands: get, give, put, etc..
|
| Revision history
| 10/17/2003   Onager   Changed do_consent() to fix buffer overflow crash bug
|                       and to only add consented character's name to the
|                       corpse name once (it was getting added on every consent)
| 11/10/2003   Onager   Added check to prevent consenting NPCs, and limited
|                       consented string lenth to 1/2 max string length
*/
extern "C"
{
  #include <ctype.h>
}

#include <connect.h>
#include <character.h>
#include <obj.h>
#include <mobile.h>
#include <room.h>
#include <structs.h>
#include <utility.h>
#include <player.h>
#include <levels.h>
#include <interp.h>
#include <handler.h>
#include <db.h>
#include <act.h>
#include <string.h>
#include <returnvals.h>
#include <spells.h>
#include <clan.h>
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif



/* extern variables */

extern CWorld world;
extern struct index_data *obj_index; 
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct obj_data *object_list;
extern int rev_dir[];

/* extern functions */
void save_corpses(void);
char *fname(char *namelist);
int isname(char *arg, char *arg2);
struct obj_data *create_money( int amount );
int palm  (struct char_data *ch, struct obj_data *obj_object, struct obj_data *sub_object);
void special_log(char *arg);
struct obj_data * bring_type_to_front(char_data * ch, int item_type);
struct obj_data * search_char_for_item(char_data * ch, sh_int item_number);


/* procedures related to get */
void get(struct char_data *ch, struct obj_data *obj_object, struct obj_data *sub_object)
{
    char buffer[MAX_STRING_LENGTH];
   
    if(!sub_object || sub_object->carried_by != ch) 
    {
      // we only have to check for uniqueness if the container is not on the character
      // or if there is no container
      if(IS_SET(obj_object->obj_flags.more_flags, ITEM_UNIQUE)) {
        if(search_char_for_item(ch, obj_object->item_number)) {
           send_to_char("The item's uniqueness prevents it!\r\n", ch);
           return;
        }
      }
      if(contents_cause_unique_problem(obj_object, ch)) {
         send_to_char("Something inside the item is unique and prevents it!\r\n", ch);
         return;
      }
    }

    if (sub_object) {
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
    }

    if((obj_object->obj_flags.type_flag == ITEM_MONEY) && 
	(obj_object->obj_flags.value[0]>=1)) {
	obj_from_char(obj_object);
	sprintf(buffer,"There was %d coins.\n\r",
		obj_object->obj_flags.value[0]);
	send_to_char(buffer,ch);
//	if (sub_object && sub_object->obj_flags.value[3] == 1 && 
//           !isname("pc",sub_object->name) && ch->clan 
//            && get_clan(ch)->tax && !IS_SET(GET_TOGGLES(ch), PLR_NOTAX))
	if (((sub_object && sub_object->obj_flags.value[3] == 1 && 
           !isname("pc",sub_object->name)) || !sub_object) && ch->clan 
            && get_clan(ch)->tax && !IS_SET(GET_TOGGLES(ch), PLR_NOTAX))
	{
	  int cgold = (int)((float)(obj_object->obj_flags.value[0]) * (float)((float)(get_clan(ch)->tax)/100.0));
	  GET_GOLD(ch) += obj_object->obj_flags.value[0] - cgold;
          get_clan(ch)->balance += cgold;
	  csendf(ch,"Your clan taxes you %d gold, leaving %d gold for you.\r\n",cgold, obj_object->obj_flags.value[0]-cgold);
	  save_clans();
	} else
	GET_GOLD(ch) += obj_object->obj_flags.value[0];
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
    bool fail  = FALSE;
    bool has_consent = FALSE;
    int type   = 3;
    bool alldot = FALSE;
    bool inventorycontainer = FALSE;
    char allbuf[MAX_STRING_LENGTH];

    argument_interpreter(argument, arg1, arg2);

    /* get type */
    if (!*arg1) {
	type = 0;
    }
    if (*arg1 && !*arg2) {
      alldot = FALSE;
      allbuf[0] = '\0';
      if ((str_cmp(arg1, "all") != 0) &&
	  (sscanf(arg1, "all.%s", allbuf) != 0)){
	strcpy(arg1, "all");
	alldot = TRUE;
      }
      if (!str_cmp(arg1,"all")) {
	type = 1;
      } else {
	type = 2;
      }
    }
    if (*arg1 && *arg2) {
      alldot = FALSE;
      allbuf[0] = '\0';
      if ((str_cmp(arg1, "all") != 0) &&
	      (sscanf(arg1, "all.%s", allbuf) != 0)){
	    strcpy(arg1, "all");
	    alldot = TRUE;
	  }
      if (!str_cmp(arg1,"all")) {
	if (!str_cmp(arg2,"all")) {
	  type = 3;
	} else {
	  type = 4;
	}
      } else {
	if (!str_cmp(arg2,"all")) {
	  type = 5;
	} else {
	  type = 6;
	}
      }
    }

    if((cmd == 10) && (GET_CLASS(ch) != CLASS_THIEF) &&
      (GET_LEVEL(ch) <= MORTAL)) {
      send_to_char("I bet you think you're a thief.\n\r", ch);
      return eFAILURE; 
    }

    if((cmd == 10) && (type != 2) && (type != 6)) {
      send_to_char("You can only palm objects that are in the same room, "
                   "one at a time.\n\r", ch);
      return eFAILURE; 
    }

    switch (type) {
	/* get */
	case 0:{ 
	    send_to_char("Get what?\n\r", ch); 
	} break;
	/* get all */
	case 1:{ 
            if(ch->in_room == real_room(3099)) {
              send_to_char("Not in the donation room.\n\r", ch);
              return eFAILURE;
            }
	    sub_object = 0;
	    found = FALSE;
	    fail    = FALSE;
	    for(obj_object = world[ch->in_room].contents;
		obj_object;
		obj_object = next_obj) {
		next_obj = obj_object->next_content;

		/* IF all.obj, only get those named "obj" */
		if(alldot && !isname(allbuf, obj_object->name))
		  continue;

                // Can't pick up NO_NOTICE items with 'get all'  only 'all.X' or 'X'
                if(!alldot && IS_SET(obj_object->obj_flags.more_flags, ITEM_NONOTICE) && GET_LEVEL(ch) < IMMORTAL)
                  continue;

                // Ignore NO_TRADE items on a 'get all'
                if(IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && GET_LEVEL(ch) < IMMORTAL) {
                  csendf(ch, "The %s appears to be NO_TRADE so you don't pick it up.\r\n", obj_object->short_description);
                  continue;
                }
		if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
			obj_object->obj_flags.value[0] > 10000 &&
			GET_LEVEL(ch) < 5)
		{
		  send_to_char("You cannot pick up that much money!\r\n",ch);
		  continue;
		}

		if (obj_object->obj_flags.eq_level > 9 && GET_LEVEL(ch) < 5)
		{
		  csendf(ch, "%s is too powerful for you to possess.\r\n", obj_object->short_description);
		  continue;	
		}

                // PC corpse
		if ((obj_object->obj_flags.value[3] == 1 && isname("pc", obj_object->name)) || isname("thiefcorpse", obj_object->name))
                {
                   sprintf(buffer, "%s_consent", GET_NAME(ch));
		   if( (isname("thiefcorpse", obj_object->name) &&
			!isname(GET_NAME(ch), obj_object->name)) || isname(GET_NAME(ch), obj_object->name) || GET_LEVEL(ch) >= OVERSEER)
                     has_consent = TRUE;
		   if(!has_consent && !isname(GET_NAME(ch), obj_object->name)) {
                     if (GET_LEVEL(ch) < OVERSEER) {
		       send_to_char("You don't have consent to take the corpse.\n\r", ch);
		       continue;
                     }
                   }
                   if(has_consent && contains_no_trade_item(obj_object)) {
                     if (GET_LEVEL(ch) < OVERSEER) {
                       send_to_char("This item contains no_trade items that cannot be picked up.\n\r", ch);
		       has_consent = FALSE; // bugfix, could loot without consent
                       continue;
                     }
                   }
                   if (GET_LEVEL(ch) < OVERSEER)
                     has_consent = FALSE;  // reset it for the next item:P
                   else
                     has_consent = TRUE;  // reset it for the next item:P
		}
		
		if (CAN_SEE_OBJ(ch, obj_object) && GET_LEVEL(ch) < IMMORTAL) 
		{
                    // Don't bother checking this if item is gold coins.
		    if ((IS_CARRYING_N(ch) + 1) > CAN_CARRY_N(ch) &&
                        !( GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && GET_LEVEL(ch) < IMMORTAL)
                       ) 
		    {
			sprintf(buffer, "%s : You can't carry that many items.\n\r", fname(obj_object->name));
			send_to_char(buffer, ch);
			fail = TRUE;
		    } 
                    else if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) > CAN_CARRY_W(ch) && GET_LEVEL(ch) < IMMORTAL) 
                    {
			sprintf(buffer, "%s : You can't carry that much weight.\n\r", fname(obj_object->name));
			send_to_char(buffer, ch);
			fail = TRUE;
		    }
		    else if (CAN_WEAR(obj_object,ITEM_TAKE)) 
		    {
			get(ch,obj_object,sub_object);
			found = TRUE;
		    } else 
		    {
			send_to_char("You can't take that.\n\r", ch);
			fail = TRUE;
		    }
		} else if (CAN_SEE_OBJ(ch, obj_object) && GET_LEVEL(ch) >= IMMORTAL && CAN_WEAR(obj_object,ITEM_TAKE)) {
		    get(ch,obj_object,sub_object);
                    found = TRUE;
                }
	    } // of for loop
	    if (found) {
		send_to_char("OK.\n\r", ch);
                 do_save(ch,"", 666);
	    } else {
		if (!fail) send_to_char("You see nothing here.\n\r", ch);
	    }
	} break;
	/* get ??? */
	case 2:{
	    sub_object = 0;
	    found = FALSE;
	    fail    = FALSE;
	    obj_object = get_obj_in_list_vis(ch, arg1, 
		world[ch->in_room].contents);
	    if (obj_object) {
	        if(obj_object->obj_flags.type_flag == ITEM_CONTAINER && 
                   obj_object->obj_flags.value[3] == 1 &&
		   isname("pc", obj_object->name)) 
                {
                   sprintf(buffer, "%s_consent", GET_NAME(ch));
		   if(isname(GET_NAME(ch), obj_object->name))
                     has_consent = TRUE;
		   if(!has_consent && !isname(GET_NAME(ch), obj_object->name)) {
		     send_to_char("You don't have consent to take the "
		               "corpse.\n\r", ch);
		     return eFAILURE;
                   }
                   has_consent = FALSE;  // reset it
                }
		if( (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                   !( GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && GET_LEVEL(ch) < IMMORTAL)
                  )
                {
		    sprintf(buffer, "%s : You can't carry that many items.\n\r", fname(obj_object->name));
		    send_to_char(buffer, ch);
		    fail = TRUE;
		} else if((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) > 
			CAN_CARRY_W(ch) && GET_LEVEL(ch) < IMMORTAL) 
                {
		    sprintf(buffer,"%s : You can't carry that much weight.\n\r", fname(obj_object->name));
		    send_to_char(buffer, ch);
		    fail = TRUE;
		}
                else if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
                        obj_object->obj_flags.value[0] > 10000 &&
			GET_LEVEL(ch) < 5)
                {
                  send_to_char("You cannot pick up that much money!\r\n",ch);
		fail = TRUE;
                }

		else if (obj_object->obj_flags.eq_level > 19 && GET_LEVEL(ch) < 5)
		{
                  if(ch->in_room != real_room(3099)) {
		     csendf(ch, "%s is too powerful for you to possess.\r\n", obj_object->short_description);
		     fail = TRUE;	
                  } else {
                     csendf(ch, "The aura of the donation room allows you to pick up %s.\n\r", obj_object->short_description);
                     get (ch, obj_object, sub_object);
                     do_save(ch,"", 666);
                     found = TRUE;
                  }
		} else if (CAN_WEAR(obj_object,ITEM_TAKE)) 
                {
                    if(cmd == 10) palm(ch, obj_object, sub_object);
		    else          get (ch, obj_object, sub_object);
                    do_save(ch,"", 666);
		    found = TRUE;
		} else {
		    send_to_char("You can't take that.\n\r", ch);
		    fail = TRUE;
		}
	    } else {
		sprintf(buffer,"You do not see a %s here.\n\r", arg1);
		send_to_char(buffer, ch);
		fail = TRUE;
	    }
	} break;
	/* get all all */
	case 3:{ 
	    send_to_char("You must be joking?!\n\r", ch);
	} break;
	/* get all ??? */
	case 4:{
	    found = FALSE;
	    fail    = FALSE; 
	    sub_object = get_obj_in_list_vis(ch, arg2, 
		world[ch->in_room].contents);
	    if (!sub_object) {
		sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying);
                inventorycontainer = TRUE;
            }

	    if (sub_object) {
	        if(sub_object->obj_flags.type_flag == ITEM_CONTAINER && 
                   ((sub_object->obj_flags.value[3] == 1 &&
		   isname("pc", sub_object->name))||isname("thiefcorpse",sub_object->name))) 
                {
                   sprintf(buffer, "%s_consent", GET_NAME(ch));
		   if((isname("thiefcorpse", sub_object->name) && !isname(GET_NAME(ch), sub_object->name) ) || isname(buffer, sub_object->name) || GET_LEVEL(ch) > 105)
                     has_consent = TRUE;
		   if(!has_consent && !isname(GET_NAME(ch), sub_object->name)) {
		     send_to_char("You don't have consent to touch the corpse.\n\r", ch);
		     return eFAILURE;
                   }
                }
		if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER
		|| GET_ITEM_TYPE(sub_object) == ITEM_ALTAR) {
		  if (IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED)){
		    sprintf(buffer, "The %s is closed.\n\r",fname(sub_object->name));
		    send_to_char(buffer, ch);
		    return eFAILURE;
		  }
		  for(obj_object = sub_object->contains;
		      obj_object;
		      obj_object = next_obj) {
		    next_obj = obj_object->next_content;
		if (GET_ITEM_TYPE(obj_object) == ITEM_CONTAINER && contains_no_trade_item(obj_object))
		{
                  csendf(ch, "%s : It seems magically attached to the corpse.\n\r", fname(obj_object->name));
		  continue;
		}
		    /* IF all.obj, only get those named "obj" */
		    if (alldot && !isname(allbuf,obj_object->name)){
		      continue;
		    }

                    // Ignore NO_TRADE items on a 'get all'
                    if(IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) && GET_LEVEL(ch) < 100) {
                      csendf(ch, "The %s appears to be NO_TRADE so you don't pick it up.\r\n", obj_object->short_description);
                      continue;
                    }

		    if (CAN_SEE_OBJ(ch,obj_object)) 
                    {
		      if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                           !( GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && GET_LEVEL(ch) < IMMORTAL)
                         )
                      {
		        sprintf(buffer,"%s : You can't carry that many items.\n\r", fname(obj_object->name));
		        send_to_char(buffer, ch);
		        fail = TRUE;
		      } else 
                      {
		        if (inventorycontainer || 
                            (IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < CAN_CARRY_W(ch) ||
                            GET_LEVEL(ch) > IMMORTAL) 
                        {
                          if(has_consent && IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE)) {
                            // if I have consent and i'm touching the corpse, then I shouldn't be able
                            // to pick up no_trade items because it is someone else's corpse.  If I am
                            // the other of the corpse, has_consent will be false.
                            if (GET_LEVEL(ch) < 100) {
                              csendf(ch, "%s : It seems magically attached to the corpse.\n\r", fname(obj_object->name));
                              continue;
                            }
                          }
               if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
                        obj_object->obj_flags.value[0] > 10000 &&
                        GET_LEVEL(ch) < 5)
                {
                  send_to_char("You cannot pick up that much money!\r\n",ch);
                  continue;
                }

			if (sub_object->carried_by != ch &&obj_object->obj_flags.eq_level > 9 && GET_LEVEL(ch) < 5)
 			{
			  csendf(ch, "%s is too powerful for you to possess.\r\n", obj_object->short_description);
			  continue;	
			}

		          if (CAN_WEAR(obj_object,ITEM_TAKE)) {
			    get(ch,obj_object,sub_object);
			    found = TRUE;
		          } else {
			    send_to_char("You can't take that.\n\r", ch);
			    fail = TRUE;
		          }
		        } else {
		          sprintf(buffer,
		            "%s : You can't carry that much weight.\n\r", 
			    fname(obj_object->name));
		          send_to_char(buffer, ch);
		          fail = TRUE;
		        }
		      }
		    }
		  }
		  if (!found && !fail) {
		    sprintf(buffer,"You do not see anything in the %s.\n\r", 
			fname(sub_object->name));
		    send_to_char(buffer, ch);
		    fail = TRUE;
		  }
		} else {
		  sprintf(buffer,"The %s is not a container.\n\r",
		      fname(sub_object->name));
		  send_to_char(buffer, ch);
		  fail = TRUE;
		}
	      } else { 
		sprintf(buffer,"You do not see or have the %s.\n\r", arg2);
		send_to_char(buffer, ch);
		fail = TRUE;
	      }
	    } break;
	case 5:{ 
	  send_to_char(
	    "You can't take a thing from more than one container.\n\r", ch);
	} break;
	case 6:{
	  found = FALSE;
	  fail  = FALSE;
	  sub_object = get_obj_in_list_vis(ch, arg2, 
			   world[ch->in_room].contents);
	  if (!sub_object){
	    sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying);
            inventorycontainer = TRUE;
	  }
	  if(sub_object) {
	    if(sub_object->obj_flags.type_flag == ITEM_CONTAINER && 
               ((sub_object->obj_flags.value[3] == 1 &&
	       isname("pc", sub_object->name)) || isname("thiefcorpse",sub_object->name))) 
            {
               sprintf(buffer, "%s_consent", GET_NAME(ch));
	       if((isname("thiefcorpse", sub_object->name) && !isname(GET_NAME(ch), sub_object->name)) || isname(buffer, sub_object->name))
                 has_consent = TRUE;
	       if(!has_consent && !isname(GET_NAME(ch), sub_object->name)) {
		 send_to_char("You don't have consent to touch the "
		               "corpse.\n\r", ch);
	         return eFAILURE;
               }
            }
	    if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER ||
		GET_ITEM_TYPE(sub_object) == ITEM_ALTAR) 
            {
	      if (IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED)){
	        sprintf(buffer,"The %s is closed.\n\r", fname(sub_object->name));
	        send_to_char(buffer, ch);
	        return eFAILURE;
	      }
	      obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains);
	      if (obj_object) 
              {
                if (GET_ITEM_TYPE(obj_object) == ITEM_MONEY &&
                        obj_object->obj_flags.value[0] > 10000 &&
                        GET_LEVEL(ch) < 5)
                {
                  send_to_char("You cannot pick up that much money!\r\n",ch);
                  fail = TRUE;
                }

		else if (sub_object->carried_by != ch && obj_object->obj_flags.eq_level 
> 9 && GET_LEVEL(ch) < 5)
		{
		  csendf(ch, "%s is too powerful for you to possess.\r\n", obj_object->short_description);
		 fail = TRUE;	
		}

	        else if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) &&
                    !( GET_ITEM_TYPE(obj_object) == ITEM_MONEY && obj_object->item_number == -1 && GET_LEVEL(ch) < IMMORTAL)
                   )
                { 
                  sprintf(buffer,"%s : You can't carry that many items.\n\r",fname(obj_object->name));
                  send_to_char(buffer, ch);
	          fail = TRUE;
	        } else if (inventorycontainer || 
                     (IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < CAN_CARRY_W(ch)) 
                {
                    if(has_consent && ( IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE) || 
                                        contains_no_trade_item(obj_object) ) ) 
                    {
                  // if I have consent and i'm touching the corpse, then I shouldn't be able
                  // to pick up no_trade items because it is someone else's corpse.  If I am
                  // the other of the corpse, has_consent will be false.
                      if (GET_LEVEL(ch) < 100) {
                        csendf(ch, "%s : It seems magically attached to the corpse.\n\r", fname(obj_object->name));
                        fail = TRUE;
                      }
                    }
		    else if (CAN_WEAR(obj_object,ITEM_TAKE)) 
                    {
                      if(cmd == 10) palm(ch, obj_object, sub_object);
		      else          get (ch, obj_object, sub_object);
		      found = TRUE;
		    } else {
		      send_to_char("You can't take that.\n\r", ch);
		      fail = TRUE;
		    }
	        } else {
		    sprintf(buffer,"%s : You can't carry that much weight.\n\r", fname(obj_object->name));
		    send_to_char(buffer, ch);
		    fail = TRUE;
	        }
	      } else {
	        sprintf(buffer,"The %s does not contain the %s.\n\r", 
		fname(sub_object->name), arg1);
	        send_to_char(buffer, ch);
	        fail = TRUE;
	      }
	    } else {
	      sprintf(buffer,
	      "The %s is not a container.\n\r", fname(sub_object->name));
	      send_to_char(buffer, ch);
	      fail = TRUE;
	    }
	  } else {
	    sprintf(buffer,"You do not see or have the %s.\n\r", arg2);
	    send_to_char(buffer, ch);
	    fail = TRUE;
	  }
	} break;
      }
      if(fail)
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
    send_to_char("Give WHO consent to touch your rotting carcass?\n\r", ch);
    return eFAILURE;
  }
 
  if(!(vict = get_char_vis(ch, buf))) {
    csendf(ch, "Consent who?  You can't see any %s.\n\r", buf);
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
    send_to_char("Now what business would THAT thing have with your mortal remains?\n\r", ch);
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
                    "your\n\rmaggot-ridden corpse already?\n\r", ch);
       return eFAILURE;
     }
     sprintf(buf2, "%s %s_consent", obj->name, buf);
     obj->name = str_hsh(buf2);
  }
  
  sprintf(buf2, "All corpses in the game which belong to you can now be "
          "molested by\n\ranyone named %s.\n\r", buf);
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
        search_char_for_item(vict, inside->item_number))
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
  bool test = FALSE;

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  argument = one_argument(argument, arg);

  if(is_number(arg)) {
    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eFAILURE;
    }

    if(strlen(arg) > 7) {
      send_to_char("Number field too big.\n\r", ch);
      return eFAILURE;
    }
    amount = atoi(arg);
    argument=one_argument(argument,arg);
    if(str_cmp("coins",arg) && str_cmp("coin",arg)) {
      send_to_char("Sorry, you can't do that (yet)...\n\r", ch);
      return eFAILURE;
    }
    if(amount < 0) {
      send_to_char("Sorry, you can't do that!\n\r",ch);
      return eFAILURE;
    }
    if(GET_GOLD(ch) < (uint32)amount) {
      send_to_char("You haven't got that many coins!\n\r",ch);
      return eFAILURE;
    }
    send_to_char("OK.\n\r",ch);
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
    if(!str_cmp(arg,"all")) {
      for(tmp_object = ch->carrying; tmp_object; tmp_object = next_obj) {
         next_obj = tmp_object->next_content;

         if(IS_SET(tmp_object->obj_flags.extra_flags, ITEM_SPECIAL))
           continue;

      if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) {
        send_to_char("Your criminal acts prohibit it.\n\r", ch);
        return eFAILURE;
      }
         if(IS_SET(tmp_object->obj_flags.more_flags, ITEM_NO_TRADE)) 
           continue;
         if(contains_no_trade_item(tmp_object))
           continue;
         if(!IS_SET(tmp_object->obj_flags.extra_flags, ITEM_NODROP) ||
            GET_LEVEL(ch) >= IMMORTAL) {
           if(IS_SET(tmp_object->obj_flags.extra_flags, ITEM_NODROP))
             send_to_char("(This item is cursed, BTW.)\n\r", ch);
           if(CAN_SEE_OBJ(ch, tmp_object)) {
             sprintf(buffer, "You drop the %s.\n\r", fname(tmp_object->name));
             send_to_char(buffer, ch);
           }
           else
             send_to_char("You drop something.\n\r", ch);

           act("$n drops $p.", ch, tmp_object, 0, TO_ROOM, INVIS_NULL);
           move_obj(tmp_object, ch->in_room);
           test = TRUE;
         }
         else {
           if(CAN_SEE_OBJ(ch, tmp_object)) {
             sprintf(buffer, "You can't drop the %s, it must be CURSED!\n\r",
		     fname(tmp_object->name));
             send_to_char(buffer, ch);
             test = TRUE;
           }
         }
      } /* for */

      if(!test)
        send_to_char("You do not seem to have anything.\n\r", ch);

    } /* if strcmp "all" */

    else {
      tmp_object = get_obj_in_list_vis(ch, arg, ch->carrying);
      if(tmp_object) {

      if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) {
        send_to_char("Your criminal acts prohibit it.\n\r", ch);
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
	  send_to_char("Don't be a dork.\n\r", ch);
	  return eFAILURE;
	}
        else if(!IS_SET(tmp_object->obj_flags.extra_flags, ITEM_NODROP) ||
            GET_LEVEL(ch) >= IMMORTAL) {
          if(IS_SET(tmp_object->obj_flags.extra_flags, ITEM_NODROP))
            send_to_char("(This item is cursed, BTW.)\n\r", ch);
          sprintf(buffer, "You drop the %s.\n\r", fname(tmp_object->name));
          send_to_char(buffer, ch);
          act("$n drops $p.", ch, tmp_object, 0, TO_ROOM, INVIS_NULL);

          move_obj(tmp_object, ch->in_room);
          return eSUCCESS;
        }
        else
	  send_to_char("You can't drop it, it must be CURSED!\n\r", ch);
      }
      else
        send_to_char("You do not have that item.\n\r", ch);

    }
    do_save(ch, "", 666);
  }
  else
    send_to_char("Drop what?\n\r", ch);
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
     if(!name) {
       sprintf(buf, "%s %s", fname(tmp_object->name), target);
       buf[99] = 0;
       found = TRUE;
       do_put(ch, buf, cmd);
     }
     else if(isname(name, tmp_object->name)) {
       sprintf(buf, "%s %s", name, target);
       buf[99] = 0;
       found = TRUE;
       do_put(ch, buf, cmd);
     }
  }

  if(!found)
    send_to_char("You don't have one.\n\r", ch);
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

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n",ch);
    return eFAILURE;
  }

  argument_interpreter(argument, arg1, arg2);

  if(*arg1) {
    if(*arg2) {
      if(!(get_obj_in_list_vis(ch, arg2, ch->carrying))
	&& !(get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents))) {
        sprintf(buffer, "You don't have a %s.\n\r", arg2);
        send_to_char(buffer, ch);
        return 1;
      }
      allbuf[0] = '\0';
      if(!str_cmp(arg1, "all")) {
        do_putalldot(ch, 0, arg2, cmd);
        return eSUCCESS;
      }
      else if(sscanf(arg1, "all.%s", allbuf) != 0) {
        do_putalldot(ch, allbuf, arg2, cmd);
        return eSUCCESS;
      }
      obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);

      if(obj_object) {
        if(IS_SET(obj_object->obj_flags.extra_flags, ITEM_NODROP)) {
          if(GET_LEVEL(ch) < IMMORTAL) {
            send_to_char("You are unable to! That item must be CURSED!\n\r", ch);
            return eFAILURE;
          }
          else
            send_to_char("(This item is cursed, BTW.)\n\r", ch);
        } 
	if (IS_SET(obj_object->obj_flags.extra_flags, ITEM_NEWBIE)) {
	  send_to_char("The protective enchantment this item holds cannot be held within this container.\r\n",ch);
	  return eFAILURE;
	}
	if(GET_ITEM_TYPE(obj_object) == ITEM_CONTAINER) {
	  send_to_char("You would ruin it!\n\r", ch);
	  return eFAILURE;
	}

        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM,
                            ch, &tmp_char, &sub_object);
        if(sub_object) {
          if(GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER || GET_ITEM_TYPE(sub_object) == ITEM_ALTAR) {
	    if (GET_ITEM_TYPE(sub_object) == ITEM_ALTAR && GET_ITEM_TYPE(obj_object) != ITEM_TOTEM) {
               send_to_char("You cannot put that in an altar.\r\n",ch);
               return eFAILURE;
            }
	    if(!IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED)) {
	      if(obj_object == sub_object) {
		send_to_char("You attempt to fold it into itself, but fail.\n\r", ch);
		return eFAILURE;
	      }
              if(IS_SET(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
               !IS_SET(sub_object->obj_flags.extra_flags, ITEM_SPECIAL))
              {
                send_to_char("Are you crazy?!  Someone could steal it!\r\n", ch);
                return eFAILURE;
              }
	      if (((sub_object->obj_flags.weight) + 
               (obj_object->obj_flags.weight)) <
               (sub_object->obj_flags.value[0]) &&
               (obj_index[sub_object->item_number].virt != 536 || 
               weight_in(sub_object) + obj_object->obj_flags.weight < 200))
              {
		send_to_char("Ok.\n\r", ch);
		if(bits == FIND_OBJ_INV) {
		  obj_from_char(obj_object);
		  /* make up for above line */
		  if (obj_index[sub_object->item_number].virt != 536)
  		     IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj_object);
		  obj_to_obj(obj_object, sub_object);
		}
                else
		  move_obj(obj_object, sub_object);
		
		act("$n puts $p in $P", ch, obj_object, sub_object, TO_ROOM, INVIS_NULL);
                return eSUCCESS;
	      }
              else {
		send_to_char("It won't fit.\n\r", ch);
	      }
	    }
            else
	      send_to_char("It seems to be closed.\n\r", ch);
          } else {
	     sprintf(buffer, "The %s is not a container.\n\r", fname(sub_object->name));
	     send_to_char(buffer, ch);
	  }
        } else {
           sprintf(buffer, "You dont have the %s.\n\r", arg2);
           send_to_char(buffer, ch);
        }
      } else {
        sprintf(buffer, "You dont have the %s.\n\r", arg1);
        send_to_char(buffer, ch);
      }
    } /* if arg2 */
    else {
       sprintf(buffer, "Put %s in what?\n\r", arg1);
       send_to_char(buffer, ch);
    }
  } /* if arg1 */
  else {
     send_to_char("Put what in what?\n\r",ch);
  }
  return eFAILURE;
}

int do_give(struct char_data *ch, char *argument, int cmd)
{
  char obj_name[MAX_INPUT_LENGTH+1], vict_name[MAX_INPUT_LENGTH+1], buf[200];
  char arg[80];
  int amount;
  int retval;
  extern int arena[4];
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
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eFAILURE;
    }

    if(strlen(obj_name) > 7) {
      send_to_char("Number field too large.\n\r", ch);
      return eFAILURE;
    }
    amount = atoi(obj_name);
    argument = one_argument(argument, arg);
    if(str_cmp("coins",arg) && str_cmp("coin",arg)) { 
      send_to_char("Sorry, you can't do that (yet)...\n\r",ch);
      return eFAILURE;
    }
    if(amount < 0) { 
      send_to_char("Sorry, you can't do that!\n\r",ch);
      return eFAILURE;
    }
    if(GET_GOLD(ch) < (uint32)amount && GET_LEVEL(ch) < DEITY) 
    {
      send_to_char("You haven't got that many coins!\n\r",ch);
      return eFAILURE;
    }
    argument = one_argument(argument, vict_name);
    if(!*vict_name) { 
      send_to_char("To who?\n\r",ch);
      return eFAILURE;
    }
    
      if (!(vict = get_char_room_vis(ch, vict_name))) {
	 send_to_char("To who?\n\r",ch);
	 return eFAILURE;
	 }
      
      if(ch == vict) {
         send_to_char("Umm okay, you give it to yourself.\r\n", ch);
         return eFAILURE;
      }

      if(GET_GOLD(vict) > 2000000000) {
         send_to_char("They can't hold that much gold!\r\n", ch);
         return eFAILURE;
      }

      csendf(ch, "You give %ld coin%s to %s.\n\r", (long) amount,
             amount == 1 ? "" : "s", GET_SHORT(vict));

      if(GET_LEVEL(ch) >= IMMORTAL || IS_NPC(ch)) { 
        sprintf(buf, "%s gives %d coins to %s", GET_NAME(ch), amount,
                GET_NAME(vict));
        log(buf, 110, LOG_PLAYER);
      }
      
      sprintf(buf, "%s gives you %d gold coin%s.\n\r", PERS(ch, vict), amount,
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
           sprintf(buf, "%s gives %d coins to %s (negative!)", GET_NAME(ch), 
                   amount, GET_NAME(vict));
           special_log(buf);
         }
      }
      GET_GOLD(vict) += amount;
      do_save(ch, "", 10);
      do_save(vict, "", 10);
      // bribe trigger automatically removes any gold given to mob
      mprog_bribe_trigger( vict, ch, amount );

      return eSUCCESS;
    }

    argument = one_argument(argument, vict_name);

    if (!*obj_name || !*vict_name)
    {
	send_to_char("Give what to who?\n\r", ch);
	return eFAILURE;
    }
    if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
    {
	send_to_char("You do not seem to have anything like that.\n\r",
	   ch);
	return eFAILURE;
    }
    if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) && GET_LEVEL(ch) < OVERSEER) {
      send_to_char("That sure would be a fucking stupid thing to do.\n\r", ch);
      return eFAILURE;
    }

    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eFAILURE;
    }

    if (IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP))
    {
      if(GET_LEVEL(ch) < DEITY)
      {
	send_to_char("You can't let go of it! Yeech!!\n\r", ch);
	return eFAILURE;
      }
      else
        send_to_char("This item is NODROP btw.\n\r", ch);
    }

    if (!(vict = get_char_room_vis(ch, vict_name)))
    {
	send_to_char("No one by that name around here.\n\r", ch);
	return eFAILURE;
    }

    if (ch == vict)
    {
       send_to_char("Why give yourself stuff?\r\n", ch);
       return eFAILURE;
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

    if (IS_NPC(vict) && mob_index[vict->mobdata->nr].non_combat_func == shop_keeper)
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
      send_to_char("Now WHY would a thief give something to a linkdead char..?\n\r", ch);
      return eFAILURE;
    }

    if ((1+IS_CARRYING_N(vict)) > CAN_CARRY_N(vict))
    {
       if((ch->in_room >= 0 && ch->in_room <= top_of_world) && !strcmp(obj_name, "potato") &&
          IS_SET(world[ch->in_room].room_flags, ARENA) && IS_SET(world[vict->in_room].room_flags, ARENA) &&
          arena[2] == -3) {
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
         arena[2] == -3) {
         ;
      } else {
        act("$E can't carry that much weight.", ch, 0, vict, TO_CHAR, 0);
        return eFAILURE;
      } 
    }
    if(IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE)) {
      if(search_char_for_item(vict, obj->item_number)) {
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

    if(GET_LEVEL(ch) >= IMMORTAL)
    {
    sprintf(buf, "%s gives %s to %s.", GET_NAME(ch), obj->short_description,
            GET_NAME(vict));
    special_log(buf);
    }
    if(IS_NPC(ch) && (!IS_AFFECTED(ch, AFF_CHARM) || GET_LEVEL(ch) > 50))
    {
       sprintf(buf, "%s gives %s to %s.", GET_NAME(ch), obj->short_description, 
                    GET_NAME(vict));
       special_log(buf);
    }

    move_obj(obj, vict);
    act("$n gives $p to $N.", ch, obj, vict, TO_ROOM, INVIS_NULL|NOTVICT);
    act("$n gives you $p.", ch, obj, vict, TO_VICT, 0);
    act("You give $p to $N.", ch, obj, vict, TO_CHAR, 0);

    if((vict->in_room >= 0 && vict->in_room <= top_of_world) && GET_LEVEL(vict) < IMMORTAL && 
      IS_SET(world[vict->in_room].room_flags, ARENA) && arena[2] == -3 && obj_index[obj->item_number].virt == 393) {
      send_to_char("Here, have some for some potato lag!!\n\r", vict);
      WAIT_STATE(vict, PULSE_VIOLENCE *2);
    }

//    send_to_char("Ok.\n\r", ch);
    do_save(ch,"",10);
    do_save(vict,"",10);
    // if I gave a no_trade item to a mob, the mob needs to destroy it
    // otherwise it defeats the purpose of no_trade:)
    if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) && IS_NPC(vict))
    {
       extract_obj(obj);
	return eSUCCESS;
    }


    retval = mprog_give_trigger( vict, ch, obj );
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
  struct obj_data *i = NULL;
  struct obj_data *j = NULL;
 
  for(i = ch->carrying; i ; i = i->next_content) {
    if(GET_ITEM_TYPE(i) == item_type)
      return i;
    if(GET_ITEM_TYPE(i) == ITEM_CONTAINER && !IS_SET(i->obj_flags.value[1], CONT_CLOSED)) { // search inside if open
  //  if(GET_ITEM_TYPE(i) == ITEM_CONTAINER) { // search inside if open
      for(j = i->contains; j ; j = j->next_content) {
        if(GET_ITEM_TYPE(j) == item_type) {
          get(ch, j, i);
          return j;
        }
      }
    }
  }
  return NULL;
}

// Find an item on a character
struct obj_data * search_char_for_item(char_data * ch, sh_int item_number)
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

int find_door(CHAR_DATA *ch, char *type, char *dir)
{
    char buf[MAX_STRING_LENGTH];
    int door;
    char *dirs[] =
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
            send_to_char("That's not a direction.\n\r", ch);
            return(-1);
        }
     
        if (EXIT(ch, door))
            if (EXIT(ch, door)->keyword)
                if (isname(type, EXIT(ch, door)->keyword))
                    return(door);
                else
                {
                    sprintf(buf, "There is no %s there.\n\r", type);
                    send_to_char(buf, ch);
                    return(-1);
                }
            else
                return(door);
        else
        {   
            sprintf(buf, "There is no %s there.\n\r", type);
            send_to_char(buf, ch);
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
                        
        sprintf(buf, "I see no %s here.\n\r", type);
        send_to_char(buf, ch);
        return(-1);
    }
}


int do_open(CHAR_DATA *ch, char *argument, int cmd)
{
   int door, other_room, retval;
   char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
   struct room_direction_data *back;
   struct obj_data *obj;
   CHAR_DATA *victim;
   CHAR_DATA *next_vict;
            
   int do_fall(CHAR_DATA *ch, short dir);
            
   retval = 0;
         
   argument_interpreter(argument, type, dir);
            
   if (!*type)
      send_to_char("Open what?\n\r", ch);
   else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
   {
      // this is an object
      if (obj->obj_flags.type_flag != ITEM_CONTAINER)
         send_to_char("That's not a container.\n\r", ch);
      else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
         send_to_char("But it's already open!\n\r", ch);
      else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE))
         send_to_char("You can't do that.\n\r", ch);
      else if (IS_SET(obj->obj_flags.value[1], CONT_LOCKED))
         send_to_char("It seems to be locked.\n\r", ch);   
      else
      {   
         REMOVE_BIT(obj->obj_flags.value[1], CONT_CLOSED);
         send_to_char("Ok.\n\r", ch);
         act("$n opens $p.", ch, obj, 0, TO_ROOM, 0);
      }
   }
   else if ((door = find_door(ch, type, dir)) >= 0)
   { 
      if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
         send_to_char("That's impossible, I'm afraid.\n\r", ch);
      else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
         send_to_char("It's already open!\n\r", ch);
      else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
         send_to_char("It seems to be locked.\n\r", ch);   
      else if (IS_SET(EXIT(ch, door)->exit_info, EX_IMM_ONLY) && GET_LEVEL(ch) < IMMORTAL)
         send_to_char("It seems to slither and resist your attempt to touch it.\n\r", ch);
      else
      {   
         REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
            
         if(IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN)) {
            if (EXIT(ch, door)->keyword) {
               act("$n reveals a hidden $F!", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
               csendf(ch, "You reveal a hidden %s!\n\r", fname((char *)EXIT(ch, door)->keyword));
            }
            else {
               act("$n reveals a hidden door!",ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
               send_to_char("You reveal a hidden door!\n\r", ch);
            }
         }  
         else {
           if (EXIT(ch, door)->keyword)
              act("$n opens the $F.", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
           else
              act("$n opens the door.", ch, 0, 0, TO_ROOM, 0);
              send_to_char("Ok.\n\r", ch);
         }
             
         /* now for opening the OTHER side of the door! */
         if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
            if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
               if (back->to_room == ch->in_room)
               {
                  REMOVE_BIT(back->exit_info, EX_CLOSED);
                  if ((back->keyword) && !IS_SET(world[EXIT(ch, door)->to_room].room_flags, QUIET))
                  {
                     sprintf(buf, "The %s is opened from the other side.\n\r",
                                fname(back->keyword));
                     send_to_room(buf, EXIT(ch, door)->to_room);
                  }
                  else
                     send_to_room("The door is opened from the other side.\n\r",
                                EXIT(ch, door)->to_room);
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
                  send_to_room("With the door no longer closed for support, this area's strange gravity takes over!\r\n", victim->in_room);
                  success = 1;
               }
               if(victim == ch)
                  retval = do_fall(victim, door);
               else do_fall(victim, door);
            }
         }
      }
   }
   // in case ch died or anything
   if(retval)
      return retval;
   return eSUCCESS;
}

int do_close(CHAR_DATA *ch, char *argument, int cmd)
{
   int door, other_room;
   char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
   struct room_direction_data *back;
   struct obj_data *obj;
   CHAR_DATA *victim;   
    
   argument_interpreter(argument, type, dir);
    
   if (!*type)
      send_to_char("Close what?\n\r", ch);
   else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
   {     
      if (obj->obj_flags.type_flag != ITEM_CONTAINER)
         send_to_char("That's not a container.\n\r", ch);
      else if (IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
         send_to_char("But it's already closed!\n\r", ch); 
      else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE))
         send_to_char("That's impossible.\n\r", ch);
      else
      {   
         SET_BIT(obj->obj_flags.value[1], CONT_CLOSED);
         send_to_char("Ok.\n\r", ch);
         act("$n closes $p.", ch, obj, 0, TO_ROOM, 0);
      }
   }
   else if ((door = find_door(ch, type, dir)) >= 0)
   {    
      if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
         send_to_char("That's absurd.\n\r", ch);
      else if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
         send_to_char("It's already closed!\n\r", ch);
      else
      {   
         SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
         if (EXIT(ch, door)->keyword)
            act("$n closes the $F.", ch, 0, EXIT(ch, door)->keyword, TO_ROOM, 0);
         else
            act("$n closes the door.", ch, 0, 0, TO_ROOM, 0);
         send_to_char("Ok.\n\r", ch);
         /* now for closing the other side, too */
         if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
            if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
               if (back->to_room == ch->in_room)
               {
                  SET_BIT(back->exit_info, EX_CLOSED);
                  if ((back->keyword) &&
                       !IS_SET(world[EXIT(ch, door)->to_room].room_flags, QUIET))
                  {
                     sprintf(buf, "The %s closes quietly.\n\r",
                                   fname(back->keyword));
                     send_to_room(buf, EXIT(ch, door)->to_room);
                  }
                  else
                     send_to_room("The door closes quietly.\n\r",
                                       EXIT(ch, door)->to_room);
               }
      }
   }
   return eSUCCESS;
}


int has_key(CHAR_DATA *ch, int key)
{
    struct obj_data *o;
    
    if (ch->equipment[HOLD]) {
        if (obj_index[ch->equipment[HOLD]->item_number].virt == key)
            return(1);
    }
     
    for (o = ch->carrying; o; o = o->next_content)
        if (obj_index[o->item_number].virt == key)
            return(1);
            
    return(0);
}

int do_lock(CHAR_DATA *ch, char *argument, int cmd)
{
    int door, other_room;
    char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
    struct room_direction_data *back;
    struct obj_data *obj;
    CHAR_DATA *victim;   
    
    argument_interpreter(argument, type, dir);
    
    if (!*type)
        send_to_char("Lock what?\n\r", ch);
    else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
        ch, &victim, &obj))
    {    
        /* ths is an object */
        
        if (obj->obj_flags.type_flag != ITEM_CONTAINER)
            send_to_char("That's not a container.\n\r", ch);
        else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
            send_to_char("Maybe you should close it first...\n\r", ch);
        else if (obj->obj_flags.value[2] < 0)
            send_to_char("That thing can't be locked.\n\r", ch);
        else if (!has_key(ch, obj->obj_flags.value[2]))
            send_to_char("You don't seem to have the proper key.\n\r", ch);
        else if (IS_SET(obj->obj_flags.value[1], CONT_LOCKED))
            send_to_char("It is locked already.\n\r", ch);
        else
        {   
            SET_BIT(obj->obj_flags.value[1], CONT_LOCKED);
            send_to_char("*Cluck*\n\r", ch);
            act("$n locks $p - 'cluck', it says.", ch, obj, 0, TO_ROOM, 0);
        }
    }
    else if ((door = find_door(ch, type, dir)) >= 0)
    {
        /* a door, perhaps */
        
        if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
            send_to_char("That's absurd.\n\r", ch);
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
            send_to_char("You have to close it first, I'm afraid.\n\r", ch);
        else if (EXIT(ch, door)->key < 0)
            send_to_char("There does not seem to be any keyholes.\n\r", ch);
        else if (!has_key(ch, EXIT(ch, door)->key))
            send_to_char("You don't have the proper key.\n\r", ch);
        else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
            send_to_char("It's already locked!\n\r", ch);
        else
        {   
            SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
            if (EXIT(ch, door)->keyword)
                act("$n locks the $F.", ch, 0,  EXIT(ch, door)->keyword,
                    TO_ROOM, 0);
            else
                act("$n locks the door.", ch, 0, 0, TO_ROOM, 0);
            send_to_char("*Click*\n\r", ch);
            /* now for locking the other side, too */
            if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
            if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
                    if (back->to_room == ch->in_room)
                        SET_BIT(back->exit_info, EX_LOCKED);
        }
    }
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
    
    if (!*type)
        send_to_char("Unlock what?\n\r", ch);
    else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
        ch, &victim, &obj))
    {
        /* ths is an object */
        
        if (obj->obj_flags.type_flag != ITEM_CONTAINER)
            send_to_char("That's not a container.\n\r", ch);
        else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
            send_to_char("Silly - it ain't even closed!\n\r", ch);
        else if (obj->obj_flags.value[2] < 0)
            send_to_char("Odd - you can't seem to find a keyhole.\n\r", ch);
        else if (!has_key(ch, obj->obj_flags.value[2]))
            send_to_char("You don't seem to have the proper key.\n\r", ch);
        else if (!IS_SET(obj->obj_flags.value[1], CONT_LOCKED))
            send_to_char("Oh.. it wasn't locked, after all.\n\r", ch);
        else
        {   
            REMOVE_BIT(obj->obj_flags.value[1], CONT_LOCKED);
            send_to_char("*Click*\n\r", ch);
            act("$n unlocks $p.", ch, obj, 0, TO_ROOM, 0);
        }
    }
    else if ((door = find_door(ch, type, dir)) >= 0)
    {
        /* it is a door */
        
        if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
            send_to_char("That's absurd.\n\r", ch);
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
            send_to_char("Heck ... it ain't even closed!\n\r", ch);
        else if (EXIT(ch, door)->key < 0)
            send_to_char("You can't seem to spot any keyholes.\n\r", ch);
        else if (!has_key(ch, EXIT(ch, door)->key))
            send_to_char("You do not have the proper key for that.\n\r", ch);
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
            send_to_char("It's already unlocked, it seems.\n\r", ch);
        else
        {   
            REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
            if (EXIT(ch, door)->keyword)
                act("$n unlocks the $F.", ch, 0, EXIT(ch, door)->keyword,
                    TO_ROOM, 0);
            else
                act("$n unlocks the door.", ch, 0, 0, TO_ROOM, 0);
            send_to_char("*click*\n\r", ch);
            /* now for unlocking the other side, too */
            if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
            if ( ( back = world[other_room].dir_option[rev_dir[door]] ) != 0 )
                    if (back->to_room == ch->in_room)
                        REMOVE_BIT(back->exit_info, EX_LOCKED);
        }
    }
    return eSUCCESS;
}

