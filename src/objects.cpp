/************************************************************************
| $Id: objects.cpp,v 1.116 2015/06/16 04:10:54 pirahna Exp $
| objects.C
| Description:  Implementation of the things you can do with objects:
|   wear them, wield them, grab them, drink them, eat them, etc..
*/
extern "C"
{
  #include <ctype.h>
  #include <string.h>
}
#include <obj.h>
#include <connect.h>
#include <utility.h>
#include <room.h>
#include <spells.h>
#include <player.h>
#include <handler.h>
#include <affect.h>
#include <levels.h>
#include <interp.h>
#include <character.h>
#include <act.h>
#include <structs.h>
#include <db.h>
#include <assert.h>
#include <mobile.h> // ACT_ISNPC
#include <race.h>
#include <returnvals.h>
#include <clan.h> // vault stuff

extern char *drinks[];
extern char *dirs[];
extern int drink_aff[][3];
extern CWorld world;

extern struct spell_info_type spell_info[MAX_SPL_LIST];
extern struct index_data *obj_index;
extern struct index_data *mob_index; 
int hands_are_free(CHAR_DATA *ch, int number);
struct obj_data *get_object_in_equip_vis(struct char_data *ch,
    char *arg, struct obj_data *equipment[], int *j, bool blindfighting);

// add an affect to an item
void add_obj_affect(obj_data * obj, int loc, int mod)
{
  obj->num_affects++;
#ifdef LEAK_CHECK
  obj->affected = (obj_affected_type *) realloc(obj->affected,
                               (sizeof(obj_affected_type) * obj->num_affects));
#else
  obj->affected = (obj_affected_type *) dc_realloc(obj->affected,
                               (sizeof(obj_affected_type) * obj->num_affects));
#endif
  obj->affected[obj->num_affects - 1].location = loc;
  obj->affected[obj->num_affects - 1].modifier = mod;
}

void remove_obj_affect_by_index(obj_data * obj, int index)
{
   // shift everyone to right of the one we're deleting to the left
   // TODO - redo this with memmove
   for(int i = index; i < obj->num_affects - 1; i++)
   {
      obj->affected[i].location = obj->affected[i+1].location;
      obj->affected[i].modifier = obj->affected[i+1].modifier;
   }

   // remove the last unused affect
   obj->num_affects--;
   if(obj->num_affects)
      obj->affected = (obj_affected_type *)realloc(obj->affected, (sizeof(obj_affected_type)*obj->num_affects));
   else {
      dc_free(obj->affected);
      obj->affected = NULL;
   }
}

void remove_obj_affect_by_type(obj_data * obj, int loc)
{
   for(int i = 0; i < obj->num_affects; i++)
      if(obj->affected[i].location == loc)
         remove_obj_affect_by_index(obj, i);
}

// given an object, return the maximum points of damage the item
// can take before being scrapped
int eq_max_damage(obj_data * obj)
{
   int amount = 0;

   switch(GET_ITEM_TYPE(obj)) {
      case ITEM_ARMOR:
         amount = 3;
         amount += ((obj->obj_flags.value[0]) / 2);  // + 1 hit per 2ac
         break;
      case ITEM_WEAPON:
         amount = 5;
         break;
      case ITEM_FIREWEAPON:
         amount = 2;
         break;
      case ITEM_CONTAINER:
         amount = 2;
         break;
      case ITEM_INSTRUMENT:
         amount = 3;
         break;
      case ITEM_WAND:
      case ITEM_STAFF:
	amount = 3;
	break;
      case ITEM_LIGHT:
	amount = 3;
	break;
      case ITEM_KEY:
         amount = 2;
         break;
      default:
         amount = 1;
         break;
   }

   return amount;
}

int eq_current_damage(obj_data * obj)
{
  for(int i = 0; i < obj->num_affects; i++) 
    if(obj->affected[i].location == APPLY_DAMAGED)
       return (obj->affected[i].modifier);

  return 0;
}

// when repairing eq, we just leave the affect of 0 in there.  That way when
// it gets damaged again, we don't have to realloc the affect list again
void eq_remove_damage(obj_data * obj)
{
  for(int i = 0; i < obj->num_affects; i++)
    if(obj->affected[i].location == APPLY_DAMAGED)
    {
      obj->affected[i].modifier = 0;
      break;
    }
}

// Damage a piece of eq once and return the amount of damage currently on it
int damage_eq_once(obj_data * obj)
{
  if(obj_index[obj->item_number].virt == SPIRIT_SHIELD_OBJ_NUMBER && obj->carried_by && obj->carried_by->in_room) {
    send_to_room("The spirit shield shimmers brightly then fades away.\n\r", obj->carried_by->in_room);
    extract_obj(obj);
    return 0;
  }
  // look for existing damage
  for(int i = 0; i < obj->num_affects; i++) 
    if(obj->affected[i].location == APPLY_DAMAGED)
    {
       obj->affected[i].modifier++;
       return (obj->affected[i].modifier);
    }

  // no existing damage.  Damage it once
  add_obj_affect(obj, APPLY_DAMAGED, 1);
  return 1;
}


void object_activity(uint64_t pulse_type)
{
  for (auto &obj : DC::instance().active_obj_list)
  {
    int32 item_number = obj->item_number;

    if (obj_index[item_number].non_combat_func)
    {
      obj_index[item_number].non_combat_func(NULL, obj, 0, "", NULL);
    } else if (obj->obj_flags.type_flag == ITEM_MEGAPHONE && obj->ex_description && obj->obj_flags.value[0]-- == 0)
    {
      obj->obj_flags.value[0] = ((obj_data*) obj_index[item_number].item)->obj_flags.value[1];
      send_to_room(obj->ex_description->description, obj->in_room, TRUE);
    } else
    {
      int retval = 0;
      extern struct zone_data *zone_table;

      if (obj->in_room != NOWHERE)
      {
        if (zone_table[world[obj->in_room].zone].players)
          retval = oprog_rand_trigger(obj);
      } else
        retval = oprog_rand_trigger(obj);
      if (!SOMEONE_DIED(retval) && objExists(obj))
        oprog_arand_trigger(obj);
    }
  }

  DC::instance().removeDead();
  return;
}

void name_from_drinkcon(struct obj_data *obj)
{
  int i;
  char *new_new_name;

  for(i = 0; (*((obj->name) + i) != ' ') && (*((obj->name)+i) != '\0'); i++) ;

  if(*((obj->name) + i) == ' ') {
    new_new_name  = str_hsh((obj->name)+i+1);
    obj->name = new_new_name;
  }
}

