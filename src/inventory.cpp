/************************************************************************
| $Id: inventory.cpp,v 1.1 2002/06/13 04:32:18 dcastle Exp $
| inventory.C
| Description:  This file contains implementation of inventory-management
|   commands: get, give, put, etc..
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

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif



/* extern variables */

extern CWorld world;
 
extern struct descriptor_data *descriptor_list;
extern struct str_app_type str_app[];
extern struct index_data *mob_index;
extern struct obj_data *object_list;

/* extern functions */
char *fname(char *namelist);
int isname(char *arg, char *arg2);
struct obj_data *create_money( int amount );
void palm  (struct char_data *ch, struct obj_data *obj_object, struct obj_data *sub_object);
void special_log(char *arg);
struct obj_data * bring_type_to_front(char_data * ch, int item_type);
struct obj_data * search_char_for_item(char_data * ch, sh_int item_number);

// local function declerations
int contains_no_trade_item(obj_data * obj);


/* procedures related to get */
void get(struct char_data *ch, struct obj_data *obj_object, 
    struct obj_data *sub_object)
{
    char buffer[MAX_STRING_LENGTH];

    if(IS_SET(obj_object->obj_flags.more_flags, ITEM_UNIQUE)) {
      if(search_char_for_item(ch, obj_object->item_number)) {
         send_to_char("The item's uniqueness prevents it!\r\n", ch);
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
	GET_GOLD(ch) += obj_object->obj_flags.value[0];
	extract_obj(obj_object);
    }
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
                if(!alldot && IS_SET(obj_object->obj_flags.more_flags, ITEM_NONOTICE))
                  continue;

                // PC corpse
		if(obj_object->obj_flags.value[3] == 1 && isname("pc", obj_object->name))
                {
                   sprintf(buffer, "%s_consent", GET_NAME(ch));
		   if(isname(GET_NAME(ch), obj_object->name))
                     has_consent = TRUE;

		   if(!has_consent && !isname(GET_NAME(ch), obj_object->name)) {
		     send_to_char("You don't have consent to take the corpse.\n\r", ch);
		     continue;
                   }
                   if(has_consent && contains_no_trade_item(obj_object)) {
                     send_to_char("This item contains no_trade items that cannot be picked up.\n\r", ch);
                     continue;
                   }
                   has_consent = FALSE;  // reset it for the next item:P
		}
		
		if (CAN_SEE_OBJ(ch, obj_object)) {
		    if ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)) {
			if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight)
				<= CAN_CARRY_W(ch)) {
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
		    } else {
			sprintf(buffer,
				"%s : You can't carry that many items.\n\r", 
			    fname(obj_object->name));
			send_to_char(buffer, ch);
			fail = TRUE;
		    }
		}
	    }
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
		if((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
		    if((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < 
			CAN_CARRY_W(ch)) {
			if (CAN_WEAR(obj_object,ITEM_TAKE)) {
                            if(cmd == 10) palm(ch, obj_object, sub_object);
			    else          get (ch, obj_object, sub_object);
                            do_save(ch,"", 666);
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
		} else {
		    sprintf(buffer,
			"%s : You can't carry that many items.\n\r", 
			fname(obj_object->name));
		    send_to_char(buffer, ch);
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
	    if (!sub_object)
		sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying);

	    if (sub_object) {
	        if(sub_object->obj_flags.type_flag == ITEM_CONTAINER && 
                   sub_object->obj_flags.value[3] == 1 &&
		   isname("pc", sub_object->name)) 
                {
                   sprintf(buffer, "%s_consent", GET_NAME(ch));
		   if(isname(buffer, sub_object->name))
                     has_consent = TRUE;
		   if(!has_consent && !isname(GET_NAME(ch), sub_object->name)) {
		     send_to_char("You don't have consent to touch the corpse.\n\r", ch);
		     return eFAILURE;
                   }
                }
		if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER) {
		  if (IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED)){
		    sprintf(buffer, "The %s is closed.\n\r",fname(sub_object->name));
		    send_to_char(buffer, ch);
		    return eFAILURE;
		  }
		  for(obj_object = sub_object->contains;
		      obj_object;
		      obj_object = next_obj) {
		    next_obj = obj_object->next_content;

		    /* IF all.obj, only get those named "obj" */
		    if (alldot && !isname(allbuf,obj_object->name)){
		      continue;
		    }
		    if (CAN_SEE_OBJ(ch,obj_object)) {
		      if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
		        if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < CAN_CARRY_W(ch)) {
                          if(has_consent && IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE)) {
                            // if I have consent and i'm touching the corpse, then I shouldn't be able
                            // to pick up no_trade items because it is someone else's corpse.  If I am
                            // the other of the corpse, has_consent will be false.
                            csendf(ch, "%s : It seems magically attached to the corpse.\n\r", fname(obj_object->name));
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
		      } else {
		        sprintf(buffer,"%s : You can't carry that many items.\n\r", 
			  fname(obj_object->name));
		        send_to_char(buffer, ch);
		        fail = TRUE;
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
	  }
	  if(sub_object) {
	    if(sub_object->obj_flags.type_flag == ITEM_CONTAINER && 
               sub_object->obj_flags.value[3] == 1 &&
	       isname("pc", sub_object->name)) 
            {
               sprintf(buffer, "%s_consent", GET_NAME(ch));
	       if(isname(buffer, sub_object->name))
                 has_consent = TRUE;
	       if(!has_consent && !isname(GET_NAME(ch), sub_object->name)) {
		 send_to_char("You don't have consent to touch the "
		               "corpse.\n\r", ch);
	         return eFAILURE;
               }
            }
	    if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER) {
	      if (IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED)){
	        sprintf(buffer,"The %s is closed.\n\r", fname(sub_object->name));
	        send_to_char(buffer, ch);
	        return eFAILURE;
	      }
	      obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains);
	      if (obj_object) {
	    if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
	      if ((IS_CARRYING_W(ch) + obj_object->obj_flags.weight) < 
		  CAN_CARRY_W(ch)) {
                if(has_consent && IS_SET(obj_object->obj_flags.more_flags, ITEM_NO_TRADE)) {
                  // if I have consent and i'm touching the corpse, then I shouldn't be able
                  // to pick up no_trade items because it is someone else's corpse.  If I am
                  // the other of the corpse, has_consent will be false.
                  csendf(ch, "%s : It seems magically attached to the corpse.\n\r", fname(obj_object->name));
                  fail = TRUE;
                }
		else if (CAN_WEAR(obj_object,ITEM_TAKE)) {
                  if(cmd == 10) palm(ch, obj_object, sub_object);
		  else          get (ch, obj_object, sub_object);
		  found = TRUE;
		} else {
		  send_to_char("You can't take that.\n\r", ch);
		  fail = TRUE;
		}
	      } else {
		sprintf(buffer,"%s : You can't carry that much weight.\n\r", 
		    fname(obj_object->name));
		send_to_char(buffer, ch);
		fail = TRUE;
	      }
	    } else {
	      sprintf(buffer,"%s : You can't carry that many items.\n\r", 
		  fname(obj_object->name));
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
  char buf[MAX_INPUT_LENGTH+1], buf2[MAX_STRING_LENGTH];
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

  for(obj = object_list; obj; obj = obj->next) {
     if(obj->obj_flags.type_flag != ITEM_CONTAINER || obj->obj_flags.value[3] != 1 || !obj->name)
       continue;
     
     if(!isname(GET_NAME(ch), obj->name))
       continue;
       
     sprintf(buf2, "%s %s_consent", obj->name, buf);
     obj->name = str_hsh(buf2);
  }
  
  sprintf(buf2, "All corpses in the game which belong to you can now be "
          "molested by\n\ranyone named %s.\n\r", buf);
  send_to_char(buf2, ch);
  return eSUCCESS;
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
    if(GET_GOLD(ch) < amount) {
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

      if(!IS_MOB(ch) && IS_AFFECTED(ch, AFF_CANTQUIT) && IS_SET(ch->pcdata->punish, PUNISH_THIEF)) {
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

      if(!IS_MOB(ch) && IS_AFFECTED(ch, AFF_CANTQUIT) && IS_SET(ch->pcdata->punish, PUNISH_THIEF)) {
        send_to_char("Your criminal acts prohibit it.\n\r", ch);
        return eFAILURE;
      }
      if(IS_SET(tmp_object->obj_flags.more_flags, ITEM_NO_TRADE)) {
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
  char buf[200], namebuf[200];
  bool found = FALSE;

 /* If "put all.object bag", get all carried items
  * named "object", and put each into the bag.
  */

  for(tmp_object = ch->carrying; tmp_object; tmp_object = next_object) {
     next_object = tmp_object->next_content;
     if(!name) {
       sprintf(buf, "%s %s", one_argument(tmp_object->name, namebuf), target);
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
      if(!(get_obj_in_list_vis(ch, arg2, ch->carrying))) {
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
            send_to_char("You are unable to! That item must be CURSED!\n\r",
                         ch);
            return eFAILURE;
          }
          else
            send_to_char("(This item is cursed, BTW.)\n\r", ch);
        }  
	if(GET_ITEM_TYPE(obj_object) == ITEM_CONTAINER)
	{
	  send_to_char("You would ruin it!\n\r", ch);
	  return eFAILURE;
	}

        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM,
                            ch, &tmp_char, &sub_object);
        if(sub_object) {
          if(GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER) {
	    if(!IS_SET(sub_object->obj_flags.value[1], CONT_CLOSED)) {
	      if(obj_object == sub_object) {
		send_to_char(
			"You attempt to fold it into itself, but fail.\n\r",
			ch);
		return eFAILURE;
	      }

              if(IS_SET(obj_object->obj_flags.extra_flags, ITEM_SPECIAL) &&
              !IS_SET(sub_object->obj_flags.extra_flags, ITEM_SPECIAL))
              {
                send_to_char("Are you crazy?!  Someone could steal it!\r\n", ch);
                return eFAILURE;
              }

	      if(((sub_object->obj_flags.weight) + 
		  (obj_object->obj_flags.weight)) <
		  (sub_object->obj_flags.value[0])) {
		send_to_char("Ok.\n\r", ch);
		if(bits == FIND_OBJ_INV) {
		  obj_from_char(obj_object);
		  /* make up for above line */
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
	    }
            else {
	      sprintf(buffer, "The %s is not a container.\n\r",
		      fname(sub_object->name));
	      send_to_char(buffer, ch);
	    }
	  }
          else {
            sprintf(buffer, "You dont have the %s.\n\r", arg2);
            send_to_char(buffer, ch);
          }
	}
        else {
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
  struct char_data *vict;
  struct obj_data *obj;

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n",
                  ch);
    return eFAILURE;
  }

  argument = one_argument(argument, obj_name);

  if(is_number(obj_name)) { 
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
    if(GET_GOLD(ch) < amount && GET_LEVEL(ch) < DEITY) 
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

      csendf(ch, "You give %ld coins to %s.\n\r", (long) amount,
             GET_SHORT(vict));

      if(GET_LEVEL(ch) >= IMMORTAL || IS_NPC(ch)) { 
        sprintf(buf, "%s gives %d coins to %s", GET_NAME(ch), amount,
                GET_NAME(vict));
        log(buf, 110, LOG_GOD);
      }
      
      sprintf(buf, "%s gives you %d gold coins.\n\r", PERS(ch, vict), amount);
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

    if(!IS_MOB(ch) && IS_AFFECTED(ch, AFF_CANTQUIT) && IS_SET(ch->pcdata->punish, PUNISH_THIEF)) {
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
      send_to_char("It seems magically attached to you.\r\n", ch);
      return eFAILURE;
    }
    if(contains_no_trade_item(obj)) {
      send_to_char("Something inside it seems magically attached to you.\r\n", ch);
      return eFAILURE;
    }

    if (IS_NPC(vict) && mob_index[vict->mobdata->nr].non_combat_func == shop_keeper)
    {
       act("$N graciously refuses your gift.", ch, 0, vict, TO_CHAR, 0);
       return eFAILURE;
    }

    if(!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_THIEF) && !vict->desc) {
      send_to_char("Now WHY would a thief give something to a linkdead "
                   "char..?\n\r", ch);
      return eFAILURE;
    }

    if ((1+IS_CARRYING_N(vict)) > CAN_CARRY_N(vict))
    {
	act("$N seems to have $S hands full.", ch, 0, vict, TO_CHAR, 0);
	return eFAILURE;
    }
    if (obj->obj_flags.weight + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
    {
	act("$E can't carry that much weight.", ch, 0, vict, TO_CHAR, 0);
	return eFAILURE;
    }
    if(IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE)) {
      if(search_char_for_item(vict, obj->item_number)) {
         send_to_char("The item's uniqueness prevents it.\r\n", ch);
         csendf(vict, "%s tried to give you an item but was unable.\r\n", GET_NAME(ch));
         return eFAILURE;
      }
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

//    send_to_char("Ok.\n\r", ch);
    do_save(ch,"",10);
    do_save(vict,"",10);

    retval = mprog_give_trigger( vict, ch, obj );
    if(SOMEONE_DIED(retval)) {
      retval = SWAP_CH_VICT(retval);
      return eSUCCESS|retval;
    }
    // if I gave a no_trade item to a mob, the mob needs to destroy it
    // otherwise it defeats the purpose of no_trade:)
    if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) && IS_NPC(vict))
       extract_obj(obj);

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
    if(GET_ITEM_TYPE(i) == ITEM_CONTAINER) { // search inside
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
    if (ch->equipment[k] && ch->equipment[k]->item_number == item_number)
      return ch->equipment[k];
  }
      
  for(i = ch->carrying; i ; i = i->next_content) {
    if(i->item_number == item_number)
      return i;

    // does not support containers inside containers
    if(GET_ITEM_TYPE(i) == ITEM_CONTAINER) { // search inside
      for(j = i->contains; j ; j = j->next_content) {
        if(i->item_number == item_number) 
          return j;
      }
    }
  }
  return NULL;
}

