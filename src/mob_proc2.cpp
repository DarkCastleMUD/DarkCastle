/***************************************************************************
 *  file: spec_pro2.c , Special module.                    Part of DIKUMUD *
 *  Usage: Procedures handling special procedures for object/room/mobile   *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: mob_proc2.cpp,v 1.8 2002/08/05 23:46:47 pirahna Exp $ */
#include <room.h>
#include <obj.h>
#include <connect.h>
#include <timeinfo.h>
#include <utility.h>
#include <character.h>
#include <handler.h>
#include <db.h>
#include <player.h>
#include <levels.h>
#include <interp.h>
#include <act.h>
#include <string.h>
#include <returnvals.h>

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif


/*   external vars  */

extern CWorld world;
 
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct time_info_data time_info;


/* extern procedures */

void hit(struct char_data *ch, struct char_data *victim, int type);
void gain_exp(struct char_data *ch, int gain);

float value_multiplier(struct obj_data *obj)
{
  // this is to keep containers from losing $$ since they can't be damaged
  if(GET_ITEM_TYPE(obj) == ITEM_CONTAINER) {
    return 1;
  }

  float v0 = (float)obj->obj_flags.value[0];
  float v1= (float)((obj->obj_flags.value[0]) - (obj->obj_flags.value[1]));

  if(v0 == 0) 
    return 1;
  else
    return (v1 / v0);
}

/* Data declarations */

struct social_type
{
  char *cmd;
  int next_line;
};