int do_switch(struct char_data *ch, char *arg, int cmd)
{
  struct obj_data *between;

  if(IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  if (!ch->equipment[WIELD] || !ch->equipment[SECOND_WIELD]) {
     send_to_char("You must be wielding two weapons to switch their "
                  "positions.\n\r", ch);
     return eFAILURE;
  }

  if (GET_MOVE(ch) < 4) {
     send_to_char("You are too tired to switch your weapons!", ch);
     return eFAILURE;
  }
  GET_MOVE(ch) -= 4;

  if (!has_skill(ch, SKILL_SWITCH) || !skill_success(ch,NULL,SKILL_SWITCH)) {
    act("$n fails to switch $s weapons.", ch,0,0, TO_ROOM, 0);
    act("You fail to switch your weapons.", ch, 0,0, TO_CHAR, 0);
    return eFAILURE;
  }
  if (GET_OBJ_WEIGHT(ch->equipment[WIELD]) > MIN(GET_STR(ch)/2 , get_max_stat(ch, STR)/2)
&& !IS_AFFECTED(ch, AFF_POWERWIELD))
  {
     send_to_char("Your primary wield is too heavy to wield as secondary.\r\n",ch);
      return eFAILURE;
  }
  between = ch->equipment[WIELD];
  ch->equipment[WIELD] = ch->equipment[SECOND_WIELD];
  ch->equipment[SECOND_WIELD] = between;
  send_to_char("You switch your weapon positions.\n\r", ch);
  return eSUCCESS;
}

int do_quaff(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH+1];
  struct obj_data *temp;
  int i/*,j*/;
  bool equipped;
  int retval = eSUCCESS;
  int is_mob = IS_MOB(ch);
  int lvl;

  equipped = FALSE;
  int pos = -1;
  one_argument(argument,buf);

  if (!(temp = get_obj_in_list_vis(ch,buf,ch->carrying))) {
    temp = ch->equipment[HOLD];
    equipped = TRUE;
	pos = HOLD;
    if ((temp==0) || !isname(buf, temp->name))
    {
      temp = ch->equipment[HOLD2];
	pos = HOLD2;
      if ((temp==0) || !isname(buf, temp->name))
      {
        equipped = FALSE;
        pos = -2;
        if(!(temp = get_obj_in_list_vis(ch,buf,ch->carrying,TRUE))) {
          act("You do not have that item.",ch,0,0,TO_CHAR, 0);
          return eFAILURE;
        }
      }
    }
  }

  if (temp->obj_flags.type_flag!=ITEM_POTION)
  {
    act("You can only quaff potions.",ch,0,0,TO_CHAR, 0);
    return eFAILURE;
  }
 
  if ((GET_COND(ch,FULL) >= 24) && (GET_COND(ch,THIRST) >= 24) && !IS_NPC(ch)) {
    act("Your stomach is too full to quaff that!", ch,0,0,TO_CHAR, 0);
    return eFAILURE;
  }

  WAIT_STATE(ch,PULSE_VIOLENCE/2);
  if (ch->fighting && number(0,1) != 0) {
    act ("During combat, $n drops $p and it SMASHES!", ch, temp, 0, TO_ROOM, 0);
    act ("During combat, you drop $p which SMASHES!",  ch, temp, 0, TO_CHAR, 0);
    if (equipped)
      unequip_char(ch, pos);
    extract_obj(temp);
    return eSUCCESS;
  }

  if (!ch->fighting && IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  if(pos == -2) WAIT_STATE(ch, PULSE_VIOLENCE);

  act("$n quaffs $p.", ch, temp, 0, TO_ROOM, 0);
  act("You quaff $p which dissolves.",ch,temp,0,TO_CHAR, 0);

  gain_condition(ch,FULL,2);
  gain_condition(ch,THIRST,2);

  for (i = 1; i < 4; i++) 
  {
     if (temp->obj_flags.value[i] >= 1) 
     {
        if(spell_info[temp->obj_flags.value[i]].spell_pointer)
        {
           lvl = (int) (1.5 * temp->obj_flags.value[0]); 
           retval = ((*spell_info[temp->obj_flags.value[i]].spell_pointer)
              ((ubyte) temp->obj_flags.value[0], ch, "", SPELL_TYPE_POTION, ch, 0, lvl));
        }
     }
     if(IS_SET(retval, eCH_DIED))
        break;
  }
  if(!is_mob || !IS_SET(retval, eCH_DIED)) // it's already been free'd when mob died
  {     
    if (equipped)
      unequip_char(ch, pos,1);
    extract_obj(temp);
  }
  return retval;
}


int do_recite(struct char_data *ch, char *argument, int cmd)
{
    char buf[MAX_INPUT_LENGTH+1];
    struct obj_data *scroll, *obj;
    struct char_data *victim;
    int i, bits;
    bool equipped;
    int retval = eSUCCESS;
    int is_mob = IS_MOB(ch);
    int lvl;
    
    if(IS_SET(world[ch->in_room].room_flags, NO_MAGIC)) {
       send_to_char("Your magic is muffled by greater beings.\n\r", ch);
       return eFAILURE; 
    }
    equipped = FALSE;
    obj = 0;
    victim = 0;
  int pos = -1;
    argument = one_argument(argument,buf);

  if (!(scroll = get_obj_in_list_vis(ch,buf,ch->carrying))) {
    scroll = ch->equipment[HOLD];
    equipped = TRUE;
	pos = HOLD;
    if ((scroll==0) || !isname(buf, scroll->name))
    {
      scroll = ch->equipment[HOLD2];
	pos = HOLD2;
      if ((scroll==0) || !isname(buf, scroll->name))
      {
        act("You do not have that item.",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
      }
    }
  }
    if (IS_SET(world[ch->in_room].room_flags, NO_MAGIC)) {
       act("Your magic is muffled by greater beings.",ch,0,0,TO_CHAR, 0);
       return eFAILURE;
    }
    if (scroll->obj_flags.type_flag!=ITEM_SCROLL)
    {
      act("Recite is normally used for scrolls.",ch,0,0,TO_CHAR, 0);
      return eFAILURE;
    }

    if (*argument) {
    	if (scroll->obj_flags.value[1] == SPELL_IDENTIFY) {
    		bits = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM, ch, &victim, &obj);
    	} else {
    		bits = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &victim, &obj);
    	}
      if (bits == 0) {
        send_to_char("No such thing around to recite the scroll on.\n\r", ch);
        return eFAILURE;
      }
    } else {
      victim = ch;
    }

    act("$n recites $p.",  ch, scroll, 0, TO_ROOM, INVIS_NULL);
    act("You recite $p which dissolves.",ch,scroll,0,TO_CHAR, 0);

    int failmark = 35 - GET_INT(ch);
    if (IS_NPC(ch)) failmark -= 15; 

    if( GET_CLASS(ch) == CLASS_MAGIC_USER ||
        GET_CLASS(ch) == CLASS_CLERIC     ||
        GET_CLASS(ch) == CLASS_DRUID
      )
      failmark -= 5;
    WAIT_STATE(ch,PULSE_VIOLENCE);
    
    if( ch->fighting && number(0, 100) < failmark )
    {
      // failed to read scroll
      act("$n mumbles the words on the scroll and it goes up in flame!", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You mumble the words and the scroll goes up in flame!\r\n", ch);
    }
    else
    {
        if (victim && !AWAKE(victim) && number(1,5) < 3)
            send_to_char("Your sleep is restless.\r\n",victim);

      // success
      for (i=1; i<4; i++)
      {
        if (scroll->obj_flags.value[i] >= 1)
        {
          lvl = (int) (1.5 * scroll->obj_flags.value[0]); 
          retval = ((*spell_info[scroll->obj_flags.value[i]].spell_pointer)
            ((ubyte) scroll->obj_flags.value[0], ch, "", SPELL_TYPE_SCROLL, victim, obj, lvl));
          if(SOMEONE_DIED(retval))
            break;
          if(victim && ch->in_room != victim->in_room) break;
        }
      }
    }
    if(!is_mob || !IS_SET(retval, eCH_DIED)) // it's already been free'd when mob died
    {
      if (equipped)
        unequip_char(ch, pos,1);
      extract_obj(scroll);
    }
    return eSUCCESS;
}

#define GOD_TRAP_ITEM    193

void set_movement_trap(struct char_data *ch, struct obj_data *obj)
{
  char buf[200];
  struct obj_data * trap_obj = NULL;

  sprintf(buf, "You set up the %s to catch people moving around in the area.\r\n", obj->short_description);
  send_to_char(buf, ch);
  act("$n sets something on the ground all around $m.", ch, 0, 0, TO_ROOM, 0);

  // make a new item
  trap_obj = clone_object(GOD_TRAP_ITEM);

  // copy the data for that trap item over
  for(int i = 0;i < 4; i++)
    trap_obj->obj_flags.value[i] = obj->obj_flags.value[i];

  // set it up in the room
  SETBIT(ch->affected_by, AFF_UTILITY);
  obj_to_room(trap_obj, ch->in_room);
}

void set_exit_trap(struct char_data *ch, struct obj_data *obj, char * arg)
{
  char buf[200];
  struct obj_data * trap_obj = NULL;

  sprintf(buf, "You set up the %s to catch people trying to leave the area.\r\n", obj->short_description);
  send_to_char(buf, ch);
  act("$n sets something on the ground all around $m.", ch, 0, 0, TO_ROOM, 0);

  // make a new item
  trap_obj = clone_object(GOD_TRAP_ITEM);

  // copy the data for that trap item over
  for(int i = 0;i < 4; i++)
    trap_obj->obj_flags.value[i] = obj->obj_flags.value[i];

  // set it up in the room
  SETBIT(ch->affected_by, AFF_UTILITY);
  obj_to_room(trap_obj, ch->in_room);
}

#define MORTAR_ROUND_OBJECT_ID      113

// Return FALSE if there was a command problem
// Return TRUE if it went off
bool set_utility_mortar(struct char_data *ch, struct obj_data *obj, char *arg)
{
  char direct[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_data * trap_obj = NULL;
  int dir;

  int do_say(struct char_data *ch, char *argument, int cmd);

  one_argument(arg, direct);
  if(!arg) {
    send_to_char("Set it off in which direction?\r\n", ch);
    return FALSE;
  }

  if(direct[0] == 'n') dir = 0;
  else if(direct[0] == 'e') dir = 1;
  else if(direct[0] == 's') dir = 2;
  else if(direct[0] == 'w') dir = 3;
  else if(direct[0] == 'u') dir = 4;
  else if(direct[0] == 'd') dir = 5;
  else {
    send_to_char("Set it off in which direction?\n\r", ch);
    return FALSE;
  }

  if( IS_SET(world[ch->in_room].room_flags, SAFE) ) {
    send_to_char("In the rear with the gear huh?  Maybe use this somewhere in the field.\r\n", ch);
    return FALSE;
  }
  if( CAN_GO(ch, dir) && IS_SET( world[ world[ch->in_room].dir_option[dir]->to_room ].room_flags, SAFE) ) {
    send_to_char("Firing it into a safe room seems wasteful.\r\n", ch);
    return FALSE;
  }

  // make a new item
  trap_obj = clone_object( real_object( MORTAR_ROUND_OBJECT_ID ) );

  // copy the data for that trap item over
  for(int i = 0;i < 4; i++)
    trap_obj->obj_flags.value[i] = obj->obj_flags.value[i];

  do_say(ch, "Fire in the hole!", 9);
  act("$n sets off $o with a flash and bang!.", ch, obj, 0, TO_ROOM, 0);
  send_to_char("You set off the device with a loud bang.\r\n", ch);

  if(!CAN_GO(ch, dir)) {
    send_to_room("It smacks against the wall, zings aorund, and lands in the middle of the room.\r\n", ch->in_room);
    // set it up in the room
    obj_to_room(trap_obj, ch->in_room);
  } else {
    sprintf(buf, "It flies with great speed %sward.\r\n", dirs[dir]);
    send_to_room(buf, ch->in_room);
    sprintf(buf, "Something flies into the area with great speed landing at your feet.\r\n");
    send_to_room(buf, world[ch->in_room].dir_option[dir]->to_room);
    // set it up in the target room
    obj_to_room(trap_obj, world[ch->in_room].dir_option[dir]->to_room);
  }

  return TRUE;
}


// With catstink, the value[1] is the sector type it was designed for
void set_catstink(struct char_data *ch, struct obj_data *obj)
{
  char buf[200];
  extern char *sector_types[];

  sprintf(buf, "You sprinkle the %s all around you.\r\n", obj->short_description);
  send_to_char(buf, ch);
  act("$n sprinkles something on the ground around $m.", ch, 0, 0, TO_ROOM, 0);

  // make sure it's useable in the place we're at
  if(world[ch->in_room].sector_type != obj->obj_flags.value[1])
  {
    if(SECT_MAX_SECT < obj->obj_flags.value[1] ||
       0             > obj->obj_flags.value[1])
    {
       send_to_char("This item has an illegal value1.  Tell a god.\r\n", ch);
       return;
    }

    sprintf(buf, "It probably won't work, since %s was designed for the smells of a %s",
           obj->short_description, sector_types[obj->obj_flags.value[1]]);
    send_to_char(buf, ch);

    // small chance of success
    if(number(0, 9))
       return;
  }  

  SETBIT(ch->affected_by, AFF_UTILITY);
  world[ch->in_room].FreeTracks();
}

void set_utility_item(struct char_data *ch, struct obj_data *obj, char *argument)
{
  int class_restricted(struct char_data *ch, struct obj_data *obj);

  if(class_restricted(ch, obj))
  {
    send_to_char("You are forbidden.\r\n", ch);
    return;
  }

  switch(obj->obj_flags.value[0]) {
    case UTILITY_CATSTINK:
      set_catstink(ch, obj);
      break;
    case UTILITY_EXIT_TRAP:
      set_exit_trap(ch, obj, argument);
      break;
    case UTILITY_MOVEMENT_TRAP:
      set_movement_trap(ch, obj);
      break;
    case UTILITY_MORTAR:
      if(! set_utility_mortar(ch, obj, argument) )
        return; // it failed
      break;
    default:
      send_to_char("Unknown utility item value.  Tell a god.\r\n", ch);
      return; 
      break;
  }

  WAIT_STATE(ch, (PULSE_VIOLENCE * obj->obj_flags.value[3]));
  extract_obj(obj);
}

int do_mortal_set(struct char_data *ch, char *argument, int cmd)
{
  struct obj_data *obj = NULL;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];

  argument = one_argument(argument, arg);

  if(!*arg) {
    send_to_char("Set what?\r\n", ch);
    return eFAILURE;
  }

  if(!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You do not seem to have a '%s'.\r\n", arg);
    send_to_char(buf, ch);
    return eFAILURE;
  }

  switch(obj->obj_flags.type_flag) {
    case ITEM_UTILITY:
      set_utility_item(ch, obj, argument);
      break;
    case ITEM_TRAP:
//      set_trap_item(ch, obj, argument);
//      break;
   default:
      send_to_char("You can't set that.\r\n", ch);
      break;
   }
   return eSUCCESS;
}

int do_use(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH+1];
  char targ[MAX_INPUT_LENGTH+1];
  char xtra_arg[MAX_INPUT_LENGTH+1];
  struct char_data *tmp_char;
  struct obj_data *tmp_object, *stick;
  int lvl;
  int bits;

  if (IS_SET(world[ch->in_room].room_flags, QUIET))
  {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  if (IS_SET(world[ch->in_room].room_flags, NO_MAGIC))
  {
    send_to_char ("Your magic is muffled by greater beings.\n\r", ch);
    return eFAILURE;
  }

  argument = one_argument(argument,buf);

  if ((ch->equipment[HOLD] == 0 || !isname(buf, ch->equipment[HOLD]->name)) &&
	(ch->equipment[HOLD2] == 0 || !isname(buf, ch->equipment[HOLD2]->name)))
	 {
    act("You must be holding an item in order to to use it.",ch,0,0,TO_CHAR, 0);
    return eFAILURE;
  }
  if (ch->equipment[HOLD] && isname(buf, ch->equipment[HOLD]->name))
   stick = ch->equipment[HOLD];
  else stick = ch->equipment[HOLD2];
 
  argument = one_argument(argument, targ);
  argument = one_argument(argument, xtra_arg);
  if (stick->obj_flags.type_flag == ITEM_STAFF)
  {
    act("$n taps $p three times on the ground.", ch, stick, 0,TO_ROOM, 0);
    act("You tap $p three times on the ground.", ch, stick, 0,TO_CHAR, 0);
    if (stick->obj_flags.value[2] > 0) { /* Charges left? */
      stick->obj_flags.value[2]--;
      lvl = (int) (1.5 * stick->obj_flags.value[0]); 
      WAIT_STATE(ch,PULSE_VIOLENCE);
	int retval = 0;
      if (spell_info[stick->obj_flags.value[3]].spell_pointer)
      retval = ((*spell_info[stick->obj_flags.value[3]].spell_pointer)
        ((ubyte) stick->obj_flags.value[0], ch, xtra_arg, SPELL_TYPE_STAFF, 0, 0, lvl));
	else retval= eFAILURE;
      return retval;
    } else {
      send_to_char("The staff seems powerless.\n\r", ch);
    }
  } else if (stick->obj_flags.type_flag == ITEM_WAND) {

    bits = generic_find(targ, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
    if (bits) {
      if (bits == FIND_CHAR_ROOM) {
        act("$n points $p at you.", ch, stick, tmp_char, TO_VICT, INVIS_NULL);
        act("$n points $p at $N.", ch, stick, tmp_char, TO_ROOM, NOTVICT|INVIS_NULL);
        act("You point $p at $N.",ch, stick, tmp_char, TO_CHAR, 0);
      } else {
        act("$n points $p at $P.", ch, stick, tmp_object, TO_ROOM, INVIS_NULL);
        act("You point $p at $P.",ch, stick, tmp_object, TO_CHAR, 0);
      }

      if (stick->obj_flags.value[2] > 0) { // are there any charges left? 
        stick->obj_flags.value[2]--;
        lvl = (int) (1.5 * stick->obj_flags.value[0]); 
	WAIT_STATE(ch,PULSE_VIOLENCE);
	int retval;
        if (spell_info[stick->obj_flags.value[3]].spell_pointer)
	retval= ((*spell_info[stick->obj_flags.value[3]].spell_pointer)
          ((ubyte) stick->obj_flags.value[0], ch, xtra_arg, SPELL_TYPE_WAND, tmp_char, tmp_object, lvl));
	else
	retval = eFAILURE;
	return retval;
      } else {
        send_to_char("The wand seems powerless.\n\r", ch);
      }
    } else {
      send_to_char("What should the wand be pointed at?\n\r", ch);
    }
  } else {
    send_to_char("Use is normally only for wands and staves.\n\r", ch);
  }
  return eFAILURE;
}


// Allows a player to change his "name" (short_desc) (Sadus) 
int do_name(struct char_data *ch, char *arg, int cmd)
{
  char buf[200];
  char _convert[2];
  int ctr;
  int nope = 0;

  if(!IS_MOB(ch) && IS_SET(ch->pcdata->punish, PUNISH_NONAME)) {
    send_to_char("You can't do that.  You must have been naughty.\n\r", ch);
    return eFAILURE;
  }
  if (GET_LEVEL(ch) < 5)
  {
    send_to_char("You cannot use the \"name\" command until you have reached level 5.\r\n",ch);
    return eFAILURE;
  }
  while(*arg == ' ')  /* get rid of white space */
    arg++;

  if(!*arg) {
    send_to_char("Set your name to what?\n\r", ch);
    return eFAILURE;
  }

  if(strlen(arg) > 30) {
    send_to_char("Name too long, must be under 30 characters long.\n\r", ch);
    return eFAILURE;
  }

  *buf = '\0';

  for(ctr = 0; (unsigned) ctr <= strlen(arg); ctr++) {
    if(arg[ctr] == '$') {
      arg[ctr] = ' ';
    }
    if((arg[ctr] == '?') && (arg[ctr + 1] == '?')) {
      arg[ctr] = ' ';
    }
    if(arg[ctr] == '%') {
      if(nope == 0)
        nope = 1;
      else if(nope == 1) { 
        send_to_char("You can only include one % in your name ;)\n\r", ch);
        return eFAILURE; 
      } 
    }
  } 
  if(nope == 0) {
    send_to_char("You MUST include your real name. Use % to indicate where you want it.\n\r", ch);
    return eFAILURE;
  }

  for(ctr = 0; (unsigned) ctr < strlen(arg); ctr++) {
    _convert[0] = arg[ctr];
    _convert[1] = '\0';
    if(arg[ctr] == '%') {
      strcat(buf, fname(GET_NAME(ch)));
      if(arg[ctr+1] != '\0' && isalpha(arg[ctr+1]))
        strcat(buf, " ");
    } 
    else if(arg[ctr+1] == '%' && isalpha(arg[ctr])) {
      strcat(buf, _convert);
      strcat(buf, " ");
    } 
    else strcat(buf, _convert); 
  }

  // only free PC short descs
  if(GET_SHORT_ONLY(ch) && !IS_NPC(ch))
    dc_free(GET_SHORT_ONLY(ch));

  if(IS_NPC(ch))
    GET_SHORT_ONLY(ch) = str_hsh(buf);
  else GET_SHORT_ONLY(ch) = str_dup(buf);
  send_to_char("Ok.\n\r", ch);
  return eSUCCESS;
}

int do_drink(struct char_data *ch, char *argument, int cmd)
{
    char buf[MAX_INPUT_LENGTH+1];
    struct obj_data *temp;
    struct affected_type af;
    int amount;

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
    {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
    }

    if((GET_COND(ch,DRUNK)>10)&&(GET_COND(ch,THIRST)>0)) /* The pig is drunk */
    {
      act("You simply fail to reach your mouth!", ch, 0, 0, TO_CHAR, 0);
      act("$n tried to drink but missed $s mouth!", ch, 0, 0, TO_ROOM, INVIS_NULL);
      return eFAILURE;
    }
      
    if(GET_COND(ch,FULL) > 20 && GET_COND(ch,THIRST) > 20) /* Stomach full */
    {
      act("Your stomach cannot contain anymore!",ch,0,0,TO_CHAR, 0);
      return eFAILURE;
    }
      
    one_argument(argument,buf);

    if((temp = get_obj_in_list_vis(ch,buf,world[ch->in_room].contents)) && temp->obj_flags.type_flag == ITEM_FOUNTAIN && CAN_SEE_OBJ(ch, temp))
    {
      act("You drink from $p.", ch, temp, 0, TO_CHAR, 0);
      act("$n drinks from $p.", ch, temp, 0, TO_ROOM, INVIS_NULL);
      act("You are full.", ch, 0, 0, TO_CHAR, 0);
      act("You are not thirsty anymore.",ch, 0, 0, TO_CHAR, 0);

      if (GET_LEVEL(ch)>=IMMORTAL)
        return eSUCCESS;
        
      if (GET_COND(ch,FULL) != -1)
        GET_COND(ch,FULL) = 22 + number(0,5);
      if (GET_COND(ch, THIRST) != -1)
        GET_COND(ch,THIRST) = 22 + number(0,5);
        
      return eSUCCESS;
    }

    if(GET_COND(ch, THIRST) > 20) {
      send_to_char("Your stomach cannot contain anymore liquid!\r\n", ch);
      return eFAILURE;
    }

    if(!(temp = get_obj_in_list_vis(ch,buf,ch->carrying)))
    {
      act("You cannot find it!",ch,0,0,TO_CHAR, 0);
      return eFAILURE;
    }

    if (temp->obj_flags.type_flag!=ITEM_DRINKCON)
    {
      act("You can't drink from that!",ch,0,0,TO_CHAR, 0);
      return eFAILURE;
    }
      
    if (temp->obj_flags.type_flag==ITEM_DRINKCON)
    {
      if(temp->obj_flags.value[1]>0)  /* Not empty */
      {   
        sprintf(buf, "$n drinks %s from $p.",drinks[temp->obj_flags.value[2]]);
        act(buf, ch, temp, 0, TO_ROOM, INVIS_NULL);
        sprintf(buf, "You drink the %s.\n\r",drinks[temp->obj_flags.value[2]]);
        send_to_char(buf,ch);

        if (GET_LEVEL(ch)>=IMMORTAL)
          return eSUCCESS;

        // TODO what is this for?  the statement immediatly afterwards wipes out value
        if (drink_aff[temp->obj_flags.value[2]][DRUNK] > 0 )
          amount = (25-GET_COND(ch,THIRST)) / drink_aff[temp->obj_flags.value[2]][DRUNK];
        else
          amount = number(3,10);
        
        amount = MIN(amount,temp->obj_flags.value[1]);

        /* You can't subtract more than the object weighs */

        gain_condition(ch,DRUNK,(int)((int)drink_aff
                        [temp->obj_flags.value[2]][DRUNK]*amount)/4);

        gain_condition(ch,FULL,(int)((int)drink_aff
                       [temp->obj_flags.value[2]][FULL]*amount)/4);
          
        gain_condition(ch,THIRST,(int)((int)drink_aff
                         [temp->obj_flags.value[2]][THIRST]*amount)/4);

        if(GET_COND(ch,DRUNK)>10)
          act("You feel drunk.",ch,0,0,TO_CHAR, 0);

        if(GET_COND(ch,THIRST)>20)
          act("You do not feel thirsty.",ch,0,0,TO_CHAR, 0);
        
        if(GET_COND(ch,FULL)>20)
          act("You are full.",ch,0,0,TO_CHAR, 0);
        
        if(temp->obj_flags.value[2] == LIQ_HOLYWATER && 
             GET_HIT(ch) < GET_MAX_HIT(ch) &&
             number(0, 1)) 
        {
           send_to_char("You feel refreshed!\r\n", ch);
           GET_HIT(ch) += 10;
           if(GET_HIT(ch) > GET_MAX_HIT(ch))
             GET_HIT(ch) = GET_MAX_HIT(ch);
        }

        if(temp->obj_flags.value[3] && (GET_LEVEL(ch)<IMMORTAL))
        {
          /* The shit was poisoned ! */
          act("Ooups, it tasted rather strange ?!!?",ch,0,0,TO_CHAR, 0);
          act("$n chokes and utters some strange sounds.",
              ch,0,0,TO_ROOM, 0);
          if(number(1,100) < get_saves(ch,SAVE_TYPE_POISON) - 15) {
             send_to_char("Luckily, your body rejects the poison almost immediately.\n\r",ch);
          } else {
             af.type = SPELL_POISON;
             af.duration = amount*3;
             af.modifier = 0;
             af.location = APPLY_NONE;
             af.bitvector = AFF_POISON;
             affect_join(ch,&af, FALSE, FALSE);
          }
        }
        
          /* empty the container, and no longer poison. */
        temp->obj_flags.value[1]-= amount;
        if(!temp->obj_flags.value[1]) {  /* The last bit */
          temp->obj_flags.value[2]=0;
          temp->obj_flags.value[3]=0;
        }
/*
        if(temp->obj_flags.value[1]<=0) { 
          act("It is now empty, and it magically disappears in a puff of smoke!",
            ch,0,0,TO_CHAR, 0);
          extract_obj(temp);
        }
*/
        return eSUCCESS;
      }
    }

    act("It's empty already.",ch,0,0,TO_CHAR, 0);
    return eFAILURE;
}



int do_eat(struct char_data *ch, char *argument, int cmd)
{
    char buf[MAX_INPUT_LENGTH+1];
    struct obj_data *temp;
    struct affected_type af;

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
    {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
    }

    one_argument(argument,buf);

    if(!(temp = get_obj_in_list_vis(ch,buf,ch->carrying)))
    {
        act("You can't find it!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if((temp->obj_flags.type_flag != ITEM_FOOD) && (GET_LEVEL(ch) < IMMORTAL))
    {
        act("Your stomach refuses to eat that!?!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(GET_COND(ch,FULL)>20) /* Stomach full */
    {   
        act("You are too full to eat more!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    act("$n eats $p.",ch,temp,0,TO_ROOM, INVIS_NULL);
    act("You eat the $o.",ch,temp,0,TO_CHAR, 0);

    gain_condition(ch,FULL,temp->obj_flags.value[0]);

    if(GET_COND(ch,FULL)>20)
        act("You are full.",ch,0,0,TO_CHAR, 0);

    if(temp->obj_flags.value[3] && (GET_LEVEL(ch) < IMMORTAL))
    {
        /* The shit was poisoned ! */
        act("Ooups, it tasted rather strange ?!!?",ch,0,0,TO_CHAR, 0);
        act("$n coughs and utters some strange sounds.",ch,0,0,TO_ROOM, 0);

        if(number(1,100) < get_saves(ch,SAVE_TYPE_POISON) - 15) {
          send_to_char("Luckily, your body rejects the poison almost immediately.\n\r", ch);
        } else {
           af.type = SPELL_POISON;
           af.duration = temp->obj_flags.value[0]*2;
           af.modifier = 0;
           af.location = APPLY_NONE;
           af.bitvector = AFF_POISON;
           affect_join(ch,&af, FALSE, FALSE);
        }
    }

    extract_obj(temp);
    return eSUCCESS;
}


int do_pour(struct char_data *ch, char *argument, int cmd)
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct obj_data *from_obj;
    struct obj_data *to_obj;
    int amount;

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
      {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
      }

    argument_interpreter(argument, arg1, arg2);

    if(!*arg1) /* No arguments */
    {
        act("What do you want to pour from?",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(!(from_obj = get_obj_in_list_vis(ch,arg1,ch->carrying)))
    {
        act("You can't find it!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(from_obj->obj_flags.type_flag!=ITEM_DRINKCON)
    {
        act("You can't pour from that!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(from_obj->obj_flags.value[1]==0)
    {
        act("The $p is empty.", ch,from_obj, 0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(!*arg2)
    {
        act("Where do you want it? Out or in what?",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(!str_cmp(arg2,"out"))
    {
        act("$n empties $p", ch,from_obj,0,TO_ROOM, INVIS_NULL);
        act("You empty the $p.", ch,from_obj,0,TO_CHAR, 0);

        from_obj->obj_flags.value[1]=0;
        from_obj->obj_flags.value[2]=0;
        from_obj->obj_flags.value[3]=0;
        return eSUCCESS;
    }

    if(!(to_obj = get_obj_in_list_vis(ch,arg2,ch->carrying)))
    {
        act("You can't find it!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(to_obj->obj_flags.type_flag!=ITEM_DRINKCON)
    {
        act("You can't pour anything into that.",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if (to_obj == from_obj)
      {
        act("A most unproductive effort.",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
      }
    
    if((to_obj->obj_flags.value[1]!=0)&&
        (to_obj->obj_flags.value[2]!=from_obj->obj_flags.value[2]))
    {
        act("There is already a different liquid in it!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(!(to_obj->obj_flags.value[1]<to_obj->obj_flags.value[0]))
    {
        act("There is no room for more.",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    sprintf(buf,"You pour the %s into the %s.\n\r",
        drinks[from_obj->obj_flags.value[2]],arg2);
    send_to_char(buf,ch);

    /* First same type liq. */
    to_obj->obj_flags.value[2]=from_obj->obj_flags.value[2];

    /* Then how much to pour */
    from_obj->obj_flags.value[1]-= (amount=
        (to_obj->obj_flags.value[0]-to_obj->obj_flags.value[1]));

    to_obj->obj_flags.value[1]=to_obj->obj_flags.value[0];

    if(from_obj->obj_flags.value[1]<0)    /* There was to little */
    {
        to_obj->obj_flags.value[1]+=from_obj->obj_flags.value[1];
        amount += from_obj->obj_flags.value[1];
        from_obj->obj_flags.value[1]=0;
        from_obj->obj_flags.value[2]=0;
        from_obj->obj_flags.value[3]=0;
    }

    /* Then the poison boogie */
    to_obj->obj_flags.value[3]=
        (to_obj->obj_flags.value[3]||from_obj->obj_flags.value[3]);

    return eSUCCESS;
}

int do_sip(struct char_data *ch, char *argument, int cmd)
{
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct obj_data *temp;

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
      {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
      }

    one_argument(argument,arg);

    if(!(temp = get_obj_in_list_vis(ch,arg,ch->carrying)))
    {
        act("You can't find it!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(temp->obj_flags.type_flag!=ITEM_DRINKCON)
    {
        act("You can't sip from that!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(GET_COND(ch,DRUNK)>10) /* The pig is drunk ! */
    {
        act("You simply fail to reach your mouth!",ch,0,0,TO_CHAR, 0);
        act("$n tries to sip, but fails!",ch,0,0,TO_ROOM, INVIS_NULL);
        return eFAILURE;
    }

    if(!temp->obj_flags.value[1])  /* Empty */
    {
        act("But there is nothing in it?",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    act("$n sips from the $o",ch,temp,0,TO_ROOM, INVIS_NULL);
    sprintf(buf,"It tastes like %s.\n\r",drinks[temp->obj_flags.value[2]]);
    send_to_char(buf,ch);

    gain_condition(ch,DRUNK,
        (int)(drink_aff[temp->obj_flags.value[2]][DRUNK]/4));

    if(GET_COND(ch,DRUNK)>10)
        act("You feel drunk.",ch,0,0,TO_CHAR, 0);

    if(temp->obj_flags.value[3] == 1 && !IS_AFFECTED(ch,AFF_POISON))
      act("But it also had a strange taste!",ch,0,0,TO_CHAR, 0);

    temp->obj_flags.value[1]--;

    if(!temp->obj_flags.value[1])  /* The last bit */
    {
        temp->obj_flags.value[2]=0;
        temp->obj_flags.value[3]=0;
    }

    return eSUCCESS;
}


int do_taste(struct char_data *ch, char *argument, int cmd)
{
    char arg[MAX_STRING_LENGTH];
    struct obj_data *temp;

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
      {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
      }

    one_argument(argument,arg);

    if(!(temp = get_obj_in_list_vis(ch,arg,ch->carrying)))
    {
        act("You can't find it!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    if(temp->obj_flags.type_flag==ITEM_DRINKCON)
    {
        return do_sip(ch,argument,0);
    }

    if(!(temp->obj_flags.type_flag==ITEM_FOOD))
    {
        act("Taste that?!? Your stomach refuses!",ch,0,0,TO_CHAR, 0);
        return eFAILURE;
    }

    act("$n tastes the $o.",  ch, temp, 0, TO_ROOM, INVIS_NULL);
    act("You taste the $o.",  ch, temp, 0, TO_CHAR, 0);

    if(temp->obj_flags.value[3] == 1 && !IS_AFFECTED(ch,AFF_POISON)) {
      act("Oops, it did not taste good at all!",ch,0,0,TO_CHAR, 0);
    }

    temp->obj_flags.value[0]--;

    if(!temp->obj_flags.value[0])   /* Nothing left */
    {
        act("There is nothing left now.",ch,0,0,TO_CHAR, 0);
        extract_obj(temp);
    }

    return eSUCCESS;
}


/* functions related to wear */

void perform_wear(struct char_data *ch, struct obj_data *obj_object,
        int keyword)
{
    switch(keyword) {
        case 0 : 
            act("$n wears $p on $s finger.", ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 1 : 
            act("You wear $p around your neck.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p around $s neck.",  ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 2 : 
            act("You wear $p on your body.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p on $s body.", ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 3 : 
            act("You wear $p on your head.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p on $s head.",ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 4 : 
            act("You wear $p on your legs.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p on $s legs.", ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 5 : 
            act("You wear $p on your feet.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p on $s feet.",ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 6 : 
            act("You wear $p on your hands.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p on $s hands.",ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 7 : 
            act("You wear $p on your arms.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p on $s arms.",ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 8 : 
            act("You wear $p on your body.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p about $s body.",ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 9 : 
            act("You wear $p on your waist.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p about $s waist.",ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 10 : 
            act("$n wears $p around $s wrist.", ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 11 :
            act("You wear $p on your face.",ch,obj_object,0,TO_CHAR,0);
            act("$n wears $p on $s face.", ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 12 : 
            act("You wield $p.",ch,obj_object,0,TO_CHAR,0);
            act("$n wields $p.",ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 13 : 
            act("You start using $p as a shield.",ch,obj_object,0,TO_CHAR,0);
            act("$n starts using $p as a shield.", ch, obj_object,0,TO_ROOM, INVIS_NULL);
           break;
        case 14 : 
            act("You grab $p.",ch,obj_object,0,TO_CHAR,0);
            act("$n grabs $p.", ch, obj_object,0,TO_ROOM, INVIS_NULL);
            break;
        case 15 :
            act("$n wears $p in $s ear.", ch, obj_object,0,TO_ROOM, 0);
            break;
        case 16 :
            act("You light $p and hold it.",ch,obj_object,0,TO_CHAR,0);
            act("$n lights $p and holds it.", ch, obj_object,0,TO_ROOM, 0);
            break;
    }
}

int class_restricted(struct char_data *ch, struct obj_data *obj)
{
if (IS_NPC(ch)) return FALSE;
if (IS_OBJ_STAT(obj, ITEM_ANY_CLASS))
    return FALSE;

if ((IS_OBJ_STAT(obj, ITEM_WARRIOR) && (GET_CLASS(ch) == CLASS_WARRIOR)) ||
    (IS_OBJ_STAT(obj, ITEM_MAGE) && (GET_CLASS(ch) == CLASS_MAGIC_USER)) ||
    (IS_OBJ_STAT(obj, ITEM_THIEF) && ( GET_CLASS(ch) == CLASS_THIEF)) ||
    (IS_OBJ_STAT(obj, ITEM_CLERIC) && (GET_CLASS(ch) == CLASS_CLERIC)) ||
    (IS_OBJ_STAT(obj, ITEM_PAL) && (GET_CLASS(ch) == CLASS_PALADIN)) ||
    (IS_OBJ_STAT(obj, ITEM_ANTI) && (GET_CLASS(ch) == CLASS_ANTI_PAL)) ||
    (IS_OBJ_STAT(obj, ITEM_BARB) && (GET_CLASS(ch) == CLASS_BARBARIAN)) ||
    (IS_OBJ_STAT(obj, ITEM_RANGER) && (GET_CLASS(ch) == CLASS_RANGER)) ||
    (IS_OBJ_STAT(obj, ITEM_BARD) && (GET_CLASS(ch) == CLASS_BARD)) ||
    (IS_OBJ_STAT(obj, ITEM_DRUID) && (GET_CLASS(ch) == CLASS_DRUID)) ||
    (IS_OBJ_STAT(obj, ITEM_MONK) && (GET_CLASS(ch) == CLASS_MONK))) 
    return FALSE;
return TRUE;
}

int charmie_restricted(struct char_data *ch, struct obj_data *obj, int wear_loc)
{
  return FALSE; // sigh, work for nohin'
  if (IS_NPC(ch) && ISSET(ch->affected_by, AFF_CHARM) && ch->master && ch->mobdata)
  {
	int vnum = mob_index[ch->mobdata->nr].virt;
	if (vnum == 8 || (vnum > 22388 && vnum < 22399))
           return FALSE; // golems and corpses wear all
        switch (ch->race)
	{
		case RACE_GHOST:
		case RACE_ELEMENT:
		case RACE_SLIME:
		   return TRUE; // screw 'em!
		case RACE_REPTILE:
		case RACE_DRAGON:
		case RACE_SNAKE:
		case RACE_HORSE:
		case RACE_BIRD:
		case RACE_RODENT:
		case RACE_FISH:
		case RACE_ARACHNID:
		case RACE_INSECT:
		case RACE_ANIMAL:
		case RACE_TREE:
		case RACE_FELINE:
		  switch (wear_loc)
		  {
			case WEAR_ABOUT:
			case WEAR_FINGER_L:
			case WEAR_FINGER_R:
			case WEAR_EAR_L:
			case WEAR_EAR_R:
			case WEAR_WRIST_L:
			case WEAR_WRIST_R:
			case WEAR_NECK_1:
			case WEAR_FACE:
				return FALSE;
			default:
				return TRUE;
		  }			
		break;
             default:
		return FALSE;
	}
  }
  return FALSE;
}

int size_restricted(struct char_data *ch, struct obj_data *obj)
{
  if(IS_SET(obj->obj_flags.size, SIZE_ANY))
    return FALSE;

  if(GET_RACE(ch) == RACE_HUMAN)  // human can wear all sizes
    return FALSE;

  if(IS_MOB(ch))  // mobs (ie charmies) can wear all sizes
    return FALSE;

  if(GET_HEIGHT(ch) < 42) {
    if(IS_SET(obj->obj_flags.size, SIZE_SMALL))
      return FALSE;
    else return TRUE;
  }

  if(GET_HEIGHT(ch) < 66) {
    if(IS_SET(obj->obj_flags.size, SIZE_SMALL) || IS_SET(obj->obj_flags.size, SIZE_MEDIUM))
      return FALSE;
    else return TRUE;
  }

  if(GET_HEIGHT(ch) < 79) {
    if(IS_SET(obj->obj_flags.size, SIZE_MEDIUM))
      return FALSE;
    else return TRUE;
  }

  if(GET_HEIGHT(ch) < 103) {
    if(IS_SET(obj->obj_flags.size, SIZE_LARGE) || IS_SET(obj->obj_flags.size, SIZE_MEDIUM))
      return FALSE;
    else return TRUE;
  }

  if(IS_SET(obj->obj_flags.size, SIZE_LARGE))
      return FALSE;

  return TRUE;
}

// find out if wearing/removing an item will screw up the items a player
// it wearing in terms of sizes vs. height
// ch = player obj = obj to remove/wear add = 1(wear) or 0(remove)
// function WILL tell the character if anything is wrong
int will_screwup_worn_sizes(char_data * ch, obj_data * obj, int add)
{
  int j;
  int mod = 0;

  // find out if the item affects the person's height
  for(j=0; j < obj->num_affects; j++)
    if(obj->affected[j].location == APPLY_CHAR_HEIGHT)
      mod += obj->affected[j].modifier;

  if(!mod)
    return FALSE;

  // temporarily affect the person's height
  if(add) {
//	  logf(ANGEL, LOG_BUG, "will_screwup_worn_sizes: %s height %d by %d = %d", GET_NAME(ch), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)+mod);
	  GET_HEIGHT(ch) += mod;
  } else {
//	  logf(ANGEL, LOG_BUG, "will_screwup_worn_sizes: %s height %d by -%d = %d", GET_NAME(ch), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)-mod);
	  GET_HEIGHT(ch) -= mod;
  }

  if(add == 1 && size_restricted(ch, obj))
  {
    // Only have to check the item itself if we're wearing it, not removing
//	  logf(ANGEL, LOG_BUG, "will_screwup_worn_sizes: %s height %d by -%d = %d", GET_NAME(ch), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)-mod);
	  GET_HEIGHT(ch) -= mod;
    send_to_char("After modifying your height that item would not fit!\r\n", ch);
    return TRUE;
  }

  int problem = 0;
  for(j = 0; j < MAX_WEAR; j++)
  {
    if(ch->equipment[j] == obj || !ch->equipment[j])
      continue;

    if(size_restricted(ch, ch->equipment[j])) {
      problem = 1;
      break;
    }
  }

  // fix height back to normal
  if(add) {
//	  logf(ANGEL, LOG_BUG, "will_screwup_worn_sizes: %s height %d by -%d = %d", GET_NAME(ch), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)-mod);
	  GET_HEIGHT(ch) -= mod;
  } else {
//	  logf(ANGEL, LOG_BUG, "will_screwup_worn_sizes: %s height %d by %d = %d", GET_NAME(ch), GET_HEIGHT(ch), mod, GET_HEIGHT(ch)+mod);
	  GET_HEIGHT(ch) += mod;
  }

  if(problem)
  {
    if(add)
      csendf(ch, "Wearing that would cause your %s to no longer fit!\r\n", ch->equipment[j]->short_description);
    else csendf(ch, "Removing that would cause your %s to no longer fit!\r\n", ch->equipment[j]->short_description);

    return TRUE;
  }
  
  return FALSE;
}

void wear(struct char_data *ch, struct obj_data *obj_object, int keyword)
{
  struct obj_data *obj;
  char buffer[MAX_STRING_LENGTH];
  int do_say(struct char_data *ch, char *argument, int cmd);
  if (!obj_object) return;

  obj = obj_object;
  if(class_restricted(ch, obj_object))
  {
    send_to_char("You are forbidden.\n\r", ch);
    return;
  }

  if(size_restricted(ch, obj_object))
  {
    send_to_char("The size does not fit.\r\n", ch);
    return;
  }

  if(will_screwup_worn_sizes(ch, obj_object, 1))
  {
    // will_screwup_worn_sizes() takes care of the messages
    return;
  }

  if (!IS_NPC(ch)) {
    if(GET_LEVEL(ch) < obj_object->obj_flags.eq_level) {
      sprintf(buffer, "You must be level %d to use this object.\n\r",
        obj_object->obj_flags.eq_level);
      send_to_char(buffer, ch);
      return;
    }
  } else {
  if (mob_index[ch->mobdata->nr].virt != 8)
      if(GET_LEVEL(ch) < obj_object->obj_flags.eq_level) {
        sprintf(buffer, "You must be level %d to use this object.\n\r",
          obj_object->obj_flags.eq_level);
        send_to_char(buffer, ch);
        return;
      }
  }
/*  if (IS_NPC(ch) && (mob_index[ch->mobdata->nr].virt < 22394 &&
	mob_index[ch->mobdata->nr].virt > 22388))
  {
     return;
  }*/

  if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) &&
     !isname(GET_NAME(ch), obj->name) && GET_LEVEL(ch) < IMP) {
    send_to_char("This item will only work on its rightful owner.\n\r", ch);
    return;
  }

  if(IS_FAMILIAR(ch)) {
    send_to_char("Familiar's cannot wear eq!\n\r", ch);
    return;
  } 
  switch (keyword) {
  case 0: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_FINGER)) {
      if (charmie_restricted(ch, obj_object, WEAR_FINGER_L))
	  send_to_char("You cannot wear this.\r\n",ch);
      else  if((ch->equipment[WEAR_FINGER_L])
        && (ch->equipment[WEAR_FINGER_R])) {
        act("You are already wearing $p on your left ring-finger.", ch, ch->equipment[WEAR_FINGER_L], 0, TO_CHAR, 0);
        act("You are already wearing $p on your right ring-finger.", ch, ch->equipment[WEAR_FINGER_R], 0, TO_CHAR, 0);
      }
      else {
        perform_wear(ch,obj_object,keyword);
        if(ch->equipment[WEAR_FINGER_L]) {
          sprintf(buffer, "You put the %s on your right ring-finger.\n\r", 
                  fname(obj_object->name));
          send_to_char(buffer, ch);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_FINGER_R);
        }
        else {
          sprintf(buffer, "You put the %s on your left ring-finger.\n\r", 
                  fname(obj_object->name));
          send_to_char(buffer, ch);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_FINGER_L);
        }
      }
    }
    else
      send_to_char("You can't wear that on your finger.\n\r", ch);
  } break;
  case 1: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_NECK)) {
      if (charmie_restricted(ch, obj_object, WEAR_NECK_1))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if((ch->equipment[WEAR_NECK_1])
        && (ch->equipment[WEAR_NECK_2])) {
        act("You are already wearing $p on your neck.", ch, ch->equipment[WEAR_NECK_1], 0, TO_CHAR, 0);
        act("You are already wearing $p on your neck.", ch, ch->equipment[WEAR_NECK_2], 0, TO_CHAR, 0);
      }
      else {
        perform_wear(ch,obj_object,keyword);
        if(ch->equipment[WEAR_NECK_1]) {
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_NECK_2);
        }
        else {
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_NECK_1);
        }
      }
    }
    else
      send_to_char("You can't wear that around your neck.\n\r", ch);
  } break;
  case 2: {
     if(CAN_WEAR(obj_object,ITEM_WEAR_BODY)) {
      if (charmie_restricted(ch, obj_object, WEAR_BODY))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_BODY]) {
        act("You already wear $p on your body.", ch, ch->equipment[WEAR_BODY], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        equip_char(ch,  obj_object, WEAR_BODY);
      }
    }
    else
      send_to_char("You can't wear that on your body.\n\r", ch);
  } break;
  case 3: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_HEAD)) {
      if (charmie_restricted(ch, obj_object, WEAR_HEAD))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_HEAD]) {
        act("You already wear $p on your head.", ch, ch->equipment[WEAR_HEAD], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        equip_char(ch, obj_object, WEAR_HEAD);
      }
    }
    else
      send_to_char("You can't wear that on your head.\n\r", ch);
  } break;
  case 4: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_LEGS)) {
      if (charmie_restricted(ch, obj_object, WEAR_LEGS))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_LEGS]) {
        act("You already wear $p on your legs.", ch, ch->equipment[WEAR_LEGS], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        equip_char(ch, obj_object, WEAR_LEGS);
      }
    }
    else
      send_to_char("You can't wear that on your legs.\n\r", ch);
  } break;
  case 5: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_FEET)) {
      if (charmie_restricted(ch, obj_object, WEAR_FEET))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_FEET]) {
        act("You already wear $p on your feet.", ch, ch->equipment[WEAR_FEET], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        equip_char(ch, obj_object, WEAR_FEET);
      }
    }
    else
      send_to_char("You can't wear that on your feet.\n\r", ch);
  } break;
  case 6: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_HANDS)) {
      if (charmie_restricted(ch, obj_object, WEAR_HANDS))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_HANDS]) {
        act("You already wear $p on your hands.", ch, ch->equipment[WEAR_HANDS], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        equip_char(ch, obj_object, WEAR_HANDS);
      }
    }
    else
      send_to_char("You can't wear that on your hands.\n\r", ch);
  } break;
  case 7: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_ARMS)) {
      if (charmie_restricted(ch, obj_object, WEAR_ARMS))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_ARMS]) {
        act("You already wear $p on your arms.", ch, ch->equipment[WEAR_ARMS], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        equip_char(ch, obj_object, WEAR_ARMS);
      }
    }
    else
      send_to_char("You can't wear that on your arms.\n\r", ch);
  } break;
  case 8: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_ABOUT)) {
      if (charmie_restricted(ch, obj_object, WEAR_ABOUT))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_ABOUT]) {
        act("You already wear $p about your body.", ch, ch->equipment[WEAR_ABOUT], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        equip_char(ch, obj_object, WEAR_ABOUT);
      }
    }
    else
      send_to_char("You can't wear that about your body.\n\r", ch);
  } break;
  case 9: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_WAISTE)) {
      if (charmie_restricted(ch, obj_object, WEAR_WAISTE))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_WAISTE]) {
        act("You already wear $p about your waist.", ch, ch->equipment[WEAR_WAISTE], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch, obj_object, keyword);
        equip_char(ch,  obj_object, WEAR_WAISTE);
      }
    }
    else
      send_to_char("You can't wear that about your waist.\n\r", ch);
  } break;
  case 10: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_WRIST)) {
      if (charmie_restricted(ch, obj_object, WEAR_WRIST_L))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if((ch->equipment[WEAR_WRIST_L])
        && (ch->equipment[WEAR_WRIST_R])) {
        act("You already wear $p around your left wrist.", ch, ch->equipment[WEAR_WRIST_L], 0, TO_CHAR, 0);
        act("You already wear $p around your right wrist.", ch, ch->equipment[WEAR_WRIST_R], 0, TO_CHAR, 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        if(ch->equipment[WEAR_WRIST_L]) {
          sprintf(buffer,"You wear the %s around your right wrist.\n\r", 
                  fname(obj_object->name));
          send_to_char(buffer, ch);
          equip_char(ch,  obj_object, WEAR_WRIST_R);
        }
        else {
          sprintf(buffer,"You wear the %s around your left wrist.\n\r", 
                  fname(obj_object->name));
          send_to_char(buffer, ch);
          equip_char(ch, obj_object, WEAR_WRIST_L);
        }
      }
    }
    else
      send_to_char("You can't wear that around your wrist.\n\r", ch);
  } break;

  case 11: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_FACE)) {
      if (charmie_restricted(ch, obj_object, WEAR_FACE))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if((ch->equipment[WEAR_FACE])) {
        send_to_char("You only have one face.\n\r", ch);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        equip_char(ch, obj_object, WEAR_FACE);
      }
    }
    else
      send_to_char("You can't wear that on your face!\n\r", ch);
  } break;

  case 12:
    if(CAN_WEAR(obj_object,ITEM_WIELD)) {
      if(!ch->equipment[WIELD] && GET_OBJ_WEIGHT(obj_object) > MIN(GET_STR(ch), get_max_stat(ch, STR)) && 
!ISSET(ch->affected_by,AFF_POWERWIELD))
        send_to_char("It is too heavy for you to use.\n\r",ch);
      else if(ch->equipment[WIELD] && GET_OBJ_WEIGHT(obj_object) > 
MIN(GET_STR(ch)/2, get_max_stat(ch, STR)/2) && 
!ISSET(ch->affected_by, AFF_POWERWIELD))

        act("$p is too heavy for you to use as a secondary weapon.", ch, obj_object, 0, TO_CHAR, 0);
        
      else if((!hands_are_free(ch, 2)) && 
             (IS_SET(obj_object->obj_flags.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD)))
        send_to_char("You need both hands for this weapon.\n\r", ch);
      else if(!hands_are_free(ch, 1))
        send_to_char("Your hands are already full.\n\r", ch);

      else if(IS_AFFECTED(ch, AFF_CHARM)) {
        send_to_char("Sorry, charmies can't wield stuff anymore:(\r\n", ch);
        do_say(ch, "I'm sorry my master, I lack the dexterity.", 0);
      }
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        if(ch->equipment[WIELD])
          equip_char(ch, obj_object, SECOND_WIELD);
        else
          equip_char(ch, obj_object, WIELD);
      }
    }
    else
      send_to_char("You can't wield that.\n\r", ch);
    break;

  case 13: {
    if(CAN_WEAR(obj_object, ITEM_WEAR_SHIELD)) {
      if (charmie_restricted(ch, obj_object, WEAR_SHIELD))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(ch->equipment[WEAR_SHIELD]) {
        act("You already using $p as a shield.", ch, ch->equipment[WEAR_SHIELD], 0, TO_CHAR, 0);
      }
      else if((!hands_are_free(ch, 2)) && 
             (IS_SET(obj_object->obj_flags.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD)))
      {
	send_to_char("You need both hands for this shield.\r\n",ch);
      }
      else if(!hands_are_free(ch, 1))
        send_to_char("Your hands are already full.\n\r", ch);

      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
        equip_char(ch, obj_object, WEAR_SHIELD);
      }
    }

    else
      send_to_char("You can't use that as a shield.\n\r", ch);
  } break;

  case 14:
    if(CAN_WEAR(obj_object, ITEM_HOLD)) {

      if (charmie_restricted(ch, obj_object, HOLD))
	  send_to_char("You cannot wear this.\r\n",ch);
      else if(!hands_are_free(ch, 1))
        send_to_char("Your hands are already full.\n\r", ch);
      else if((!hands_are_free(ch, 2)) && 
             (IS_SET(obj_object->obj_flags.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD)))
      {
	send_to_char("You need both hands for this item.\r\n",ch);
      }
      else if (obj_object->obj_flags.extra_flags == ITEM_INSTRUMENT && ((ch->equipment[HOLD] && ch->equipment[HOLD]->obj_flags.type_flag == ITEM_INSTRUMENT) ||	(ch->equipment[HOLD2] && ch->equipment[HOLD2]->obj_flags.type_flag == ITEM_INSTRUMENT)))
	{
	  send_to_char("You're busy enough playing the instrument you're already using.\r\n",ch);
	}
      else {
        obj_from_char(obj_object);
        perform_wear(ch,obj_object,keyword);
	if (ch->equipment[HOLD])
	equip_char(ch, obj_object, HOLD2);
      else  equip_char(ch, obj_object, HOLD);
      }
    }
    else
      send_to_char("You can't hold this.\n\r", ch);
  break;

  case 15: {
    if(CAN_WEAR(obj_object,ITEM_WEAR_EAR)) {
      if (charmie_restricted(ch, obj_object, WEAR_EAR_L))
	  send_to_char("You cannot wear this.\r\n",ch);
     else if((ch->equipment[WEAR_EAR_L])
        && (ch->equipment[WEAR_EAR_R])) {
        act("You already wearing $p on your left ear.", ch, ch->equipment[WEAR_EAR_L], 0, TO_CHAR, 0);
        act("You already wearing $p on your right ear.", ch, ch->equipment[WEAR_EAR_R], 0, TO_CHAR, 0);
      }
      else {
        perform_wear(ch,obj_object,keyword);
        if(ch->equipment[WEAR_EAR_L]) {
          act("You wear $p in your right ear.",ch,obj_object,0,TO_CHAR,0);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_EAR_R);
        }
        else {
          act("You wear $p in your left ear.",ch,obj_object,0,TO_CHAR,0);
          obj_from_char(obj_object);
          equip_char(ch, obj_object, WEAR_EAR_L);
        }
      }
    }
    else
      send_to_char("You can't wear that in your ear.\n\r", ch);
  } break;

  case 16: {  /* LIGHT SOURCE */
      if (charmie_restricted(ch, obj_object, WEAR_LIGHT))
	  send_to_char("You cannot wear this.\r\n",ch);
   else if(ch->equipment[WEAR_LIGHT]) {
      act("You are already holding $p as a light.", ch, ch->equipment[WEAR_LIGHT], 0, TO_CHAR, 0);
    }
      else if((!hands_are_free(ch, 2)) && 
             (IS_SET(obj_object->obj_flags.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD)))
      {
	send_to_char("You need both hands for this light.\r\n",ch);
      }
    else if(!hands_are_free(ch, 1)) 
      send_to_char("Your hands are already full.\n\r", ch);
    else if(obj_object->obj_flags.type_flag != ITEM_LIGHT)
      send_to_char("That isn't a light you cheating fuck!\n\r", ch);
    else {
      obj_from_char(obj_object);
      perform_wear(ch,obj_object,keyword);
      equip_char(ch, obj_object, WEAR_LIGHT);
    }
  } break;



  case 17: //primary
    if(CAN_WEAR(obj_object,ITEM_WIELD)) 
    {
       //if not wielding anything, just call regular wield
       if(!ch->equipment[WIELD])
       {
         wear(ch, obj_object, 12);
         return;
       }

       if(!hands_are_free(ch, 1))
       {
         send_to_char("Your hands are already full.\n\r", ch);
         break;
       }


       if (GET_MOVE(ch) < 4) {
         send_to_char("You are too tired to switch your weapons!", ch);
         return;
       }
       GET_MOVE(ch) -= 4;


       if (!has_skill(ch, SKILL_SWITCH) || !skill_success(ch,NULL,SKILL_SWITCH)) 
       {
         act("$n fails to switch $s weapons.", ch,0,0, TO_ROOM, 0);
         act("You fail to switch your weapons.", ch, 0,0, TO_CHAR, 0);
         return;
       }

       struct obj_data *obj_temp = ch->equipment[WIELD];
       obj_to_char(unequip_char(ch, WIELD) , ch);
       wear(ch, obj_object, 12);
       wear(ch, obj_temp, 12);
       return;
    }
    else
      send_to_char("You can't wield that.\n\r", ch);
  break;




  case -1: {
    sprintf(buffer,"Wear %s where?.\n\r", fname(obj_object->name));
    send_to_char(buffer, ch);
  } break;
  case -2: {
    sprintf(buffer,"You can't wear the %s.\n\r",
            fname(obj_object->name));
    send_to_char(buffer, ch);
  } break;
  default: {
    log("Unknown type called in wear.", ANGEL, LOG_BUG);
  } break;
 }
        redo_hitpoints(ch);
        redo_mana(ch);
	redo_ki(ch);

}

int keywordfind(struct obj_data *obj_object)
{
    int keyword;

    keyword = -2;
    if (CAN_WEAR(obj_object,ITEM_WEAR_FINGER))      keyword = 0;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_NECK))   keyword = 1;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_BODY))   keyword = 2;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_HEAD))   keyword = 3;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_LEGS))   keyword = 4;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_FEET))   keyword = 5;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_HANDS))  keyword = 6;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_ARMS))   keyword = 7;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_ABOUT))  keyword = 8;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_WAISTE)) keyword = 9;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_WRIST))  keyword = 10;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_FACE))   keyword = 11;
    else if (CAN_WEAR(obj_object,ITEM_WIELD))       keyword = 12;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_SHIELD)) keyword = 13;
    else if (CAN_WEAR(obj_object,ITEM_HOLD))        keyword = 14;
    else if (CAN_WEAR(obj_object,ITEM_WEAR_EAR))    keyword = 15;
    return keyword;
}

int do_wear(struct char_data *ch, char *argument, int cmd)
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[256];
    char buffer[MAX_STRING_LENGTH];
    struct obj_data *obj_object, *tmp_object, *next_obj;
    int keyword;
    bool blindlag = FALSE;
    static char *keywords[] = {
        "finger",
        "neck",
        "body",
        "head",
        "legs",
        "feet",
        "hands",
        "arms",
        "about",
        "waist",
        "wrist",
        "face",
        "wield",
        "shield",
        "hold",
        "ear",
        "light",
	"primary",
        "\n"
    };

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
      {
      send_to_char ("SHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
      }
  
    argument_interpreter(argument, arg1, arg2);

    if (!(*arg1))
    {
        send_to_char( "Wear what?\n\r", ch );
        return eFAILURE;
    }

    if ( !str_cmp( arg1, "all" ) )
    {
        for ( tmp_object = ch->carrying; tmp_object; tmp_object = next_obj ) {
            int keyword;
            next_obj = tmp_object->next_content;
            if ( !CAN_SEE_OBJ( ch, tmp_object ) )
                continue;
            keyword = keywordfind( tmp_object );
            if ( keyword != -2 )
                wear( ch, tmp_object, keyword );
        }

        return eSUCCESS;
    }

    obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if(!obj_object && IS_AFFECTED(ch, AFF_BLIND) && has_skill(ch, SKILL_BLINDFIGHTING)) {
       obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying, TRUE);
       blindlag = TRUE;
    }
    if (obj_object) {
        if (*arg2) {
            keyword = search_block(arg2, keywords, FALSE);
            if (keyword == -1) {
                sprintf(buf,
                "%s is an unknown body location.\n\r", arg2);
                send_to_char(buf, ch);
            } 
            else {
                 wear(ch, obj_object, keyword);
            }
        }
        else {
             wear(ch, obj_object, keywordfind(obj_object));
        }
        if(blindlag) WAIT_STATE(ch, PULSE_VIOLENCE);
    }
    else {
         sprintf(buffer, "You do not seem to have the '%s'.\n\r",arg1);
         send_to_char(buffer,ch);
    }
    return eSUCCESS;
}