// TODO - clean up the repair guys.  We can probably pull some of this out into a 
// function and/or clear up the act's to run faster.
int repair_guy(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char buf[256], item[256];
  int value0, cost, price, x;
  int percent, eqdam;

  if ((cmd != 66) && (cmd != 65)) return eFAILURE;

  one_argument(arg, item);

  if (!*item) {
    send_to_char("What item?\n\r", ch);
    return eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, item, ch->carrying);

  if (obj == NULL) {
    send_to_char("You don't have that item.\n\r", ch);
    return eSUCCESS;
  }
  if(IS_OBJ_STAT(obj, ITEM_NOREPAIR)) {
    act("The Repair Guy says, 'I can't fix this.'", ch, 0, 0, TO_CHAR, 0);     
    return eSUCCESS;
  }

  act("You give The Repair Guy $p.",ch,obj,0,TO_CHAR, 0);
  act("$n gives $p to The Repair Guy.", ch,obj,0, TO_ROOM, INVIS_NULL);
  act("\n\rThe Repair Guy examines $p...",ch,obj,0, TO_CHAR, 0);
  act("\n\rThe Repair Guy examines $p...",ch,obj,0, TO_ROOM, INVIS_NULL);

  if((obj->obj_flags.type_flag != ITEM_ARMOR) || 
      ( IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED))) {

    act("The Repair Guy says 'I can't repair this.'",ch,0,0, TO_CHAR, 0);
    act("The Repair Guy says 'I can't repair this.'",ch,0,0, TO_ROOM, INVIS_NULL);
    act("The Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
    act("The Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }
  cost = obj->obj_flags.cost;
  value0 = eq_max_damage(obj);
  eqdam = eq_current_damage(obj);

  if (eqdam == 0) {
    act("The Repair Guy says 'Looks fine to me.'",ch,0,0, TO_CHAR, 0);
    act("The Repair Guy says 'Looks fine to me.'",ch,0,0, TO_ROOM, 0);
    act("The Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
    act("The Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }
  if (eqdam <= 0)
    percent = 0;
  else
    percent = ((100* eqdam) / value0);

  x = (100 - percent);          // now we know what percent to repair ..  
  price = ((cost * x) / 100);   // now we know what to charge them fuckers! 

  if (price < 100)
    price = 100;     // Welp.. Repair Guy needs to feed the kids somehow.. :)
  
  if (GET_GOLD(ch) < (uint32)price) {
    act("The Repair Guy says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_CHAR, 0);
    act("The Repair Guy says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_ROOM, 0);
    sprintf(buf, "The Repair Guy says 'It would cost %d coins to repair %s, which you don't have!'",
          price, obj->short_description);
    act(buf, ch,0,0, TO_CHAR, 0);
    act(buf, ch,0,0, TO_ROOM, 0);
    act("The Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
    act("The Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
    return eSUCCESS;

  } 

  if (cmd == 65) {
      sprintf(buf, "The Repair Guy says 'It will only cost you %d coins to repair %s.'",
          price, obj->short_description);
      act(buf, ch,0,0, TO_CHAR, 0);
      act(buf, ch,0,0, TO_ROOM, 0);
      act("The Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
      act("The Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
      return eSUCCESS;
  }   /* price quote */

  GET_GOLD(ch) -= price;
  eq_remove_damage(obj);
  sprintf(buf, "The Repair Guy says 'It will cost you %d coins to repair %s.'",
          price, obj->short_description);
  act(buf, ch,0,0, TO_CHAR, 0);
  act(buf, ch,0,0, TO_ROOM, 0);
  act("You watch The Repair Guy fix $p...\n\r\n\r"
        "The Repair Guy says 'All fixed!'\n\r"
        "The Repair Guy gives you $p.", ch, obj, 0, TO_CHAR, 0);
  act("You watch The Repair Guy fix $p...\n\r\n\r"
        "The Repair Guy says 'All fixed!'\n\r"
        "The Repair Guy gives $n $p.", ch, obj, 0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}

int super_repair_guy(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char buf[256], item[256];
  int value0, value2, cost, price, x;
  int percent, eqdam;

  if ((cmd != 66) && (cmd != 65)) return eFAILURE;

  one_argument(arg, item);

  if (!*item) {
    send_to_char("What item?\n\r", ch);
    return eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, item, ch->carrying);

  if (obj == NULL) {
    send_to_char("You don't have that item.\n\r", ch);
    return eSUCCESS;
  }
  if(IS_OBJ_STAT(obj, ITEM_NOREPAIR)) {
    act("The Super Repair Guy says, 'I can't fix this.'", ch, 0,0, TO_ROOM, 0);
    return eSUCCESS;
  }
  act("You give The Super Repair Guy $p.",ch,obj,0,TO_CHAR, 0);
  act("$n gives $p to The Super Repair Guy.", ch,obj,0, TO_ROOM, INVIS_NULL);
  act("\n\rThe Super Repair Guy examines $p...",ch,obj,0, TO_CHAR, 0);
  act("\n\rThe Super Repair Guy examines $p...",ch,obj,0, TO_ROOM, INVIS_NULL);

  if (obj->obj_flags.type_flag == ITEM_ARMOR) {

    cost = obj->obj_flags.cost;
    value0 = eq_max_damage(obj);
    eqdam = eq_current_damage(obj);

    if (eqdam == 0) {
      act("The Super Repair Guy says 'Looks fine to me.'",ch,0,0, TO_CHAR, 0);
      act("The Super Repair Guy says 'Looks fine to me.'",ch,0,0, TO_ROOM, 0);
      act("The Super Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
      act("The Super Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
      return eSUCCESS;
    }
    if (eqdam <= 0)
      percent = 0;
    else
      percent = ((100* eqdam) / value0);

    x = (100 - percent);    /* now we know what percent to repair ..  */
    price = ((cost * x) / 100);   /* now we know what to charge them fuckers! */
    price *= 2;             /* he likes to charge more..  */
                           /*  for armor... cuz he can.. */

    if (IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
      price *= 2;

    if (price < 1000)
      price = 1000;     /* Welp.. Repair Guy needs to feed the kids somehow.. :) */
  
    if (GET_GOLD(ch) < (uint32)price) {
      act("The Super Repair Guy says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_CHAR, 0);
      act("The Super Repair Guy says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_ROOM, 0);
      sprintf(buf, "The Super Repair Guy says 'It would cost %d coins to repair %s, which you don't have!'",
          price, obj->short_description);
      act(buf, ch,0,0, TO_CHAR, 0);
      act(buf, ch,0,0, TO_ROOM, 0);
      act("The Super Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
      act("The Super Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
      return eSUCCESS;
    } else {
      if (cmd == 65) {
        sprintf(buf, "The Super Repair Guy says 'It will only cost you %d coins to repair %s.'",
          price, obj->short_description);
        act(buf, ch,0,0, TO_CHAR, 0);
        act(buf, ch,0,0, TO_ROOM, 0);
        act("The Super Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
        act("The Super Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, 0);
        return eSUCCESS;
      }   /* price quote */


      GET_GOLD(ch) -= price;
      eq_remove_damage(obj);
      sprintf(buf, "The Super Repair Guy says 'It will cost you %d coins to repair %s.'",
          price, obj->short_description);
      act(buf, ch,0,0, TO_CHAR, 0);
      act(buf, ch,0,0, TO_ROOM, 0);
      act("You watch The Super Repair Guy fix $p...\n\r",ch,obj,0, TO_CHAR, 0);
      act("You watch The Super Repair Guy fix $p...\n\r",ch,obj,0,TO_ROOM,INVIS_NULL);
      act("The Super Repair Guy says 'All fixed!'",ch,obj,0, TO_CHAR, 0);
      act("The Super Repair Guy says 'All fixed!'",ch,obj,0, TO_ROOM, 0);
      act("The Super Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
      act("The Super Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
      return eSUCCESS;
    }
  } else if (obj->obj_flags.type_flag == ITEM_WEAPON) {

    cost = obj->obj_flags.cost;
    value0 = eq_max_damage(obj);
    eqdam = eq_current_damage(obj);

    if (eqdam == 0) {
      act("The Super Repair Guy says 'Looks fine to me.'",ch,0,0, TO_CHAR, 0);
      act("The Super Repair Guy says 'Looks fine to me.'",ch,0,0, TO_ROOM, 0);
      act("The Super Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
      act("The Super Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
      return eSUCCESS;
    }
    if (eqdam <= 0)
      percent = 0;
    else
      percent = ((100* eqdam) / (value0 + value2));

    x = (100 - percent);    /* now we know what percent to repair ..  */
    price = ((cost * x) / 100);   /* now we know what to charge them fuckers! */

    if (price < 1000)
      price = 1000;     /* Welp.. Repair Guy needs to feed the kids somehow.. :) */
  
    if (GET_GOLD(ch) < (uint32)price) {
      act("The Super Repair Guy says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_CHAR, 0);
      act("The Super Repair Guy says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_ROOM, 0);
      sprintf(buf, "The Super Repair Guy says 'It would cost %d coins to repair %s, which you don't have!'",
          price, obj->short_description);
      act(buf, ch,0,0, TO_CHAR, 0);
      act(buf, ch,0,0, TO_ROOM, 0);
      act("The Super Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
      act("The Super Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
      return eSUCCESS;

    } else {

      GET_GOLD(ch) -= price;
      eq_remove_damage(obj);

      sprintf(buf, "The Super Repair Guy says 'It will cost you %d coins to repair %s.'",
          price, obj->short_description);
      act(buf, ch,0,0, TO_CHAR, 0);
      act(buf, ch,0,0, TO_ROOM, 0);
      act("You watch The Super Repair Guy fix $p...\n\r",ch,obj,0, TO_CHAR, 0);
      act("You watch The Super Repair Guy fix $p...\n\r",ch,obj,0, TO_ROOM, 0);
      act("The Super Repair Guy says 'All fixed!'",ch,obj,0, TO_CHAR, 0);
      act("The Super Repair Guy says 'All fixed!'",ch,obj,0, TO_ROOM, 0);
      act("The Super Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
      act("The Super Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
      return eSUCCESS;
    }
  }
  act("The Super Repair Guy says 'I can't repair this.'",ch,0,0, TO_CHAR, 0);
  act("The Super Repair Guy says 'I can't repair this.'",ch,0,0, TO_ROOM, 0);
  act("The Super Repair Guy gives you $p.", ch,obj,0, TO_CHAR, 0);
  act("The Super Repair Guy gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
  return eSUCCESS;
}


int repair_shop(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char buf[256], item[256];
  int value0, value2, cost, price, x;
  int percent, eqdam;

    if ((cmd != 66) && (cmd != 65)) return eFAILURE;

       one_argument(arg, item);

      if (!*item) {
send_to_char("What item?\n\r", ch);
        return eSUCCESS;
         }

      obj = get_obj_in_list_vis(ch, item, ch->carrying);

     if (obj == NULL) {
send_to_char("You don't have that item.\n\r", ch);
       return eSUCCESS;
        }

act("You give Fingers $p.",ch,obj,0,TO_CHAR, 0);
act("$n gives $p to Fingers.", ch,obj,0, TO_ROOM, INVIS_NULL);
act("\n\rFingers examines $p...",ch,obj,0, TO_CHAR, 0);
act("\n\rFingers examines $p...",ch,obj,0, TO_ROOM, 0);

  if (obj->obj_flags.type_flag == ITEM_ARMOR) {

      cost = obj->obj_flags.cost;
    value0 = eq_max_damage(obj);
    eqdam = eq_current_damage(obj);

    if (eqdam == 0) {
act("Fingers says 'Looks fine to me.'",ch,0,0, TO_CHAR, 0);
act("Fingers says 'Looks fine to me.'",ch,0,0, TO_ROOM, 0);
act("Fingers gives you $p.", ch,obj,0, TO_CHAR, 0);
act("Fingers gives $n $p.", ch,obj,0, TO_ROOM, 0);
    return eSUCCESS;
      }
if (eqdam <= 0)
     percent = 0;
   else
     percent = ((100* eqdam) / value0);

      x = (100 - percent);    /* now we know what percent to repair ..  */
  price = ((cost * x) / 100);   /* now we know what to charge them fuckers! */
  price *= 4;             /* he likes to charge more..  */
                         /*  for armor... cuz he's a crook..  */

 if (IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
    price *= 2;

 if (price < 5000)
   price = 5000;     /* Welp.. Repair Guy needs to feed the kids somehow.. :) */
  
    if (GET_GOLD(ch) < (uint32)price) {
act("Fingers says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_CHAR, 0);
act("Fingers says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_ROOM, 0);
sprintf(buf, "Fingers says 'It would cost %d coins to repair %s, which you don't have!'",
          price, obj->short_description);
act(buf, ch,0,0, TO_CHAR, 0);
act(buf, ch,0,0, TO_ROOM, 0);
act("Fingers gives you $p.", ch,obj,0, TO_CHAR, 0);
act("Fingers gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
    return eSUCCESS;

     } else {
   if (cmd == 65) {
sprintf(buf, "Fingers says 'It will only cost you %d coins to repair %s.'",
          price, obj->short_description);
act(buf, ch,0,0, TO_CHAR, 0);
act(buf, ch,0,0, TO_ROOM, 0);
act("Fingers gives you $p.", ch,obj,0, TO_CHAR, 0);
act("Fingers gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
     }   /* price quote */


  GET_GOLD(ch) -= price;
  eq_remove_damage(obj);
sprintf(buf, "Fingers says 'It will cost you %d coins to repair %s.'",
          price, obj->short_description);
act(buf, ch,0,0, TO_CHAR, 0);
act(buf, ch,0,0, TO_ROOM, 0);
act("You watch Fingers fix $p...\n\r",ch,obj,0, TO_CHAR, 0);
act("You watch Fingers fix $p...\n\r",ch,obj,0, TO_ROOM, 0);
act("Fingers says 'All fixed!'",ch,obj,0, TO_CHAR, 0);
act("Fingers says 'All fixed!'",ch,obj,0, TO_ROOM, 0);
act("Fingers gives you $p.", ch,obj,0, TO_CHAR, 0);
act("Fingers gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
   }
  } else if (obj->obj_flags.type_flag == ITEM_WEAPON) {

      cost = obj->obj_flags.cost;
    value0 = eq_max_damage(obj);
    eqdam = eq_current_damage(obj);

    if (eqdam == 0) {
act("Fingers says 'Looks fine to me.'",ch,0,0, TO_CHAR, 0);
act("Fingers says 'Looks fine to me.'",ch,0,0, TO_ROOM, 0);
act("Fingers gives you $p.", ch,obj,0, TO_CHAR, 0);
act("Fingers gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
      }
if (eqdam <= 0)
     percent = 0;
   else
     percent = ((100* eqdam) / (value0 + value2));

      x = (100 - percent);    /* now we know what percent to repair ..  */
  price = ((cost * x) / 100);   /* now we know what to charge them fuckers! */
  price *= 3;

 if (price < 5000)
   price = 5000;     /* Welp.. Repair Guy needs to feed the kids somehow.. :) */
  
    if (GET_GOLD(ch) < (uint32)price) {
act("Fingers says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_CHAR, 0);
act("Fingers says 'Trying to sucker me for a free repair job?'",ch,0,0, TO_ROOM, 0);
sprintf(buf, "Fingers says 'It would cost %d coins to repair %s, which you don't have!'",
          price, obj->short_description);
act(buf, ch,0,0, TO_CHAR, 0);
act(buf, ch,0,0, TO_ROOM, 0);
act("Fingers gives you $p.", ch,obj,0, TO_CHAR, 0);
act("Fingers gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
    return eSUCCESS;

     } else {

  GET_GOLD(ch) -= price;
  eq_remove_damage(obj);

sprintf(buf, "Fingers says 'It will cost you %d coins to repair %s.'",
          price, obj->short_description);
act(buf, ch,0,0, TO_CHAR, 0);
act(buf, ch,0,0, TO_ROOM, 0);
act("You watch Fingers fix $p...\n\r",ch,obj,0, TO_CHAR, 0);
act("You watch Fingers fix $p...\n\r",ch,obj,0, TO_ROOM, 0);
act("Fingers says 'All fixed!'",ch,obj,0, TO_CHAR, 0);
act("Fingers says 'All fixed!'",ch,obj,0, TO_ROOM, 0);
act("Fingers gives you $p.", ch,obj,0, TO_CHAR, 0);
act("Fingers gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
     return eSUCCESS;
     }
   }
act("Fingers says 'I can't repair this.'",ch,0,0, TO_CHAR, 0);
act("Fingers says 'I can't repair this.'",ch,0,0, TO_ROOM, 0);
act("Fingers gives you $p.", ch,obj,0, TO_CHAR, 0);
act("Fingers gives $n $p.", ch,obj,0, TO_ROOM, INVIS_NULL);
   return eSUCCESS;
}

int mortician(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  struct obj_data *obj2;
  int x = 0, cost = 0, which;
  char buf[100];
  bool has_consent = FALSE;

  if(cmd != 56 && cmd != 59 && cmd != 58)
    return eFAILURE;

  if(cmd == 59) {  /* list */ 
    sprintf(buf, "%s_consent", GET_NAME(ch));
    send_to_char("Available corpses (freshest first):\n\r", ch);
    for(obj = object_list; obj; obj = obj->next) {
       if(!isname("pc", obj->name)     ||
          !isname("corpse", obj->name) ||
          (!isname(GET_NAME(ch), obj->name) || !isname(buf, obj->name)))
         continue; 
       cost = 0;
       for(obj2 = obj->contains; obj2; obj2 = obj2->next_content) {
          if(obj2->obj_flags.type_flag == ITEM_MONEY)
            continue;
          cost += obj2->obj_flags.cost; 
       } 
       cost /= 20000;
       cost = MAX(cost, 30); 
       sprintf(buf, "%d) %-21s %d Platinum coins.\n\r", ++x, obj->name,
               cost);
       send_to_char(buf, ch); 
    }
    send_to_char(
"If any corpses were listed, they are still where you left them.  This\n\r"
"list is therefore always changing.  If you purchase one, it will be\n\r"
"transferred to your inventory instantly if it is your corpse.  If it is\n\r"
"someone else's corpse you have consent for, it will be placed at your feet.\n\r"
"Use \"buy <number>\" to purchase a corpse.\n\r", ch);
    return eSUCCESS;
  }

  if(cmd == 58) // value
  {
    for(obj2 = ch->carrying; obj2; obj2 = obj2->next_content) {
      if(obj2->obj_flags.type_flag == ITEM_MONEY)
        continue;
      cost += obj2->obj_flags.cost;
    }
    for(x = 0; x <= WEAR_MAX; x++) {
      if(ch->equipment[x]) {
        cost += ch->equipment[x]->obj_flags.cost;
      }
    }

    cost /= 20000;
    cost = MAX(cost, 30);
    csendf(ch, "The Undertaker takes a look at you and estimates your corpse would cost around %d coins.\n\r", cost);
    return eSUCCESS;
  }

  /* buy */
  if((which = atoi(arg)) == 0) { 
    send_to_char("Try \"buy <number>\", or \"list\" for a list of "
                 "available corpses.\n\r", ch);
    return eSUCCESS;
  }

  for(obj = object_list; obj; obj = obj->next) {
     sprintf(buf, "%s_consent", GET_NAME(ch));

     if(obj->obj_flags.type_flag != ITEM_CONTAINER ||
        !isname("pc", obj->name)         ||
        !isname("corpse", obj->name)     ||
        (!isname(GET_NAME(ch), obj->name) || !isname(buf, obj->name)) ||   // name, or consented name
        ++x < which)
       continue;

     if(isname(buf, obj->name))
       has_consent = TRUE;

     for(obj2 = obj->contains; obj2; obj2 = obj2->next_content) {
        if(obj2->obj_flags.type_flag == ITEM_MONEY)
          continue;
        cost += obj2->obj_flags.cost;
     }
     cost /= 20000;
     cost = MAX(cost, 30);
     if(GET_PLATINUM(ch) < (uint32)cost) {
       send_to_char("You can't afford that!\n\r", ch);
       return eSUCCESS;
     } 
     obj->obj_flags.timer = 1;
     if(has_consent) {
       move_obj(obj, ch->in_room);
       send_to_char("The mortician goes into his freezer and returns with a corpse, which he\n\r"
                    "places at your feet.\n\r", ch);
     } else {
       move_obj(obj, ch);
       send_to_char("The mortician goes into his freezer and returns with a corpse, which he\n\r"
                    "places in your arms.\n\r", ch);
     }
     GET_PLATINUM(ch) -= cost;
     do_save(ch, "", 10);
     return eSUCCESS; 
  }
  
  send_to_char("No such corpse was found.  Try \"list\".\n\r", ch);
  return eSUCCESS; 
}


struct platinumsmith_data {
  char *name;
  char *attributes;
  int  price;
  int  vnum;
};

// TODO - rewrite this more modular, so we can spread lots of plat guys all around
// the world that sell different pieces of godload
int platinumsmith(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  int x = 0, y;
  struct obj_data *new_new_obj;
  char buf[200];
  char buf2[200];
  
  struct platinumsmith_data for_sale[] = {
   { "an Arch-Angel's Staff", "5d8 +6+6 20-harm 20-dispel evil", 2500, 10008 },
   { "A Flickering Poinard", "5d7 +7+5 10-colour spray 30-sparks", 3000, 10009 },
   { "Shartis Spiritbane", "6d8 +7+7 35-lightning bolt 10-howl", 2300, 10010 },
   { "the Prankquean's Blade", "4d11 +5+7 15-lightning 10-dispel", 2800, 10011 },
   { "the Un-sword", "10d5, +7+7 100-invis 100-protection from evil", 2500, 10012 },  
   { "the Rune-Sword", "6d8 +8+8 15-souldrain 2-heal spray twohanded", 3000, 10013 },
   { "a Glass Dagger", "3d14 +5+5 8-dispel 5-acid blast", 3500, 10014 },
   { "the Flaming Whip", "4d9 +7+7 15-fireball 15-flamestrike 5-lightning", 2500, 10015 },
   { "the Blade of Mirrors", "5d7 +7+7 5-reflect 100-camoflague", 3000, 10016 },
   { "Blah Blah", "burp", -1, -1 } 
  };
 

  if(cmd < 56 || cmd > 59) 
    return eFAILURE;

  if(cmd == 59) {
    send_to_char("The Platinumsmith tells you 'All prices are in Platinum.'"
                 "\n\r", ch);
    while(for_sale[x].price != -1) {
      sprintf(buf, "%d) %s  %s %d Coins.\n\r", x+1, for_sale[x].name,
              for_sale[x].attributes, for_sale[x].price);
      send_to_char(buf, ch);
      ++x;
    }
    return eSUCCESS;
  }


  /* value */
  if(cmd == 58) {
    one_argument(arg, buf);
    if(!*buf) {
      send_to_char("What do you want the value of?\n\r", ch);
      return eSUCCESS;
    }
    if(!(new_new_obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
      send_to_char("You don't seem to be holding any such object...\n\r", ch);
      return eSUCCESS;
    }
    for(x = 0; for_sale[x].price != -1;x++)
       if(obj_index[new_new_obj->item_number].virt == for_sale[x].vnum)
         break; 

    if(for_sale[x].price == -1) {
      send_to_char("The Platinumsmith tells you, 'I don't deal with that kind"
                   " of trashy eq.'\n\r", ch);
      return eSUCCESS;
    }

    csendf(ch, "The Platinumsmith tells you, 'I will pay you %d platinum "
           "coins for that.'\n\r",
          (int)((float)(for_sale[x].price) * value_multiplier(new_new_obj)));
    return eSUCCESS;
  }

  /* sell */
  if(cmd == 57) {
    one_argument(arg, buf);
    if(!*buf) {
      send_to_char("What do you want to sell?\n\r", ch);
      return eSUCCESS;
    }
    if(!(new_new_obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
      send_to_char("You don't seem to be holding any such object...\n\r", ch);
      return eSUCCESS;
    }
    for(x = 0; for_sale[x].price != -1; x++)
       if(obj_index[new_new_obj->item_number].virt == for_sale[x].vnum)
         break;

    if(for_sale[x].price == -1) {
      send_to_char("The Platinumsmith tells you, 'I don't deal with that kind"
                   " of trashy eq.'\n\r", ch);
      return eSUCCESS;
    }

    if(!isname(GET_NAME(ch), new_new_obj->name)) {
      sprintf(buf2, "%s tried to sell: %s", GET_NAME(ch), new_new_obj->name);
      send_to_char("The Platinumsmith tells you, 'Selling someone "
                   "elses eq?'\n\r", ch);
      log(buf2, OVERSEER, LOG_MORTAL);
      return eSUCCESS; 
    }

    // weapon damage values are kept in value0
    if(new_new_obj->obj_flags.value[0] != 0) {
      send_to_char("The Platinumsmith tells you, 'I don't buy broken shit.'\r\n", ch);
      return eSUCCESS;
    }

    y = (int)((float)(for_sale[x].price) * value_multiplier(new_new_obj));

    if(y < 0) {
      sprintf (buf2, "The Platinumsmith tells you. 'Whoa, it's worth %i."
                    " Tell Sadus about it so he can fix it'\n\r", y);
      send_to_char (buf2, ch);
      return eSUCCESS;
    }

    send_to_char("The Platinumsmith tells you, 'Here you go.'\n\r", ch);
    GET_PLATINUM(ch) += y; 
    extract_obj(new_new_obj);
    do_save(ch, "", 666);
    return eSUCCESS;
  }

  /* buy */
  if((x = atoi(arg)) == 0 || x < 0) {
    send_to_char("The Platinumsmith tells you 'Pick a number...'\n\r", ch);
    return eSUCCESS;
  }
  else if(x > 9) {  /* highest number you can buy */
    send_to_char("The Platinumsmith tells you 'Which one is THAT?'\n\r", ch);
    return eSUCCESS;
  } 
  x--; /* arrays start at 0 */

  if(GET_PLATINUM(ch) < (uint32)for_sale[x].price) {
    send_to_char("The Platinumsmith tells you 'You can't afford that!'\n\r",
                 ch);
    return eSUCCESS;
  }

  GET_PLATINUM(ch) -= for_sale[x].price;
  new_new_obj = clone_object(real_object(for_sale[x].vnum)); 
  sprintf(buf, "%s %s", new_new_obj->name, GET_NAME(ch));
  new_new_obj->name = str_hsh(buf);
  obj_to_char(new_new_obj, ch);
  send_to_char("Ok.\n\r", ch);
  do_save(ch, "", 666);
  return eSUCCESS; 
}




int platmerchant(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  int x = 0, y;
  struct obj_data *new_new_obj;
  char buf[MAX_STRING_LENGTH];
  char buf2[200];

  struct platinumsmith_data merchant_sale[] = {
   { "the Armbands of Durance", "+5+5 4-str 100-hps", 1800, 10036 },
   { "a band of starlight", "+6+6 100-freefloat 75-mana", 2300, 10050},
   { "Bands of Twisted Thorn", "+4+4 3-con 100-hps 100-staunchblood", 2000, 10037 },
   { "a Belt of Golden Pouches", "+4+4 3-str Carries 300 lbs", 4500, 10065 },
   { "Boartooth Studded Buskins", "7-dam 100-hps 100-move", 3000, 10032 },
   { "a Cat's Eye Ruby", "+4+4 100-move 100-sense life 2-dex", 2200, 10073 },
   { "a Cape of Living Serpents", "+4+4 100-hps 100-insomnia", 2000, 10058 },
   { "Chaplet of Flame", "+4+4 100-resist fire 35-mana 35-hps", 2000, 10017 },
   { "a Chain of Lightning", "+5+5 75-mana 20-all saves 100-resist electricity", 2000, 10046 },
   { "the Claws of the Dragon", "+4+6 3-wis 100-infra", 2100, 10063 },
   { "the Comet's Cloak", "100-move 100-P.F.E", 2300, 10056 },
   { "a Copper Buckler", "+7+7 3-str -40-armor", 2500, 10026 },
   { "Crown of Carnivals", "+5+5 100-resist electricity", 2500, 10020 },
   { "a Crown of Spring Blossoms", "+5+5 100-hps 50-mana -10-armor", 2300, 10018 },
   { "the Demonbane", "+3+3 Light Source 100-P.F.E. 100-hps -10-armor", 3500, 10068 },
   { "a Dragonbone Breastplate", "+5+5 100-hps", 2500, 10023 },
   { "a dragonscale bracer", "+6+6 100-hps", 2200, 10054 },
   { "the Dryad's Necklace", "+4+4 100-mana 50-hps 100-camouflage", 2000, 10047 },
   { "the Duke of Silence's Gloves", "+4+4 100-hps 3-wis 3-int", 2000, 10061 },
   { "Ebon Leggings", "+5+5 100-hps 100-sense life 100-detect invis", 2300, 10031 },
   { "an Ebon Tunic", "+5+5 50-mana -10-armor", 2400, 10022 },
   { "an Emerald-Studded Choker", "+5+5 100-hps 100-insomnia", 1900, 10045 },
   { "the Gitana's Bangle", "+5+5 100-mana 100-move 100-freefloat", 2000, 10053 },
   { "a Globe of Prismatic Light", "+4+4 Light Source", 2300, 10067 },
   { "the Godling's Girdle", "+6+6 3-str 100-sense life", 2800, 10066 },
   { "Godzilla's Bracer", "+7+7", 2200, 10055 },
   { "the Golem's Mask", "+5+5 100-hps -30-armor", 2000, 10040 },
   { "Greaves of Gilded Steel", "+6+6 2-con 75-hps", 2400, 10030 },
   { "The Helm of Battles", "+6+6 50-move 50-hps", 2500, 10019 },
   { "the Ki-Rin's Hooves", "+5+5 3-wis 100-move 30-all saves", 1800, 10035 },
   { "the Leggings of the Woodswalker", "+4+4 50-camouflage 100-mana 100-detect invis", 2200, 10029 },
   { "a loop of ivy", "+5+5 100-hps 100-staunchblood", 2000, 10051 },
   { "a loop of star-forged silver", "+3+3 5-ki -10-armor", 2800, 10070 },
   { "the Maharani's Teardrop", "+5+5 75-mana", 2000, 10069 },
   { "the Mason's Mystical Goggles", "+5+5 50-hps 100-mana 100-sense life", 2000, 10042 },
   { "a Mercurial Talisman", "+3+3 2-dex 5-reflect -20-armor", 2500, 10074 },
   { "the Mountain's Heart", "+5+5 75-hps -30-armor 100-stability", 2000, 10048 },
   { "a Necklace of Hollow-Eyed Skulls", "+4+4 5-reflect 100-hps 100-lightning shield", 2100, 10043 },
   { "the North Wind's Bracers", "+4+4 100-frostshield 100-resist fire", 2000, 10038 },
   { "the Poet's Crystal", "+4+4 100-freefloat 50-mana", 2300, 10072 },
   { "the Prankquean's Kiss", "+4+4 100-lightning shield 5-ki", 2800, 10075 },
   { "Razor Tipped Gloves", "+5+5 100-move 3-dex", 2000, 10060 },
   { "A Rose Embroidered Tabard", "+3+3 100-hps 10-ki 2-dex", 2400, 10021 },
   { "a sash embroidered with golden bees", "+5+5 3-wis 100-resist fire Carries 300 lbs", 5000, 10064 },
   { "the Seaqueen's Stolen Eyes", "+4+4 100-farsight 100-infra 100-detect invis", 2100, 10041 },
   { "the Shadow of the Wind", "100-shadowslip", 2100, 10057 },
   { "the Shield of the Stormlord", "+5+5 5-reflect 100-lightning shield -50-armor", 2500, 10027 },
   { "the Slippers of the Shadow Dancer", "+4+4 100-invis 5-reflect 100-stability", 2200, 10033 },
   { "a silver-etched black steel breastplate", "+5+5 100-lightning shield 30-all saves", 2200, 10025 },
   { "a sphere of solid light", "+5+5 75-hps 100-sense life", 2500, 10071 },
   { "the Sphere of the StoneKing", "+3+3 100-hps -25-armor", 2500, 10076},
   { "the Stardaughter's Ring", "+5+5 100-mana 3-wis 100-hps", 2100, 10049 },
   { "the Stoneking's Gauntlets", "+5+5 100-hps 3-str 3-con", 2300, 10062 },
   { "the Stormlord's Breeches", "+5+5 2-str 100-resist electricity -10-armor", 2000, 10028 },
   { "a String of Jackal's Teeth", "+5+5 100-mana -10-armor", 2000, 10044 },
   { "a Twist of Braided Mane", "+5+5 100-hps 100-move 20-all saves", 2000, 10052 },
   { "the Traveller's Rucksack", "+3+3 carries 300 lbs", 4000, 10059 },
   { "a Vest of Pale Leather", "+4+4 100-resist cold 40-hps", 2400, 10024 },
   { "the Windwalker's Boots", "+5+5 100-mana 100-freefloat", 2000, 10034},
   { "a Wisp of Starlight", "+3+3 5-ki 100-infra 100-detect invis", 2100, 10039 },
   { "Blah Blah", "burp", -1, -1 } 
  };
 

  if(cmd < 56 || cmd > 59) 
    return eFAILURE;

  if (cmd == 59) {
     if (ch->desc) {
        send_to_char("The Platinum Merchant tells you 'All prices are in Platinum.'"
                     "\n\r", ch);
        buf[0] = '\0';
        while(merchant_sale[x].price != -1) {
           sprintf(buf + strlen(buf), "%d) %s  %s %d Coins.\n\r",
                   x+1, merchant_sale[x].name,
                   merchant_sale[x].attributes, merchant_sale[x].price);
           ++x;
           }
        page_string(ch->desc, buf, 1);
        }
     return eSUCCESS;
     }


  /* value */
  if(cmd == 58) {
    one_argument(arg, buf);
    if(!*buf) {
      send_to_char("What do you want the value of?\n\r", ch);
      return eSUCCESS;
    }
    if(!(new_new_obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
      send_to_char("You don't seem to be holding any such object...\n\r", ch);
      return eSUCCESS;
    }
    for(x = 0; merchant_sale[x].price != -1;x++)
       if(obj_index[new_new_obj->item_number].virt == merchant_sale[x].vnum)
         break; 

    if(merchant_sale[x].price == -1) {
      send_to_char("The Platinum Merchant tells you, 'I don't deal with that kind"
                   " of trashy eq.'\n\r", ch);
      return eSUCCESS;
    }

    csendf(ch, "The Platinum Merchant tells you, 'I will pay you %d platinum "
           "coins for that.'\n\r",
          (int)((float)(merchant_sale[x].price) * value_multiplier(new_new_obj)));
    return eSUCCESS;
  }

  /* sell */
  if(cmd == 57) {
    one_argument(arg, buf);
    if(!*buf) {
      send_to_char("What do you want to sell?\n\r", ch);
      return eSUCCESS;
    }
    if(!(new_new_obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
      send_to_char("You don't seem to be holding any such object...\n\r", ch);
      return eSUCCESS;
    }
    for(x = 0; merchant_sale[x].price != -1; x++)
       if(obj_index[new_new_obj->item_number].virt == merchant_sale[x].vnum)
         break;

    if(merchant_sale[x].price == -1) {
      send_to_char("The Platinum Merchant tells you, 'I don't deal with that kind"
                   " of trashy eq.'\n\r", ch);
      return eSUCCESS;
    }

    if(!isname(GET_NAME(ch), new_new_obj->name)) {
      sprintf(buf2, "%s tried to sell: %s", GET_NAME(ch), new_new_obj->name);
      send_to_char("The Platinum Merchant tells you, 'Selling someone "
                   "elses eq?'\n\r", ch);
      log(buf2, OVERSEER, LOG_MORTAL);
      return eSUCCESS; 
    }

    // weapon damage values are kept in value1
    if(new_new_obj->obj_flags.value[1] != 0 && GET_ITEM_TYPE(new_new_obj)!=ITEM_CONTAINER) {
      send_to_char("The Platinum Merchant tells you, 'I don't buy broken shit.'\r\n", ch);
      return eSUCCESS;
    }

    y = (int)((float)(merchant_sale[x].price) * value_multiplier(new_new_obj));

    if(y < 0) {
      sprintf (buf2, "The Platinum Merchant tells you. 'Whoa, it's worth %i."
                    " Tell Sadus about it so he can fix it'\n\r", y);
      send_to_char (buf2, ch);
      return eSUCCESS;
    }

    send_to_char("The Platinum Merchant tells you, 'Here you go.'\n\r", ch);
    GET_PLATINUM(ch) += y; 
    extract_obj(new_new_obj);
    return eSUCCESS;
  }

  // buy
  if((x = atoi(arg)) == 0 || x < 0) {
    send_to_char("The Platinum Merchant tells you 'Pick a number...'\n\r", ch);
    return eSUCCESS;
  }
  else if(x > 60) {  // highest number you can buy
    send_to_char("The Platinum Merchant tells you 'Which one is THAT?'\n\r", ch);
    return eSUCCESS;
  } 
  x--; // arrays start at 0

  if(GET_PLATINUM(ch) < (uint32)merchant_sale[x].price) {
    send_to_char("The Platinum Merchant tells you 'You can't afford that!'\n\r",
                 ch);
    return eSUCCESS;
  }

  GET_PLATINUM(ch) -= merchant_sale[x].price;
  new_new_obj = clone_object(real_object(merchant_sale[x].vnum)); 
  sprintf(buf, "%s %s", new_new_obj->name, GET_NAME(ch));
  new_new_obj->name = str_hsh(buf);
  obj_to_char(new_new_obj, ch);
  send_to_char("Ok.\n\r", ch);
  return eSUCCESS; 
}


// TODO - set up meta to DC2 so the cost/exp goes up at a smoother rate
// and is a little more balanced depending on class
// TODO - make sure meta checks for get_stat_max to make sure you can't meta
// past your racial maximum

int meta_dude(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char buf[256];
  char argument[256];

  int stat;
  int choice;
  int increase;
  int hit_cost, mana_cost, move_cost, hit_exp, move_exp, mana_exp;
  int price, str_price, int_price, wis_price, dex_price, con_price;

  long gold;

  struct obj_data *new_new_obj;
  sbyte *pstat; 
  int *pprice;

  const double EXPCONV  = 400000000.0;
  const double GOLDCONV = 1000000.0;

  if((cmd != 59) && (cmd != 56))
    return eFAILURE;

  if (IS_AFFECTED(ch, AFF_BLIND))
    return eFAILURE;

  if(IS_NPC(ch))
    return eFAILURE;

  if(GET_LEVEL(ch) < 10) {
    send_to_char("The Meta-physician tells you, 'You're to low level for "
                 "me to waste my time on you.\n\r"
                 "Prove to me you are gonna stick around first!'.", ch);
    return eSUCCESS;
  }

  // TODO - redo these more class specific and how we're sure we want them

  hit_exp = (GET_MAX_HIT(ch) * 8000);
  move_exp = (GET_MAX_MOVE(ch) * 8000);
  mana_exp = (GET_MAX_MANA(ch) * 8000);

  if(hit_exp < 1) hit_exp = 1;
  if(move_exp < 1) move_exp = 1;
  if(mana_exp < 1) mana_exp = 1;

  hit_cost = 100 + GET_HP_METAS(ch);
  move_cost = 100 + GET_MOVE_METAS(ch);
  mana_cost = 100 + GET_MANA_METAS(ch); 

  stat = ch->raw_str;
  str_price = 1000000+((GET_LEVEL(ch)*4)*10000)+((stat*8)*30000);
  stat = ch->raw_intel;
  int_price = 1000000+((GET_LEVEL(ch)*4)*10000)+((stat*8)*30000);
  stat = ch->raw_wis;
  wis_price = 1000000+((GET_LEVEL(ch)*4)*10000)+((stat*8)*30000);
  stat = ch->raw_dex;
  dex_price = 1000000+((GET_LEVEL(ch)*4)*10000)+((stat*8)*30000);
  stat = ch->raw_con;
//  con_price = 1000000+((GET_LEVEL(ch)*4)*10000)+((stat*8)*30000);
  con_price = ((GET_LEVEL(ch)*4)*10000)+((stat*8)*30000);

  if(cmd == 59) {           /* List */
    send_to_char("The Meta-physician tells you, 'This is what I can do "
                 "for you... \n\r", ch);
/*
    stat = ch->raw_str;
    if(stat == 25)
      strcpy(buf, "1) Str:       Your strength is already 25.\n\r");
    else
      sprintf(buf, "1) Str: %d        Cost: %d exp + %d Platinum coins. \n\r",
              stat + 1, str_price, stat < 18 ? 100 : 500);
    send_to_char(buf, ch);

    stat = ch->raw_intel;
    if(stat == 25)
      strcpy(buf, "2) Int:       Your intelligence is already 25.\n\r");
    else
      sprintf(buf, "2) Int: %d        Cost: %d exp + %d Platinum coins.\n\r",
              stat+1, int_price, stat < 18 ? 100 : 500);
    send_to_char(buf, ch);

    stat = ch->raw_wis;
    if(stat == 25)
      strcpy(buf, "3) Wis:       Your wisdom is already 25.\n\r");
    else
      sprintf(buf, "3) Wis: %d        Cost: %d exp + %d Platinum coins.\n\r",
              stat+1, wis_price, stat < 18 ? 100 : 500);
    send_to_char(buf, ch);

    stat = ch->raw_dex;
    if(stat == 25)
      strcpy(buf, "4) Dex:       Your dexterity is already 25.\n\r");
    else
      sprintf(buf, "4) Dex: %d        Cost: %d exp + %d Platinum coins.\n\r",
              stat+1, dex_price, stat < 18 ? 100 : 500);
    send_to_char(buf, ch);
*/
    stat = ch->raw_con;
    if(stat >= 16)
      strcpy(buf, "5) Con:       Your constitution is already 16+.\n\r");
    else
      sprintf(buf, "5) Con: %d        Cost: %d exp + %d Platinum coins.\n\r",
              stat+1, con_price, stat < 16 ? 100 : 500);
    send_to_char(buf, ch);
/*    
    csendf(ch, "6) Add to your hit points:   %d exp + %d "
            "Platinum coins.\n\r", hit_exp, hit_cost); 

    if(GET_MAX_HIT(ch) > 29000)
       csendf(ch, "   Note:  You've hit the Tyrre tax level.\n\r");
    
    csendf(ch, "7) Add to your mana points:  %d exp + %d "
            "Platinum coins.\n\r", mana_exp, mana_cost);
    
    csendf(ch, "8) Add to your movement points: %d exp + %d "
            "Platinum coins.\n\r", move_exp, move_cost);
*/    
    send_to_char(
//    "9) Freedom from HUNGER and THIRST:  100000000 exp + 2000 Platinum coins.\n\r"
//    "10) Deep Red Vial:     10 Platinum coins.\n\r"
//    "11) Deep Blue Potion:   5 Platinum coins.\n\r"
    "12) Five (5) Platinum coins   Cost: 100,000 Gold Coins.\n\r"
//    "13) 100,000 Gold Coins        Cost: 5 Platinum Coins.\n\r"
//    "14) One (1) Platinum coin     Cost: 20,000 Gold Coins.\n\r"
//    "15) 20,000 Gold Coins         Cost: 1 Platinum Coin.\n\r"
//    "16) Convert your experience points to gold coins.\n\r"
    "17) Buy a practice session for 50 plats.\n\r"
                 , ch);
/*
    csendf(ch, "18) Add 1 lonely hit point:      %d exp + %d "
            "Platinum coins.\n\r", hit_exp/5, hit_cost/5); 
*/    
    return eSUCCESS;
  }

  if(cmd == 56)   {      /* buy  */
    one_argument(arg, argument);
    if((choice = atoi(argument)) == 0 || choice < 0) {
      send_to_char("The Meta-physician tells you, 'Pick a number.'\n\r", ch);
      return eSUCCESS;
    }
    switch(choice) {
      case 1: stat = ch->raw_str;
             pstat = &(ch->raw_str); pprice = &str_price; break;
      case 2: stat = ch->raw_intel;
             pstat = &(ch->raw_intel); pprice = &int_price; break;
      case 3: stat = ch->raw_wis;
             pstat = &(ch->raw_wis); pprice = &wis_price; break;
      case 4: stat = ch->raw_dex;
             pstat = &(ch->raw_dex); pprice = &dex_price; break;
      case 5: stat = ch->raw_con;
             pstat = &(ch->raw_con); pprice = &con_price; break;
      default: stat = 0;
    }

//    if(choice < 6) {
    if(choice == 5) {
      if(GET_PLATINUM(ch) < 100 || 
         (*pstat >= 18 && GET_PLATINUM(ch) < 500)) {
        send_to_char("The Meta-physician tells you, 'You can't afford "
                     "my services.  FUCK OFF!\n\r", ch);
        return eSUCCESS;
      }
      if(stat >= 16) {
        send_to_char("The Meta-physician tells you, 'I can't help you "
                     "anymore with that!'\n\r", ch); 
        return eSUCCESS;
      }

      if(GET_EXP(ch) < *pprice) {
        send_to_char("The Meta-physician tells you, 'You "
                     "lack the experience.'\n\r", ch);
        return eSUCCESS;
      }
  
      GET_EXP(ch) -= *pprice;
      GET_PLATINUM(ch) -= (*pstat < 18 ? 100 : 500);

      *pstat += 1;

      act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
      act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

      affect_total(ch);
      redo_hitpoints(ch);
      redo_mana(ch);
      return eSUCCESS;
    }
/*
   if(choice == 6) {
     if(GET_EXP(ch) < hit_exp) {
       send_to_char("The Meta-physician tells you, 'You lack the "
                    "experience.'\n\r", ch);
       return eSUCCESS;
     }

     if(GET_PLATINUM(ch) < (uint32)hit_cost) {
       send_to_char("The Meta-physician tells you, 'You can't "
                    "afford my services!  SCRAM!'\n\r", ch);
       return eSUCCESS;
     }
     GET_EXP(ch) -= hit_exp;
     GET_PLATINUM(ch) -= hit_cost;

     switch(GET_CLASS(ch)) {
       case CLASS_MAGIC_USER:
       case CLASS_CLERIC:
       case CLASS_DRUID:
         increase = 1;
         break;
       default:
         increase = 2;
     }

     ch->raw_hit += increase;
     GET_HP_METAS(ch) += 1;

     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

     affect_total(ch);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 7) {

     if(GET_EXP(ch) < mana_exp) {
       send_to_char("The Meta-physician tells you, 'You lack the "
                    "experience.'\n\r", ch);
       return eSUCCESS;
     }

     if(GET_PLATINUM(ch) < (uint32)mana_cost) {
       send_to_char("The Meta-physician tells you, 'You can't afford my "
                    "services!  SCRAM!'\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch) -= mana_exp;
     GET_PLATINUM(ch) -= mana_cost;

     switch(GET_CLASS(ch)) {
       case CLASS_MAGIC_USER:
       case CLASS_CLERIC:
       case CLASS_DRUID:
         increase = 2;
         break;
       default:
         increase = 1;
     }

     ch->raw_mana += increase;
     GET_MANA_METAS(ch) += 1;

     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

     affect_total(ch);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 8) {

     if(GET_EXP(ch) < move_exp) {
       send_to_char("The Meta-physician tells you, 'You lack the "
                    "experience.'\n\r", ch);
       return eSUCCESS;
     }

     if(GET_PLATINUM(ch) < (uint32)move_cost) {
       send_to_char("The Meta-physician tells you, 'You can't " 
                    "afford my services!  SCRAM!'\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch)  -= move_exp;
     GET_PLATINUM(ch) -= move_cost;

     ch->raw_move += 3;
     GET_MOVE_METAS(ch) += 1;

     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

     affect_total(ch);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 9) {
     price = 100000000;
     if(GET_COND(ch, FULL) == -1) {
       send_to_char("The Meta-physician tells you, 'You already have "
                    "freedom from hunger and thirst!\n\r", ch);
       return eSUCCESS;
     }

     if(GET_EXP(ch) < price) {
       send_to_char("The Meta-physician tells you, 'You lack the "
                    "experience.'\n\r", ch);
       return eSUCCESS;
     }

     if(GET_PLATINUM(ch) < 2000) {
       send_to_char("The Meta-physician tells you, 'You can't "
                    "afford my services!  SCRAM!'\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch) -= price;
     GET_PLATINUM(ch) -= 2000;

     GET_COND(ch,THIRST) = -1;
     GET_COND(ch, FULL) = -1;

     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

     affect_total(ch);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 10) {

     send_to_char("The Meta-physician tells you, 'I don't offer that right now.'\n\r", ch);
     return eSUCCESS;

     if(GET_PLATINUM(ch) < 10) {
       send_to_char("The Meta-physician tells you, 'You can't afford "
                    "that!'\n\r", ch);
       return eSUCCESS;
     }
     GET_PLATINUM(ch) -= 10;
     new_new_obj = clone_object(real_object(10004));
     obj_to_char(new_new_obj, ch);
     send_to_char("Ok.\n\r", ch);
     return eSUCCESS;
   }
   if(choice == 11) {

     send_to_char("The Meta-physician tells you, 'I don't offer that right now.'\n\r", ch);
     return eSUCCESS;

     if(GET_PLATINUM(ch) < 5) {
       send_to_char("The Meta-physician tells you, 'You can't afford "
                    "that!'\n\r", ch);
       return eSUCCESS;
     }
     GET_PLATINUM(ch) -= 5;
     new_new_obj = clone_object(real_object(10003));
     obj_to_char(new_new_obj, ch);
     send_to_char("Ok.\n\r", ch);
     return eSUCCESS;
   }
*/
   if(choice == 12) {
     if(GET_GOLD(ch) < 100000) {
       send_to_char("The Meta-physician tells you, 'You can't afford "
                    "that!'\n\r", ch);
       return eSUCCESS;
     }
     GET_GOLD(ch) -= 100000;
     GET_PLATINUM(ch) += 5;
     send_to_char("Ok.\n\r", ch);
     return eSUCCESS;
   }
/*
  if(choice == 13) {
    if(GET_PLATINUM(ch) < 5) {
      send_to_char("The Meta-physician tells you, 'You can't afford "
                   "that!'\n\r", ch);
      return eSUCCESS;
    }
    if(GET_GOLD(ch) > 2000000000) {
      send_to_char("The Meta-physician tells you, 'You got too much gold already dude.\r\n", ch);
      return eSUCCESS;
    }

    GET_PLATINUM(ch) -= 5;
    GET_GOLD(ch) += 100000;
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
  }

   if(choice == 14) {
     if(GET_GOLD(ch) < 20000) {
       send_to_char("The Meta-physician tells you, 'You can't afford "
                    "that!'\n\r", ch);
       return eSUCCESS;
     }
     GET_GOLD(ch) -= 20000;
     GET_PLATINUM(ch) += 1;
     send_to_char("Ok.\n\r", ch);
     return eSUCCESS;
   }
  if(choice == 15) {
    if(GET_PLATINUM(ch) < 1) {
      send_to_char("The Meta-physician tells you, 'You can't afford "
                   "that!'\n\r", ch);
      return eSUCCESS;
    }
    GET_PLATINUM(ch) -= 1;
    GET_GOLD(ch) += 20000;
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
  }

  if(choice == 16) {
    if (GET_EXP(ch) < EXPCONV) {
       send_to_char ("You need more experience before it can be "
                     "converted!\n\r", ch);
       return eSUCCESS;
    }
    
    gold = (long)((GET_EXP(ch) / EXPCONV) * GOLDCONV);
     
    csendf(ch, "The Meta-Physician converted your %d experience into "
           "%ld gold!\n\r", GET_EXP(ch), gold);
    
    logf(IMMORTAL, LOG_GOD, "%s converts %d exp to %ld gold",
         GET_SHORT(ch), GET_EXP(ch), gold);
     
    GET_GOLD(ch) = GET_GOLD(ch) + gold;
    GET_EXP(ch) = 0;
    return eSUCCESS;
  }
*/
  if(choice == 17) {
    if (GET_PLATINUM(ch) < 50) {
       send_to_char ("Costs 50 plats...which you don't have.\n\r", ch);
       return eSUCCESS;
    }
    if (IS_MOB(ch)) {
       send_to_char ("You can't buy practices chode...\r\n", ch);
       return eSUCCESS;
    }
    send_to_char("The Meta-Physician gives you a practice session.\n\r", ch);
    
    GET_PLATINUM(ch) -= 50;
    ch->pcdata->practices += 1;
    return eSUCCESS;
  }
/*
   if(choice == 18) {
     if(GET_EXP(ch) < hit_exp/5) {
       send_to_char("The Meta-physician tells you, 'You lack the "
                    "experience.'\n\r", ch);
       return eSUCCESS;
     }

     if(GET_PLATINUM(ch) < (uint32)(hit_cost/5)) {
       send_to_char("The Meta-physician tells you, 'You can't "
                    "afford my services!  SCRAM!'\n\r", ch);
       return eSUCCESS;
     }
     GET_EXP(ch) -= hit_exp/5;
     GET_PLATINUM(ch) -= hit_cost/5;

     ch->raw_hit += 1;

     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

     affect_total(ch);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }
*/
 }
  send_to_char("The Meta-physician tells you, 'Buy what?!'\n\r", ch);
  return eSUCCESS;
}