int do_wield(struct char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  struct obj_data *obj_object;
  bool blindlag = FALSE;
  int keyword = 12;

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
      {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
      }

    argument_interpreter(argument, arg1, arg2);
    if (*arg1) 
    {
        obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
        if(!obj_object && IS_AFFECTED(ch, AFF_BLIND) && has_skill(ch, SKILL_BLINDFIGHTING)) 
        {
           obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying, TRUE);
           blindlag = TRUE;
        }
        if (obj_object) 
	{
	  if (*arg2) 
          {
            if(arg2[0] == 'p')
              keyword = 17;
          }          

	  wear(ch, obj_object, keyword);

          if(blindlag) WAIT_STATE(ch, PULSE_VIOLENCE);
        } else {
          sprintf(buffer, "You do not seem to have the '%s'.\n\r",arg1);
          send_to_char(buffer,ch);
        }
    } else {
        send_to_char("Wield what?\n\r", ch);
    }
    return eSUCCESS;
}


int do_grab(struct char_data *ch, char *argument, int cmd)
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH];
    struct obj_data *obj_object;
    bool blindlag = FALSE;

    if (IS_SET(world[ch->in_room].room_flags, QUIET))
      {
      send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
      return eFAILURE;
      }

    argument_interpreter(argument, arg1, arg2);

    if (*arg1) {
        obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
        if(!obj_object && IS_AFFECTED(ch, AFF_BLIND) && has_skill(ch, SKILL_BLINDFIGHTING)) {
           obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying, TRUE);
           blindlag = TRUE;
        }
        if (obj_object) {
            if (obj_object->obj_flags.type_flag == ITEM_LIGHT)
                wear(ch, obj_object, 16);
            else
                wear(ch, obj_object, 14);
            if(blindlag) WAIT_STATE(ch, PULSE_VIOLENCE);
        } else {
            sprintf(buffer, "You do not seem to have the '%s'.\n\r",arg1);
            send_to_char(buffer,ch);
        }
    } else {
        send_to_char("Hold what?\n\r", ch);
    }
    return eSUCCESS;
}


int hands_are_free(struct char_data *ch, int number)
{
  struct obj_data *wielded;
  int hands = 0;

   wielded = ch->equipment[WIELD];

  if(wielded)
    if (IS_SET(wielded->obj_flags.extra_flags, ITEM_TWO_HANDED) && !ISSET(ch->affected_by, AFF_POWERWIELD))
       hands = 2;

  if(ch->equipment[WIELD]) hands++;
  if(ch->equipment[SECOND_WIELD]) hands++;

  if(ch->equipment[WEAR_SHIELD]) {
         if (IS_SET(ch->equipment[WEAR_SHIELD]->obj_flags.extra_flags, ITEM_TWO_HANDED) &&
		!ISSET(ch->affected_by, AFF_POWERWIELD))
	hands++;
	hands++;
  } 
 if(ch->equipment[HOLD]) {
         if (IS_SET(ch->equipment[HOLD]->obj_flags.extra_flags, ITEM_TWO_HANDED) &&
		!ISSET(ch->affected_by, AFF_POWERWIELD))
	hands++;
  hands++;
  }
  if(ch->equipment[WEAR_LIGHT]) { 
         if (IS_SET(ch->equipment[WEAR_LIGHT]->obj_flags.extra_flags, ITEM_TWO_HANDED) &&
		!ISSET(ch->affected_by, AFF_POWERWIELD))
	hands++;
  hands++;
  }
  if (ch->equipment[HOLD2]) hands++;

       if(number == 1 && hands < 2) return(1);   
  else if(number == 2 && hands < 1) return(1);
  else                              return(0); 
}

int do_remove(struct char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH];
  struct obj_data *obj_object;
  bool blindlag = FALSE;
  int j;

  if (IS_SET(world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  one_argument(argument, arg1);

  if (*arg1) 
  {
    if (!strcmp(arg1, "all")) 
    {
      for (j=0; j<MAX_WEAR; j++) 
      {
        if (CAN_CARRY_N(ch) != IS_CARRYING_N(ch)) 
        {
          if (ch->equipment[j] && CAN_SEE_OBJ(ch, ch->equipment[j])) 
          {
            obj_object =  ch->equipment[j];
            if(IS_SET(obj_object->obj_flags.extra_flags, ITEM_NODROP) && GET_LEVEL(ch) <= MORTAL) 
            {
              sprintf(arg1, "You can't remove %s, it must be CURSED!\n\r", obj_object->short_description);
              send_to_char(arg1, ch);
              continue;
	    }
 	    if(obj_index[obj_object->item_number].virt == 30010 && obj_object->obj_flags.timer < 40)
 	    {
		csendf(ch, "The ruby brooch is bound to your flesh. You cannot remove it!\r\n");
		continue;
   	    }

            if(obj_index[obj_object->item_number].virt == SPIRIT_SHIELD_OBJ_NUMBER)
            {
              send_to_room("The spirit shield shimmers brightly then fades away.\n\r", ch->in_room);
              extract_obj(obj_object);
              continue;
            } else 
               obj_to_char(unequip_char(ch, j) , ch);
             act("You stop using $p.",ch,obj_object,0,TO_CHAR, 0);
             act("$n stops using $p.",ch,obj_object,0,TO_ROOM, INVIS_NULL);

          }
        } else {
          send_to_char("You can't carry that many items.\n\r", ch);
          j = MAX_WEAR;
        }
      }
    } else 
    {
      obj_object = get_object_in_equip_vis(ch, arg1, ch->equipment, &j, FALSE);
      if(!obj_object && IS_AFFECTED(ch, AFF_BLIND) && has_skill(ch, SKILL_BLINDFIGHTING)) {
         obj_object = get_object_in_equip_vis(ch, arg1, ch->equipment, &j, TRUE);
         blindlag = TRUE;
      }
      if (obj_object) 
      {
        if(CAN_CARRY_N(ch) != IS_CARRYING_N(ch)) 
        {
          if(IS_SET(obj_object->obj_flags.extra_flags, ITEM_NODROP) && GET_LEVEL(ch) <= MORTAL) 
          {
            sprintf(arg1, "You can't remove %s, it must be CURSED!\n\r", obj_object->short_description);
            send_to_char(arg1, ch);
            return eFAILURE;
	  }
 	  if(obj_index[obj_object->item_number].virt == 30010 && obj_object->obj_flags.timer < 40)
 	  {
	    csendf(ch, "The ruby brooch is bound to your flesh. You cannot remove it!\r\n");
	    return eFAILURE;
   	  }

          if(will_screwup_worn_sizes(ch, obj_object, 0))
          {
            // will_screwup_worn_sizes() takes care of the messages
            return eFAILURE;
          }
          if(j == WIELD) {
            obj_to_char(unequip_char(ch, j), ch);
            ch->equipment[WIELD] = ch->equipment[SECOND_WIELD];
            ch->equipment[SECOND_WIELD] = 0;
          }
          else if(obj_index[obj_object->item_number].virt == SPIRIT_SHIELD_OBJ_NUMBER)
          {
            send_to_room("The spirit shield shimmers brightly then fades away.\n\r", ch->in_room);
            extract_obj(obj_object);
            return eSUCCESS;
          }
          else 
            obj_to_char(unequip_char(ch, j), ch);

          act("You stop using $p.",ch,obj_object,0,TO_CHAR, 0);
          act("$n stops using $p.",ch,obj_object,0,TO_ROOM, INVIS_NULL);
          if(blindlag) WAIT_STATE(ch, PULSE_VIOLENCE);
          
        } else {
          send_to_char("You can't carry that many items.\n\r", ch);
          j = MAX_WEAR;
        }
      } else {
        send_to_char("You are not using it.\n\r", ch);
      }
    }
  } else {
    send_to_char("Remove what?\n\r", ch);
  }
  return eSUCCESS;
}

// Urizen, hack of will_screwup_worn_sizes
// Checks for, and removes items that are no longer
// wear-able, because of disarm, scrap etc.
int recheck_height_wears(char_data * ch)
{
  int j;
  struct obj_data *obj = NULL;
  if (!ch || IS_NPC(ch)) return eFAILURE;  // NPCs get to wear the stuff.

  for(j = 0; j < MAX_WEAR; j++)
  {
    if(!ch->equipment[j]) continue;

    if(size_restricted(ch, ch->equipment[j])) {
      obj = unequip_char(ch,j);
      obj_to_char(obj,ch);
      act("$n looks uncomfortable, and shifts $p into $s inventory.",ch, obj, NULL, TO_ROOM,0);
      act("$p feels uncomfortable and you shift it into your inventory.",ch, obj,NULL,TO_CHAR,0);
    }
  }
  return eSUCCESS;
}

bool fullSave(obj_data *obj)
{
    if (!obj)
	return 0;

  if (eq_current_damage(obj))
    return 1;

  obj_data *tmp_obj = get_obj(GET_OBJ_VNUM(obj));
  if(!tmp_obj)
  {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "crash bug! vault.cpp, tmp_obj was null! %s is obj", obj->name);
    log(buf, IMMORTAL, LOG_BUG);
    return 0;
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)) {
	  return 1;
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_24H_SAVE)) {
	  return 1;
  }

  if (strcmp(GET_OBJ_SHORT(obj), GET_OBJ_SHORT(tmp_obj)))
    return 1;

  if (strcmp(obj->name, tmp_obj->name)) // GL. and stuff.
    return 1;
  
  if (obj->obj_flags.extra_flags != tmp_obj->obj_flags.extra_flags)
	return 1;

  if (obj->obj_flags.more_flags != tmp_obj->obj_flags.more_flags)
	return 1;

  if (obj->obj_flags.type_flag == ITEM_STAFF && obj->obj_flags.value[1] != obj->obj_flags.value[2])
    return 1;

  if (obj->obj_flags.type_flag == ITEM_WAND && obj->obj_flags.value[1] != obj->obj_flags.value[2])
    return 1;

  if (obj->obj_flags.type_flag == ITEM_DRINKCON && obj->obj_flags.value[0] != obj->obj_flags.value[1])
    return 1;

  return 0;
}

void heightweight(char_data *ch, bool add)
{
  int i, j;
  for (i=0; i<MAX_WEAR; i++)
  {
    if (ch->equipment[i])
      for (j=0; j<ch->equipment[i]->num_affects; j++)
      {
        if (ch->equipment[i]->affected[j].location == APPLY_CHAR_HEIGHT)
          affect_modify(ch, ch->equipment[i]->affected[j].location,
                           ch->equipment[i]->affected[j].modifier,
                           -1, add);
	else if (ch->equipment[i]->affected[j].location == APPLY_CHAR_WEIGHT)
          affect_modify(ch, ch->equipment[i]->affected[j].location,
                           ch->equipment[i]->affected[j].modifier,
                           -1, add);
      }
  }
}
