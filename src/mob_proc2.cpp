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
/* $Id: mob_proc2.cpp,v 1.74 2006/12/30 19:19:03 dcastle Exp $ */
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
#include <spells.h>

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif


/*   external vars  */

extern CWorld world;
extern struct obj_data * search_char_for_item(char_data * ch, int16 
item_number, bool wearonly = FALSE);
 
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
 extern int class_restricted(char_data *ch, struct obj_data *obj);
 extern int size_restricted(char_data *ch, struct obj_data *obj);


/* extern procedures */

void save_corpses(void);
void hit(struct char_data *ch, struct char_data *victim, int type);
void gain_exp(struct char_data *ch, int gain);

long long  new_meta_platinum_cost(int start, int end)
{ // This is the laziest function ever. I didn't feel like
  // figuring out a formulae to work with the ranges, so I didn't.
  long long platcost = 0;
  if (end <= start || end < 0 || start < 0) return 0; // That's cheap!
  while (start < end)
  {
    if (start < 1000) platcost += 100 + (start / 100);
    else if (start < 1250) platcost += 110 + (start / 30);
    else if (start < 1500) platcost += 150 + (start / 30);
    else if (start < 1750) platcost += 200 + (start / 35);
    else if (start < 2000) platcost += 250 + (start / 40);
    else if (start < 3000) platcost += 300 + (start / 15);
    else if (start < 4000) platcost += 500 + (start / 20);
    else if (start < 5000) platcost += 700 + (start / 25);
    else if (start < 6000) platcost += 900 + (start / 20);
    else platcost += (1200 + (start / 20)) > 1500 ? 1500: (1200 + (start / 20));
    start += 5;
  }
  return platcost;
}

int r_new_meta_platinum_cost(int start, long long plats)
{ // This is a copy of the laziest function ever. I didn't feel like
  // figuring out a formulae to work with the ranges, so I didn't.
  long long platcost = 0;
  if (plats <= 0 || start < 0) return 0;
  while (platcost < plats)
  {
    if (start < 1000) platcost += 100 + (start / 100);
    else if (start < 1250) platcost += 110 + (start / 30);
    else if (start < 1500) platcost += 150 + (start / 30);
    else if (start < 1750) platcost += 200 + (start / 35);
    else if (start < 2000) platcost += 250 + (start / 40);
    else if (start < 3000) platcost += 300 + (start / 15);
    else if (start < 4000) platcost += 500 + (start / 20);
    else if (start < 5000) platcost += 700 + (start / 25);
    else if (start < 6000) platcost += 900 + (start / 20);
    else platcost += (1200 + (start / 20)) > 1500 ? 1500: (1200 + (start / 20));
    start += 5;
  }
  return start-5;
}

int r_new_meta_exp_cost(int start, long long exp)
{
   if (exp <= 0) return start;
   while (exp > 0)
   {
     exp -= new_meta_platinum_cost(start, start+1) * 51523;
     start += 5;
   }
   return start-5;
}

int new_meta_exp_cost_one(int start)
{
   if (start < 0) return 0;
   return new_meta_platinum_cost(start, start+1) * 51523;
}


long long moves_exp_spent(char_data * ch)
{
   int start = GET_MAX_MOVE(ch) - GET_MOVE_METAS(ch);
   long long expcost = 0;
   while (start < GET_MAX_MOVE(ch))
   {
    expcost += (int)((5000000 + (start * 2500))*1.2);
    start++;
   }
   return expcost; 
}

long long moves_plats_spent(char_data * ch)
{
  long long expcost = 0;
  int start = GET_MAX_MOVE(ch) - GET_MOVE_METAS(ch);
  while (start < GET_MAX_MOVE(ch))
  {
    expcost += (long long)(((int)(125 + (int)((0.025 * start *(start/1000 == 0 ? 1: start/1000))))*0.9));
    start++;
  }
  return expcost;
}

long long hps_exp_spent(char_data * ch)
{
   long long expcost = 0;
   int cost;
   switch (GET_CLASS(ch))
   {
      case CLASS_BARBARIAN: cost = 2000; break;
      case CLASS_WARRIOR: cost = 2100; break;
      case CLASS_PALADIN: cost = 2200; break;
      case CLASS_MONK: cost = 2300; break;
      case CLASS_RANGER: cost = 2500; break;
      case CLASS_ANTI_PAL: cost = 2500; break;
      case CLASS_THIEF: cost = 2600; break;
      case CLASS_BARD: cost = 2600; break;
      case CLASS_DRUID: cost = 2800; break;
      case CLASS_CLERIC: cost = 2900; break;
      case CLASS_MAGIC_USER: cost = 3000; break;
      default:
        cost = 3000; break;
   }
   int base = GET_MAX_HIT(ch) - GET_HP_METAS(ch);
   while (base < GET_MAX_HIT(ch))
   {
     expcost += (long long)((5000000 + (cost * base))*1.2);
     base++;
   }
   return expcost;
}

long long hps_plats_spent(char_data * ch)
{
   int cost;
   long long platcost = 0;
   switch (GET_CLASS(ch))
   {
      case CLASS_BARBARIAN: cost = 0; break;
      case CLASS_WARRIOR: cost = 10; break;
      case CLASS_PALADIN: cost = 20; break;
      case CLASS_MONK: cost = 30; break;
      case CLASS_RANGER: cost = 50; break;
      case CLASS_ANTI_PAL: cost = 50; break;
      case CLASS_THIEF: cost = 60; break;
      case CLASS_BARD: cost = 60; break;
      case CLASS_DRUID: cost = 80; break;
      case CLASS_CLERIC: cost = 90; break;
      case CLASS_MAGIC_USER: cost = 100; break;
      default:
        cost = 100; break;
   }
   int base = GET_MAX_HIT(ch) - GET_HP_METAS(ch);
   while (base < GET_MAX_HIT(ch))
   {
     platcost += (long long)((100 + cost + (int)(0.025 * base *(base/1000 == 0 ? 1: base/1000))) * 0.9);
     base++;
   }
   return platcost;
}

long long mana_exp_spent(char_data * ch)
{
   int cost;
   long long expcost = 0;
   switch (GET_CLASS(ch))
   {
      case CLASS_PALADIN: cost = 2800; break;
      case CLASS_RANGER: cost = 2500; break;
      case CLASS_ANTI_PAL: cost = 2500; break;
      case CLASS_DRUID: cost = 2200; break;
      case CLASS_CLERIC: cost = 2100; break;
      case CLASS_MAGIC_USER: cost = 2000; break;
      default:
        return 0;
   }
   int base = GET_MAX_MANA(ch) - GET_MANA_METAS(ch);
   while (base < GET_MAX_MANA(ch))
   {
     expcost += (long long)((5000000 + (cost * base))*1.2);
     base++;     
   }
   return expcost;
}


long long mana_plats_spent(char_data * ch)
{
   int cost;
   long long platcost = 0;
   switch (GET_CLASS(ch))
   {
      case CLASS_PALADIN: cost = 80; break;
      case CLASS_RANGER: cost = 50; break;
      case CLASS_ANTI_PAL: cost = 50; break;
      case CLASS_DRUID: cost = 20; break;
      case CLASS_CLERIC: cost = 10; break;
      case CLASS_MAGIC_USER: cost = 0; break;
      default:
        return 0;
   }
  int base = GET_MAX_MANA(ch) - GET_MANA_METAS(ch);
  while (base < GET_MAX_MANA(ch))
  {
    platcost += (long long)((100 + cost + (int)(0.025 * base * (base/1000 == 0 ? 1: base/1000)))*0.9);
    base++;
  }
  return platcost;
}



float value_multiplier(struct obj_data *obj)
{
  // this is to keep containers from losing $$ since they can't be damaged
  if(GET_ITEM_TYPE(obj) == ITEM_CONTAINER || GET_ITEM_TYPE(obj) == ITEM_LOCKPICK) {
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


void repair_shop_fix_eq(char_data * ch, char_data * owner, int price,
                        obj_data * obj)
{
  char buf[256];

  GET_GOLD(ch) -= price;
  eq_remove_damage(obj);
  sprintf(buf, "It will cost you %d coins to repair %s.", price, obj->short_description);
  do_say(owner, buf, 9);
  act("You watch $N fix $p...\n\r\n\r", ch, obj, owner, TO_CHAR, 0);
  act("You watch $N fix $p...\n\r\n\r", ch, obj, owner, TO_ROOM, 0);
  do_say(owner, "All fixed!", 9);
  act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
  act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
}

void repair_shop_complain_no_cash(char_data * ch, char_data * owner, int price,
                                  obj_data * obj)
{
  char buf[256];

  do_say(owner, "Trying to sucker me for a free repair job?", 9); 
  sprintf(buf, "It would cost %d coins to repair %s, which you don't have!",
          price, obj->short_description);
  do_say(owner, buf, 9);
  act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
  act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
}

void repair_shop_price_check(char_data * ch, char_data * owner, int price,
                             obj_data * obj)
{
  char buf[256];

  sprintf(buf, "It will only cost you %d coins to repair %s.'",
          price, obj->short_description);
  do_say(owner, buf, 9);
  act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
  act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
}


int repair_guy(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char item[256];
  int value0, cost, price, x;
  int percent, eqdam;

  if ((cmd != 66) && (cmd != 65)) return eFAILURE;

  if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eSUCCESS;
    }

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

  act("You give $N $p.", ch, obj, owner, TO_CHAR, 0);
  act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL);
  act("\n\r$N examines $p...", ch, obj, owner, TO_CHAR, 0);
  act("\n\r$N examines $p...", ch, obj, owner, TO_ROOM, INVIS_NULL);

  if(IS_OBJ_STAT(obj, ITEM_NOREPAIR)                     ||
     IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED)  ||
     obj->obj_flags.type_flag != ITEM_ARMOR || 
     IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)
    ) 
  {
    do_say(owner, "I can't repair this.", 9);
    act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
    act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  eqdam = eq_current_damage(obj);

  if (eqdam <= 0) 
  {
    do_say(owner, "Looks fine to me.", 9);
    act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
    act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  cost = obj->obj_flags.cost;
  value0 = eq_max_damage(obj);
  percent = ((100* eqdam) / value0);
  x = (100 - percent);          // now we know what percent to repair ..  
  price = ((cost * x) / 100);   // now we know what to charge them fuckers! 

  if (price < 100)
    price = 100;     // Welp.. Repair Guy needs to feed the kids somehow.. :)

  if (cmd == 65) {
    repair_shop_price_check(ch, owner, price, obj);
    return eSUCCESS;
  }

  if (GET_GOLD(ch) < (uint32)price) {
    repair_shop_complain_no_cash(ch, owner, price, obj);
    return eSUCCESS;
  } 

  repair_shop_fix_eq(ch, owner, price, obj);
  return eSUCCESS;
}

int super_repair_guy(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char item[256];
  int value0, value2, cost, price, x;
  int percent, eqdam;

  if ((cmd != 66) && (cmd != 65)) return eFAILURE;

  if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eSUCCESS;
    }

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
    do_say(owner, "I can't repair this.", 9);
    return eSUCCESS;
  }

  act("You give $N $p.", ch, obj, owner, TO_CHAR, 0);
  act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL);
  act("\n\r$N examines $p...", ch, obj, owner, TO_CHAR, 0);
  act("\n\r$N examines $p...", ch, obj, owner, TO_ROOM, INVIS_NULL);

  eqdam = eq_current_damage(obj);

  if (eqdam <= 0) {
    do_say(owner, "Looks fine to me.", 9);
    act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
    act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  cost = obj->obj_flags.cost;
  value0 = eq_max_damage(obj);

  if (obj->obj_flags.type_flag == ITEM_ARMOR ||
      obj->obj_flags.type_flag == ITEM_CONTAINER ||
		obj->obj_flags.type_flag == ITEM_LIGHT && !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    percent = ((100* eqdam) / value0);
    x = (100 - percent);               /* now we know what percent to repair ..  */
    price = ((cost * x) / 100);        /* now we know what to charge */
    price *= 2;                        /* he likes to charge more..  */
                                       /*  for armor... cuz he can.. */
  } 
  else if (obj->obj_flags.type_flag == ITEM_WEAPON ||
           obj->obj_flags.type_flag == ITEM_FIREWEAPON ||
		obj->obj_flags.type_flag == ITEM_INSTRUMENT ||
		obj->obj_flags.type_flag == ITEM_STAFF ||
		obj->obj_flags.type_flag == ITEM_WAND &&
     !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) 
  {
    percent = ((100* eqdam) / (value0 + value2));
    x = (100 - percent);               /* now we know what percent to repair ..  */
    price = ((cost * x) / 100);        /* now we know what to charge */
  }
  else {
    // Dunno how to repair non-weapons/armor
    do_say(owner, "I can't repair this.", 9);
    act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
    act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
	return eSUCCESS;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
    price *= 2;

  if (price < 1000)
    price = 1000;                      // Minimum price
  
  if (cmd == 65) {
    repair_shop_price_check(ch, owner, price, obj);
    return eSUCCESS;
  }

  if (GET_GOLD(ch) < (uint32)price) {
    repair_shop_complain_no_cash(ch, owner, price, obj);
    return eSUCCESS;
  } 
  else {
      repair_shop_fix_eq(ch, owner, price, obj);
      return eSUCCESS;
  }

  return eSUCCESS;
}

// Fingers
int repair_shop(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char item[256];
  int value0, value2, cost, price, x;
  int percent, eqdam;

  if ((cmd != 66) && (cmd != 65)) return eFAILURE;

  if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
  {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eSUCCESS;
  }

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
    do_say(owner, "I can't repair this.", 9);
    return eSUCCESS;
  }

  act("You give $N $p.", ch, obj, owner, TO_CHAR, 0);
  act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL);
  act("\n\r$N examines $p...", ch, obj, owner, TO_CHAR, 0);
  act("\n\r$N examines $p...", ch, obj, owner, TO_ROOM, INVIS_NULL);

  eqdam = eq_current_damage(obj);

  if (eqdam <= 0) {
    do_say(owner, "Looks fine to me.", 9);
    act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
    act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  cost = obj->obj_flags.cost;
  value0 = eq_max_damage(obj);

  if (obj->obj_flags.type_flag == ITEM_ARMOR ||
	obj->obj_flags.type_flag == ITEM_LIGHT &&
     !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) 
  {

    percent = ((100* eqdam) / value0);
    x = (100 - percent);          /* now we know what percent to repair ..  */
    price = ((cost * x) / 100);   /* now we know what to charge them fuckers! */
    price *= 4;                   /* he likes to charge more..  */
                                  /*  for armor... cuz he's a crook..  */
  } else if (obj->obj_flags.type_flag == ITEM_WEAPON ||
             obj->obj_flags.type_flag == ITEM_FIREWEAPON ||
             obj->obj_flags.type_flag == ITEM_CONTAINER ||
		obj->obj_flags.type_flag == ITEM_STAFF ||
		obj->obj_flags.type_flag == ITEM_WAND &&
     !IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) 

  {

    percent = ((100* eqdam) / (value0 + value2));
    x = (100 - percent);          /* now we know what percent to repair ..  */
    price = ((cost * x) / 100);   /* now we know what to charge them fuckers! */
    price *= 3;
  }
  else
  {
    // Dunno how to repair non-weapons/armor
    do_say(owner, "I can't repair this.", 9);
    act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
    act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
    price *= 2;

  if (price < 5000)
    price = 5000;     /* Welp.. Repair Guy needs to feed the kids somehow.. :) */
  
  if (cmd == 65) 
  {
    repair_shop_price_check(ch, owner, price, obj);
    return eSUCCESS;
  }

  if (GET_GOLD(ch) < (uint32)price) {
    repair_shop_complain_no_cash(ch, owner, price, obj);
    return eSUCCESS;
  } 
  else 
  {
    repair_shop_fix_eq(ch, owner, price, obj);
    return eSUCCESS;
  }
}

int corpse_cost(char_data * ch)
{
   int cost = 0;
   obj_data * curr_cont;

   for(obj_data * obj2 = ch->carrying; obj2; obj2 = obj2->next_content) 
   {
      if(obj2->obj_flags.type_flag == ITEM_MONEY)
        continue;
      for(curr_cont = obj2->contains; curr_cont; curr_cont = curr_cont->next_content)
      {
	if (!IS_SET(curr_cont->obj_flags.extra_flags, ITEM_SPECIAL))
        cost += curr_cont->obj_flags.cost; 
	}
	if (!IS_SET(obj2->obj_flags.extra_flags, ITEM_SPECIAL))
      cost += obj2->obj_flags.cost;
   }
   for(int x = 0; x < MAX_WEAR; x++) 
   {
      if(ch->equipment[x]) 
      {
        for(curr_cont = ch->equipment[x]->contains; 
            curr_cont; 
            curr_cont = curr_cont->next_content)
	if (!IS_SET(curr_cont->obj_flags.extra_flags, ITEM_SPECIAL))
          cost += curr_cont->obj_flags.cost; 

	if (!IS_SET(ch->equipment[x]->obj_flags.extra_flags, ITEM_SPECIAL))
        cost += ch->equipment[x]->obj_flags.cost;
      }
   }
   return cost;
}

int corpse_cost(obj_data * obj)
{
   int cost = 0;
   obj_data * curr_cont;

   for(obj_data * obj2 = obj->contains; obj2; obj2 = obj2->next_content) 
   {
      if(obj2->obj_flags.type_flag == ITEM_MONEY)
         continue;
      for(curr_cont = obj2->contains; curr_cont; curr_cont = curr_cont->next_content)
         cost += curr_cont->obj_flags.cost; 
      cost += obj2->obj_flags.cost; 
   }
   return cost;
}

int mortician(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  int x = 0, cost = 0, which;
  int count = 0;
  char buf[100];
  bool has_consent = FALSE;

  if(cmd != 56 && cmd != 59 && cmd != 58)
    return eFAILURE;

  // TODO - when determining price, it WILL NOT work if we ever institute
  // containers being inside other containers.

  if(cmd == 59)  // list 
  {
    sprintf(buf, "%s_consent", GET_NAME(ch));
    send_to_char("Available corpses (freshest first):\n\r$B", ch);
    for(obj = object_list; obj; obj = obj->next) 
    {
       if( GET_ITEM_TYPE(obj) != ITEM_CONTAINER ||
           obj->obj_flags.value[3] != 1)  // only look at corpses
          continue;

       if( !isname("pc", obj->name)     ||
           (!isname(GET_NAME(ch), obj->name) && !isname(buf, obj->name)))
         continue; 

       if (obj->in_room == ch->in_room) continue;
       if(!obj->contains)  // skip empty corpses
         continue;

       cost = corpse_cost(obj);
       cost /= 20000;
       cost = MAX(cost, 30); 
       sprintf(buf, "%d) %-21s %d Platinum coins.\n\r", ++count, obj->short_description,
               cost);
       send_to_char(buf, ch); 
    }
    send_to_char("$RIf any corpses were listed, they are still where you left them.  This\n\r"
                 "list is therefore always changing.  If you purchase one, it will be\n\r"
                 "placed at your feet. Use \"buy <number>\" to purchase a corpse.\n\r"
                 "Use 'value' to find how much your eq would cost with what you\n\r"
                 "have on you now.\n\r", ch);
    return eSUCCESS;
  }

  if(cmd == 58) // value
  {
    cost = corpse_cost(ch);
    cost /= 20000;
    cost = MAX(cost, 30);
    csendf(ch, "The Undertaker takes a look at you and estimates your corpse would cost around %d platinum coins.\n\r", cost);
    return eSUCCESS;
  }

  /* buy */
  if((which = atoi(arg)) == 0) { 
    send_to_char("Try \"buy <number>\", or \"list\" for a list of "
                 "available corpses.\n\r", ch);
    return eSUCCESS;
  }

  for(obj = object_list; obj; obj = obj->next) 
  {
     sprintf(buf, "%s_consent", GET_NAME(ch));

     if( GET_ITEM_TYPE(obj) != ITEM_CONTAINER ||
         obj->obj_flags.value[3] != 1)  // only look at corpses
        continue;

     if( !isname("pc", obj->name)         ||
         (!isname(GET_NAME(ch), obj->name) && !isname(buf, obj->name)) ||
        ++x < which)
       continue;

     if(!obj->contains)  // skip empty corpses
       continue;

     if(isname(buf, obj->name))
       has_consent = TRUE;
     if (obj->in_room == ch->in_room) continue; // Skip bought corpses

     cost = corpse_cost(obj);
     cost /= 20000;
     cost = MAX(cost, 30);
     if(GET_PLATINUM(ch) < (uint32)cost) {
       send_to_char("You can't afford that!\n\r", ch);
       return eSUCCESS;
     } 
     move_obj(obj, ch->in_room);
     REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
     send_to_char("The mortician goes into his freezer and returns with a corpse, which he\n\r"
                  "places at your feet.\n\r", ch);
     GET_PLATINUM(ch) -= cost;
     do_save(ch, "", 10);
     save_corpses();
     return eSUCCESS; 
  }
  
  send_to_char("No such corpse was found.  Try \"list\".\n\r", ch);
  return eSUCCESS; 
}


struct platinumsmith_data {
  char *name;
  char *attributes;
   int price;
  int  vnum;
};

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
 

// TODO - rewrite this more modular, so we can spread lots of plat guys all around
// the world that sell different pieces of godload
int platinumsmith(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  int x = 0, y;
  struct obj_data *new_new_obj;
  char buf[200];
  char buf2[200];

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

int meta_get_stat_exp_cost(char_data * ch, ubyte stat)
{
    int xp_price;
    int curr_stat;
    switch(stat) {
      case CONSTITUTION:
	curr_stat = ch->raw_con;
        break;
      case STRENGTH:
	  curr_stat = ch->raw_str;
        break;
      case DEXTERITY:
	curr_stat = ch->raw_dex;
//        xp_price = ((GET_LEVEL(ch)*4)*10000)+((ch->raw_dex*8)*30000);
        break;
      case INTELLIGENCE:
	curr_stat = ch->raw_intel;
//        xp_price = ((GET_LEVEL(ch)*4)*10000)+((ch->raw_intel*8)*30000);
        break;
      case WISDOM:
	curr_stat = ch->raw_wis;
//        xp_price = ((GET_LEVEL(ch)*4)*10000)+((ch->raw_wis*8)*30000);
        break;
      default:
        xp_price = 9999999;
        break;
    }
    switch(curr_stat)
    {
	case 1:
	case 2:
	case 3:
	case 4:
          xp_price = 2000000; break;
        case 5:
          xp_price = 3000000; break;
        case 6: xp_price = 4000000;break;
        case 7: xp_price = 5000000;break;
        case 8: xp_price = 5500000;break;
        case 9: xp_price = 6000000;break;
        case 10: xp_price = 6500000;break;
        case 11: xp_price = 7000000;break;
        case 12: xp_price = 7500000;break;
        case 13: xp_price = 8000000;break;
        case 14: xp_price = 8500000; break;
        case 15: xp_price = 9000000; break;
        case 16: xp_price = 9500000; break;
        case 29: xp_price = 25000000;break;
        default: xp_price = (curr_stat-7)*1000000; break;
    }

//    if(ch->pcdata->statmetas > 0)
 //     xp_price += ch->pcdata->statmetas * 20000;

    return xp_price;
}

int meta_get_stat_plat_cost(char_data * ch, ubyte targetstat)
{
  int plat_cost;
  int stat;

  switch(targetstat) {
    case CONSTITUTION:
      stat = ch->raw_con;
      break;
    case STRENGTH:
      stat = ch->raw_str;
      break;
    case DEXTERITY:
      stat = ch->raw_dex;
      break;
    case WISDOM:
      stat = ch->raw_wis;
      break;
    case INTELLIGENCE:
      stat = ch->raw_intel;
      break;
    default:
      stat = 99;
      break;
  }

  if (stat < 5) plat_cost = 100;
  else if (stat < 13) plat_cost = 250;
  else if (stat < 28) plat_cost = 250 + ((stat-12) *50);
  else if (stat == 28) plat_cost = 1250;
  else plat_cost = 1500;
/*  if(stat >= 18) {
     plat_cost = 500;
     if(ch->pcdata->statmetas > 0)
        plat_cost += ch->pcdata->statmetas * 20;
  } else {
     plat_cost = 100;
     if(ch->pcdata->statmetas > 0)
        plat_cost += ch->pcdata->statmetas * 10;
  }*/

  return plat_cost;
}

void meta_list_stats(char_data * ch)
{
    int xp_price, plat_cost, max_stat;

    xp_price = meta_get_stat_exp_cost(ch, STRENGTH);
    plat_cost = meta_get_stat_plat_cost(ch, STRENGTH);
    max_stat = get_max_stat(ch, STRENGTH);
    if(ch->raw_str >= max_stat)
      csendf(ch, "$B1)$R Str:       Your strength is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B1)$R Str: %d        Cost: %d exp + %d Platinum coins. \n\r",
              ( ch->raw_str + 1), xp_price, plat_cost);

    xp_price = meta_get_stat_exp_cost(ch, DEXTERITY);
    plat_cost = meta_get_stat_plat_cost(ch, DEXTERITY);
    max_stat = get_max_stat(ch, DEXTERITY);
    if(ch->raw_dex >= max_stat)
      csendf(ch, "$B2)$R Dex:       Your dexterity is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B2)$R Dex: %d        Cost: %d exp + %d Platinum coins.\n\r",
              ( ch->raw_dex + 1 ), xp_price, plat_cost);

    xp_price = meta_get_stat_exp_cost(ch, CONSTITUTION);
    plat_cost = meta_get_stat_plat_cost(ch, CONSTITUTION);
    max_stat = get_max_stat(ch, CONSTITUTION);
    if(ch->raw_con >= max_stat)
      csendf(ch, "$B3)$R Con:       Your constitution is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B3)$R Con: %d        Cost: %d exp + %d Platinum coins.\n\r",
              ( ch->raw_con + 1 ), xp_price, plat_cost);

    xp_price = meta_get_stat_exp_cost(ch, INTELLIGENCE);
    plat_cost = meta_get_stat_plat_cost(ch, INTELLIGENCE);
    max_stat = get_max_stat(ch, INTELLIGENCE);
    if(ch->raw_intel >= max_stat)
      csendf(ch, "$B4)$R Int:       Your intelligence is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B4)$R Int: %d        Cost: %d exp + %d Platinum coins.\n\r",
              ( ch->raw_intel + 1 ), xp_price, plat_cost);

    xp_price = meta_get_stat_exp_cost(ch, WISDOM);
    plat_cost = meta_get_stat_plat_cost(ch, WISDOM);
    max_stat = get_max_stat(ch, WISDOM);
    if(ch->raw_wis >= max_stat)
      csendf(ch, "$B5)$R Wis:       Your wisdom is already %d.\n\r", max_stat);
    else
      csendf(ch, "$B5)$R Wis: %d        Cost: %d exp + %d Platinum coins.\n\r",
              ( ch->raw_wis + 1 ), xp_price, plat_cost);

}

int meta_get_moves_exp_cost(char_data * ch)
{
   int meta = GET_MOVE_METAS(ch);  
  if (GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch) < 0)
   meta += GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch);
   return new_meta_exp_cost_one(MAX(0,meta));
}

int meta_get_moves_plat_cost(char_data * ch)
{
   int meta = GET_MOVE_METAS(ch);
  if (GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch) < 0)
   meta += GET_MAX_MOVE(ch) - GET_RAW_MOVE(ch);
   return (int)new_meta_platinum_cost(MAX(0,meta), MAX(0,meta)+1);
}

int meta_get_hps_exp_cost(char_data * ch)
{
   int meta = GET_HP_METAS(ch);
   int bonus = 0;

   for(int i = 16; i < GET_RAW_CON(ch); i++)
      bonus += (i * i) / 30;

   meta -= bonus;

   if (GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch) > 0)
      meta -= GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch);

   return new_meta_exp_cost_one(MAX(0,meta));
}

int meta_get_hps_plat_cost(char_data * ch)
{
   int meta = GET_HP_METAS(ch);
   int bonus = 0;

   for(int i = 16; i < GET_RAW_CON(ch); i++)
      bonus += (i * i) / 30;

   meta -= bonus;

   if (GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch) > 0)
      meta -= GET_RAW_HIT(ch) + bonus - GET_MAX_HIT(ch);

   return (int)new_meta_platinum_cost(MAX(0,meta), MAX(0,meta)+1);
}

int meta_get_mana_exp_cost(char_data * ch)
{
   int meta = GET_MANA_METAS(ch);
   int stat, bonus = 0;

   if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
      stat = GET_RAW_INT(ch);
   else if(GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_DRUID)
      stat = GET_RAW_WIS(ch);
   else stat = 0;

   for(int i = 16; i < stat; i++)
      bonus += (i * i) / 30;

   meta -= bonus;

   if (GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch) > 0)
      meta -= GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch);

   return new_meta_exp_cost_one(MAX(0,meta));
}

int meta_get_mana_plat_cost(char_data * ch)
{
   int meta = GET_MANA_METAS(ch);
   int stat, bonus = 0;

   if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_ANTI_PAL || GET_CLASS(ch) == CLASS_RANGER)
      stat = GET_RAW_INT(ch);
   else if(GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_DRUID)
      stat = GET_RAW_WIS(ch);
   else stat = 0;

   for(int i = 16; i < stat; i++)
      bonus += (i * i) / 30;

   meta -= bonus;

   if (GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch) > 0)
      meta -= GET_RAW_MANA(ch) + bonus - GET_MAX_MANA(ch);

   return (int)new_meta_platinum_cost(MAX(0,meta), MAX(0,meta)+1);
}

int meta_get_ki_exp_cost(char_data * ch)
{
  int cost, stat;
  switch (GET_CLASS(ch))
  {
    case CLASS_MONK:
      cost = 7700;
      stat = GET_RAW_WIS(ch) - 15;
      stat = MAX(0,stat);
      break;
    case CLASS_BARD:
      cost = 7400;
      stat = GET_RAW_INT(ch) - 15;
      stat = MAX(0,stat);
      break;
    default: return 0;
  }
  cost = 10000000 + ((GET_MAX_KI(ch)-stat) * cost);
  return (int)(cost*1.2);
}
 
int meta_get_ki_plat_cost(char_data * ch)
{
  int cost, stat;
  switch (GET_CLASS(ch))
  {
    case CLASS_MONK:
      cost = 500;
      stat = GET_RAW_WIS(ch) - 15;
      stat = MAX(0,stat);
      break;
    case CLASS_BARD:
      cost = 400;
      stat = GET_RAW_INT(ch) - 15;
      stat = MAX(0,stat);
      break;
    default: return 0;
  } 
  cost = 500 + cost + (((GET_MAX_KI(ch)-stat)/2) * ((GET_MAX_KI(ch)-stat)/10));
  return (int)(cost*0.9);
}

int meta_dude(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  //char buf[256];
  char argument[256];

  int stat;
  int choice;
  int increase;
  int hit_cost, mana_cost, move_cost, ki_cost, hit_exp, move_exp, mana_exp, ki_exp;
  int statplatprice, max_stat;

  //long gold;

  //truct obj_data *new_new_obj;
  sbyte *pstat; 
  int pprice;

  //const double EXPCONV  = 400000000.0;
  //const double GOLDCONV = 1000000.0;

  if((cmd != 59) && (cmd != 56))
    return eFAILURE;

  if (IS_AFFECTED(ch, AFF_BLIND))
    return eFAILURE;

  if(IS_NPC(ch))
    return eFAILURE;

  if(GET_LEVEL(ch) < 10) {
    send_to_char("$B$2The Meta-physician tells you, 'You're too low level for$R "
                 "$B$2me to waste my time on you.$R\n\r"
                 "$B$2Prove to me you are gonna stick around first!'$R.", ch);
    return eSUCCESS;
  }

  hit_exp = meta_get_hps_exp_cost(ch);
  move_exp = meta_get_moves_exp_cost(ch);
  mana_exp = meta_get_mana_exp_cost(ch);

  hit_cost = meta_get_hps_plat_cost(ch);
  move_cost = meta_get_moves_plat_cost(ch);
  mana_cost = meta_get_mana_plat_cost(ch); 

  if(!IS_MOB(ch)) {
    ki_exp = meta_get_ki_exp_cost(ch);
    ki_cost = meta_get_ki_plat_cost(ch);
  }

  if(cmd == 59) {           /* List */
    send_to_char("$B$2The Meta-physician tells you, 'This is what I can do for you...$R \n\r", ch);

    meta_list_stats(ch);
     
    if (hit_exp && hit_cost) 
    csendf(ch, "$B6)$R Add 5 points to your hit points:   %d experience points and %d"
            " Platinum coins.\n\r", hit_exp, hit_cost); 
    else
    csendf(ch, "$B6)$R Add to your hit points:   You cannot do this.\r\n");

    if (hit_exp && hit_cost) 
    csendf(ch, "$B7)$R Add 1 point to your hit points:   %d experience points and %d"
            " Platinum coins.\n\r", (int)(hit_exp/5*1.1), (int)(hit_cost/5*1.1)); 
    else
    csendf(ch, "$B7)$R Add to your hit points:   You cannot do this.\r\n");

    if (mana_exp && mana_cost)
    csendf(ch, "$B8)$R Add 5 points to your mana points:  %d experience points and %d"
            " Platinum coins.\n\r", mana_exp, mana_cost);
    else
    csendf(ch, "$B8)$R Add to your mana points:  You cannot do this.\r\n");

    if (mana_exp && mana_cost) 
    csendf(ch, "$B9)$R Add 1 point to your mana points:   %d experience points and %d"
            " Platinum coins.\n\r", (int)(mana_exp/5*1.1), (int)(mana_cost/5*1.1)); 
    else
    csendf(ch, "$B9)$R Add to your mana points:   You cannot do this.\r\n");

    if (move_exp && move_cost)
    csendf(ch, "$B10)$R Add 5 points to your movement points: %d experience points and %d"
            " Platinum coins.\n\r", move_exp, move_cost);
    else
    csendf(ch, "$B10)$R Add to your movement points:  You cannot do this.\r\n");

    if (move_exp && move_cost) 
    csendf(ch, "$B11)$R Add 1 points to your movement points:   %d experience points and %d"
            " Platinum coins.\n\r", (int)(move_exp/5*1.1), (int)(move_cost/5*1.1)); 
    else
    csendf(ch, "$B11)$R Add to your movement points:   You cannot do this.\r\n");

    if(!IS_MOB(ch) && ki_cost && ki_exp) {   // mobs can't meta ki
//        send_to_char("12) Your ki is already meta'd fully.\n\r", ch);
      csendf(ch, "$B12)$R Add a point of ki:        %d experience points and %d Platinum.\n\r", ki_exp, ki_cost);
    }
    else if (!IS_MOB(ch))
    csendf(ch, "$B12)$R Add a point of ki:        You cannot do this.\r\n");

    send_to_char(
    "$B13)$R One (1) Platinum coin     Cost: 20,000 Gold Coins.\n\r"
    "$B14)$R Five (5) Platinum coins   Cost: 100,000 Gold Coins.\n\r"
    "$B15)$R 250 Platinum coins        Cost: 5,000,000 Gold Coins.\r\n"
    "$B16)$R 100,000 Gold Coins        Cost: Five (5) Platinum coins.\r\n"
    "$B17)$R 5,000,000 Gold Coins      Cost: 250 Platinum coins.\r\n"
    "$B18)$R Convert experience to gold. (100mil Exp. = 500000 Gold.)\r\n"
    "$B19)$R A deep blue potion of healing. Cost: 25 Platinum coins.\r\n"
    "$B20)$R A deep red vial of mana. Cost: 50 Platinum coins.\r\n"
    "$B21)$R Buy a practice session for 25 plats.\r\n"
    "$B22)$R Freedom from HUNGER and THIRST:  Currently out of stock.\r\n"
                 , ch);

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
             pstat = &(ch->raw_str); 
             pprice = meta_get_stat_exp_cost(ch, STRENGTH);
             statplatprice = meta_get_stat_plat_cost(ch, STRENGTH);
             max_stat = get_max_stat(ch, STRENGTH);
             break;
      case 2: stat = ch->raw_dex;
             pstat = &(ch->raw_dex); 
             pprice =  meta_get_stat_exp_cost(ch, DEXTERITY); 
             statplatprice = meta_get_stat_plat_cost(ch, DEXTERITY);
             max_stat = get_max_stat(ch, DEXTERITY);
             break;
      case 3: stat = ch->raw_con;
             pstat = &(ch->raw_con); 
             pprice = meta_get_stat_exp_cost(ch, CONSTITUTION); 
             statplatprice = meta_get_stat_plat_cost(ch, CONSTITUTION);
             max_stat = get_max_stat(ch, CONSTITUTION);
             break;
      case 4: stat = ch->raw_intel;
             pstat = &(ch->raw_intel); 
             pprice = meta_get_stat_exp_cost(ch, INTELLIGENCE); 
             statplatprice = meta_get_stat_plat_cost(ch, INTELLIGENCE);
             max_stat = get_max_stat(ch, INTELLIGENCE);
             break;
      case 5: stat = ch->raw_wis;
             pstat = &(ch->raw_wis); 
             pprice = meta_get_stat_exp_cost(ch, WISDOM); 
             statplatprice = meta_get_stat_plat_cost(ch, WISDOM);
             max_stat = get_max_stat(ch, WISDOM);
             break;
      default: stat = 0;
    }

    if(choice < 6) {
      
      if(GET_PLATINUM(ch) < (unsigned)statplatprice) {
        send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services.  SCRAM!'$R\n\r", ch);
        return eSUCCESS;
      }
      if(GET_EXP(ch) < pprice) {
        send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
        return eSUCCESS;
      }
      if(stat >= max_stat) {
        send_to_char("$B$2The Meta-physician tells you, 'You're already as good at that as yer gonna get.'$R\n\r", ch);
        return eSUCCESS;
      }

      GET_EXP(ch) -= pprice;
      GET_PLATINUM(ch) -= statplatprice;

      *pstat += 1;
      ch->pcdata->statmetas++;

      act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
      act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

      // affect the stat by 0 to reflect the new raw stat
      affect_modify(ch, APPLY_STR, 0, -1, TRUE);
      affect_modify(ch, APPLY_DEX, 0, -1, TRUE);
      affect_modify(ch, APPLY_INT, 0, -1, TRUE);
      affect_modify(ch, APPLY_WIS, 0, -1, TRUE);
      affect_modify(ch, APPLY_CON, 0, -1, TRUE);

      redo_hitpoints(ch);
      redo_mana(ch);
      redo_ki(ch);
      return eSUCCESS;
    }

   if(choice == 6 && hit_exp && hit_cost) {
     if(GET_EXP(ch) < hit_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)hit_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }
     GET_EXP(ch) -= hit_exp;
     GET_PLATINUM(ch) -= hit_cost;

     increase = 5;
     ch->raw_hit += increase;
     GET_HP_METAS(ch) += 5;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_hitpoints(ch);
     return eSUCCESS;
   }

   if(choice == 7 && hit_exp && hit_cost) {
     hit_exp = (int)(hit_exp/5*1.1);
     hit_cost = (int)(hit_cost/5*1.1);

     if(GET_EXP(ch) < hit_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)hit_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!$R'\n\r", ch);
       return eSUCCESS;
     }
     GET_EXP(ch) -= hit_exp;
     GET_PLATINUM(ch) -= hit_cost;

     increase = 1;
     ch->raw_hit += increase;
     GET_HP_METAS(ch) += 1;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_hitpoints(ch);
     return eSUCCESS;
   }

   if(choice == 8 && mana_exp && mana_cost) {

     if(GET_EXP(ch) < mana_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)mana_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch) -= mana_exp;
     GET_PLATINUM(ch) -= mana_cost;

     increase = 5;
     ch->raw_mana += increase;
     GET_MANA_METAS(ch) += 5;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 9 && mana_exp && mana_cost) {
     mana_exp = (int)(mana_exp/5*1.1);
     mana_cost = (int)(mana_cost/5*1.1);

     if(GET_EXP(ch) < mana_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)mana_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch) -= mana_exp;
     GET_PLATINUM(ch) -= mana_cost;

     increase = 1;
     ch->raw_mana += increase;
     GET_MANA_METAS(ch) += 1;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 10 && move_exp && move_cost) 
   {
     if(GET_EXP(ch) < move_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)move_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch)  -= move_exp;
     GET_PLATINUM(ch) -= move_cost;
     ch->raw_move += 5;
     ch->max_move += 5;
     GET_MOVE_METAS(ch) += 5;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }

   if(choice == 11 && move_exp && move_cost) 
   {
     move_exp = (int)(move_exp/5*1.1);
     move_cost = (int)(move_cost/5*1.1);

     if(GET_EXP(ch) < move_exp) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }
     if(GET_PLATINUM(ch) < (uint32)move_cost) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch)  -= move_exp;
     GET_PLATINUM(ch) -= move_cost;
     ch->raw_move += 1;
     ch->max_move += 1;
     GET_MOVE_METAS(ch) += 1;
     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
     redo_hitpoints(ch);
     redo_mana(ch);
     return eSUCCESS;
   }

/*   if(choice == 22) {
     price = 100000000;
     if(GET_COND(ch, FULL) == -1) {
       send_to_char("$B$2The Meta-physician tells you, 'You already have freedom from hunger and thirst!'$R\n\r", 
ch);
       return eSUCCESS;
     }

     if(GET_EXP(ch) < price) {
       send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
       return eSUCCESS;
     }

     if(GET_PLATINUM(ch) < 2000) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }

     GET_EXP(ch) -= price;
     GET_PLATINUM(ch) -= 2000;

     GET_COND(ch,THIRST) = -1;
     GET_COND(ch, FULL) = -1;

     act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
     act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);

     return eSUCCESS;
   }
*/
  if (choice == 19 || choice == 20)
  {
   int vnum = choice == 19 ? 27903: 27904;
   unsigned int cost = choice == 19 ? 25:50;
   if (GET_PLATINUM(ch) < cost)
   {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford that!'$R\r\n",ch);
      return eSUCCESS;
   }
   struct obj_data *obj = clone_object(real_object(vnum));
   if ( IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch) )
    {
        send_to_char( "You can't carry that many items.\n\r", ch );
	extract_obj(obj);
        return eSUCCESS;
    }

    if ( IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch) )
    {
        send_to_char( "You can't carry that much weight.\n\r", ch );
	extract_obj(obj);
        return eSUCCESS;
   }
   GET_PLATINUM(ch) -= cost;
   obj_to_char(obj,ch);
   send_to_char("$B$2The Meta-physician tells you, 'Here is your potion.'$R\r\n",ch);
   return eSUCCESS;
  }
   if(choice == 14) {
     if (affected_by_spell(ch, FUCK_GTHIEF))
     {
	send_to_char("$B$2The Meta-physician tells you, 'You cannot do this because of your criminal actions!'$R\r\n",ch);
	return eSUCCESS;
     }
     if(GET_GOLD(ch) < 100000) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }
     GET_GOLD(ch) -= 100000;
     GET_PLATINUM(ch) += 5;
     send_to_char("Ok.\n\r", ch);
     return eSUCCESS;
   }
   if(choice == 13) {
     if (affected_by_spell(ch, FUCK_GTHIEF))
     {
        send_to_char("$B$2The Meta-physician tells you, 'You cannot do this because of your criminal actions!'$R\r\n",ch);
        return eSUCCESS;
     }
     if(GET_GOLD(ch) < 20000) {
       send_to_char("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R\n\r", ch);
       return eSUCCESS;
     }
     GET_GOLD(ch) -= 20000;
     GET_PLATINUM(ch) += 1;
     send_to_char("Ok.\n\r", ch);
     return eSUCCESS;
   }
  if(choice == 15) {
    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eSUCCESS;
    }

    if(GET_GOLD(ch) < 5000000) {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R\n\r", ch);
      return eSUCCESS;
    }
    GET_PLATINUM(ch) += 250;
    GET_GOLD(ch) -= 5000000;
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
  }
  if(choice == 16) {
    if(GET_PLATINUM(ch) < 5) {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford that.  SCRAM!'$R\n\r", ch);
      return eSUCCESS;
    }
    GET_PLATINUM(ch) -= 5;
    GET_GOLD(ch) += 100000;
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
  }

  if(choice == 17) {
    if(GET_PLATINUM(ch) < 250) {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford that!  SCRAM$R\n\r", ch);
      return eSUCCESS;
    }
    GET_PLATINUM(ch) -= 250;
    GET_GOLD(ch) += 5000000;
    send_to_char("Ok.\n\r", ch);
    return eSUCCESS;
  }


  if(choice == 21) {
    if (GET_PLATINUM(ch) < 25) {
       send_to_char ("Costs 25 plats...which you don't have.\n\r", ch);
       return eSUCCESS;
    }
    if (IS_MOB(ch)) {
       send_to_char ("You can't buy practices chode...\r\n", ch);
       return eSUCCESS;
    }
    send_to_char("The Meta-Physician gives you a practice session.\n\r", ch);
 
    GET_PLATINUM(ch) -= 25;
    ch->pcdata->practices += 1;
    return eSUCCESS;
  }
  if(choice == 12 && ki_exp && ki_cost) {
    if(IS_MOB(ch)) {
      send_to_char("Mobs cannot meta ki.\r\n", ch);
      return eSUCCESS;
    }
    if(GET_EXP(ch) < ki_exp) {
      send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
      return eSUCCESS;
    }    
    if(GET_PLATINUM(ch) < (uint32)(ki_cost)) {
      send_to_char("$B$2The Meta-physician tells you, 'You can't afford my services!  SCRAM!'$R\n\r", ch);
      return eSUCCESS;
    }/*
    if(GET_KI_METAS(ch) > 4) {
      send_to_char("$B$2The Meta-physician tells you, 'You have already meta'd your ki to the maximum.'$R\n\r", ch);
      return eSUCCESS;
    }*/

    GET_EXP(ch) -= ki_exp;
    GET_PLATINUM(ch) -= ki_cost;
    
    ch->raw_ki += 1;
    GET_KI_METAS(ch) += 1;
    redo_ki(ch);
    act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
    act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
    return eSUCCESS;
  }
  if (choice == 18) {
    if (GET_EXP(ch) < 100000000) {
      send_to_char("$B$2The Meta-physician tells you, 'You lack the experience.'$R\n\r", ch);
      return eSUCCESS;
    }
    if (IS_MOB(ch)) {
       send_to_char ("What would you have to spend gold on chode?\r\n", ch);
       return eSUCCESS;
    }
    
    GET_EXP(ch) -= 100000000;
    GET_GOLD(ch) += 500000;

    act("The Meta-physician touches $n.",  ch, 0, 0, TO_ROOM, 0);
    act("The Meta-physician touches you.",  ch, 0, 0, TO_CHAR, 0);
    return eSUCCESS;
  }
 }
  send_to_char("$B$2The Meta-physician tells you, 'Buy what?!'$R\n\r", ch);
  return eSUCCESS;
}


char *gl_item(OBJ_DATA *obj, int number, CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],buf3[MAX_STRING_LENGTH];
  int length,i = 0;
  sprintf(buf,"$B$7%-2d$R) %s ", number+1, obj->short_description);
  extern char* apply_types[];
  if (obj->obj_flags.type_flag == ITEM_WEAPON) { // weapon
    sprintf(buf, "%s%dd%d, %s, ", buf, obj->obj_flags.value[1], obj->obj_flags.value[2],
		IS_SET(obj->obj_flags.extra_flags, ITEM_TWO_HANDED)?"Two-handed":"One-handed");
  }
  length = strlen(buf);
 for (;i<obj->num_affects;i++)
    if ((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0))
    {
      if (obj->affected[i].location < 1000)
         sprinttype(obj->affected[i].location,apply_types,buf2);
      else if (get_skill_name(obj->affected[i].location/1000))
         strcpy(buf2, get_skill_name(obj->affected[i].location/1000));
      else strcpy(buf2, "Invalid");

      sprintf(buf3,"%s by %d, ", buf2, obj->affected[i].modifier);

      for (unsigned int a = 0; a < strlen(buf3);a++)
	buf3[a] = LOWER(buf3[a]); // these affects are all lowercase
      if (length + strlen(buf3) > 90)
      {
	length = 0;
	sprintf(buf, "%s\r\n     %s",buf,buf3);
      } else {
	length += strlen(buf3);
	sprintf(buf, "%s%s",buf,buf3);
      }
      
    }
    if (class_restricted(ch, obj) || size_restricted(ch, obj))
    {
      sprintf(buf2,"$4[restricted]$R, ");
      if (length + strlen(buf2) > 90)
      {
	length = 0;
	sprintf(buf, "%s\r\n     %s",buf,buf2);
      } else {
  	length += strlen(buf2);
  	sprintf(buf, "%s%s",buf,buf2);
      }

    }
    sprintf(buf2,"costing %d coins.\r\n",obj->obj_flags.cost/10);
    if (length + strlen(buf2) > 90)
    {
	length = 0;
	sprintf(buf, "%s\r\n     %s",buf,buf2);
    } else {
	length += strlen(buf2);
	sprintf(buf, "%s%s",buf,buf2);
    }
  return str_dup(buf);
}

struct platsmith
{
  int vnum;
  int sales[13];
};

const struct platsmith platsmith_list[]=
{
 {10019, {512, 513, 514, 515, 537, 538, 539, 540, 541,   0,   0,   0,   0}},
 {10020, {554, 555, 556, 557, 524, 525, 526, 527, 504, 505, 506, 511,   0}},
 {10021, {516, 517, 518, 519, 507, 508, 509, 510, 546, 547, 548, 549,   0}},
 {10022, {500, 501, 502, 503, 520, 521, 522, 523, 528, 529, 530, 531,   0}},
 {10023, {542, 543, 544, 545, 532, 533, 534, 535, 536, 550, 551, 552, 553}},
 {10026, {558, 559, 560, 561, 562, 563, 564, 565, 566,   0,   0,   0,   0}}, 
 {10004, {570, 571, 575, 577, 578, 580, 582, 584, 586, 587, 590, 591,   0}}, //weapon dude in cozy
 {10024, {593, 594, 567, 568,   0,   0,   0,   0,   0,   0,   0,   0,   0}}, //2handed weapon/bow dude
 {0, {0,0,0,0,0,0,0,0,0,0,0,0,0}}
};

//Apoc enjoys the dirty mooselove. Honest.
int godload_sales(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  extern struct index_data *mob_index;
  int mobvnum = mob_index[owner->mobdata->nr].virt;
  int o;
  char buf[MAX_STRING_LENGTH];
//  return eFAILURE; //disabled for now
 if (cmd == 59) {
  if (!CAN_SEE(owner, ch))
  {
     do_say(owner, "I don't trade with people I can't see!",0);
	return eSUCCESS;
  }

  for (o = 0; platsmith_list[o].vnum != 0; o++)
    if (mobvnum == platsmith_list[o].vnum) break;
  if (platsmith_list[o].vnum == 0)
  {
    sprintf(buf, "%s Sorry, I don't seem to be working correctly. Do tell someone.",GET_NAME(ch));
    do_tell(owner, buf, 0); 
    return eSUCCESS;
  }
  extern char* pc_clss_types3[];
  sprintf(buf, "%s Here's what I can do for you, %s.",GET_NAME(ch),pc_clss_types3[GET_CLASS(ch)]);
  do_tell(owner, buf, 0); 
  for (int z = 0; z < 13 && platsmith_list[o].sales[z] != 0; z++) {
    char *tmp = gl_item((OBJ_DATA*)obj_index[real_object(platsmith_list[o].sales[z])].item,z,ch);
    send_to_char(tmp, ch);
    dc_free(tmp);
  }
  return eSUCCESS; 
 } else if (cmd == 56) {
  if (!CAN_SEE(owner, ch))
  {
     do_say(owner, "I don't trade with people I can't see!",0);
	return eSUCCESS;
  }

  for (o = 0; platsmith_list[o].vnum != 0; o++)
    if (mobvnum == platsmith_list[o].vnum) break;
  char buf[MAX_STRING_LENGTH],arg2[MAX_INPUT_LENGTH];
  one_argument(arg,arg2);
  if (platsmith_list[o].vnum == 0)
  {
    sprintf(buf, "%s Sorry, I don't seem to be working correctly. Do tell someone.",GET_NAME(ch));
    do_tell(owner, buf, 0); 
    return eSUCCESS;
  }
  if (!is_number(arg2))
  {
    sprintf(buf, "%s Sorry, mate. You type buy <number> to specify what you want..",GET_NAME(ch));
    do_tell(owner, buf, 0); 
    return eSUCCESS;
  }
  int k = atoi(arg2)-1;
  if (k >= 13 || k < 0 || platsmith_list[o].sales[k] == 0)
  {
    sprintf(buf, "%s Don't have that I'm afraid. Type \"list\" to see my wares.",GET_NAME(ch));
    do_tell(owner, buf, 0); 
    return eSUCCESS;
  }
   struct obj_data *obj;
   obj = clone_object(real_object(platsmith_list[o].sales[k]));
   if (class_restricted(ch, obj) || size_restricted(ch, obj) ||search_char_for_item(ch, obj->item_number) )
   {
    sprintf(buf, "%s That item is not available to you.",GET_NAME(ch));
    do_tell(owner, buf, 0); 
    extract_obj(obj);
    return eSUCCESS;
   }
   if (GET_PLATINUM(ch) < (unsigned int)(obj->obj_flags.cost/10))
   {
    sprintf(buf, "%s Come back when you've got the platinum.",GET_NAME(ch));
    do_tell(owner, buf, 0); 
    extract_obj(obj);
    return eSUCCESS;
   }
   GET_PLATINUM(ch) -= obj->obj_flags.cost/10;
   sprintf(buf, "%s %s", obj->name, GET_NAME(ch));
   obj->name = str_hsh(buf);
   obj_to_char(obj, ch);
   sprintf(buf, "%s Here's your %s$B$2. Have a nice time with it.",GET_NAME(ch),obj->short_description);
   do_tell(owner, buf, 0); 
    return eSUCCESS;
  } else if (cmd == 57) {
    OBJ_DATA *obj;
    char arg2[MAX_INPUT_LENGTH];
    one_argument(arg,arg2);
    obj = get_obj_in_list_vis(ch, arg2, ch->carrying);
    if (!CAN_SEE(owner, ch))
    {
      do_say(owner, "I don't trade with people I can't see!",0);
      return eSUCCESS;
    }
    if (!obj)
    {
      sprintf(buf, "%s Try that on the cooky meta-physician..",GET_NAME(ch));
      do_tell(owner, buf, 0); 
      return eSUCCESS;
    }
    if (!IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
        sprintf(buf, "%s I don't deal in worthless junk.",GET_NAME(ch));
        do_tell(owner, buf, 0); 
        return eSUCCESS;
    }
    int cost = obj->obj_flags.cost/10;

    sprintf(buf, "%s I'll give you %d plats for that. Thanks for shoppin'.",GET_NAME(ch),cost);
    do_tell(owner, buf, 0);
    extract_obj(obj);
    GET_PLATINUM(ch) += cost;
    return eSUCCESS;
  }
 return eFAILURE;
}

//gl_repair_guy
int gl_repair_shop(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  char item[256];
  int value0, value2, cost, price, x;
  int percent, eqdam;

  if ((cmd != 66) && (cmd != 65)) return eFAILURE;

  if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eSUCCESS;
    }


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
    do_say(owner, "I can't repair this.", 9);
    return eSUCCESS;
  }

  act("You give $N $p.", ch, obj, owner, TO_CHAR, 0);
  act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL);
  act("\n\r$N examines $p...", ch, obj, owner, TO_CHAR, 0);
  act("\n\r$N examines $p...", ch, obj, owner, TO_ROOM, INVIS_NULL);

  eqdam = eq_current_damage(obj);

  if (eqdam <= 0) {
    do_say(owner, "Looks fine to me.", 9);
    act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
    act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  cost = obj->obj_flags.cost;
  value0 = eq_max_damage(obj);
  
  if (!IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
      do_say(owner,"I don't repair this kind of junk.",9);
      act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
      act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
      return eSUCCESS;
  }
  if (obj->obj_flags.type_flag == ITEM_ARMOR ||
	obj->obj_flags.type_flag == ITEM_LIGHT)
  {

    percent = ((100* eqdam) / value0);
    x = (100 - percent);          /* now we know what percent to repair ..  */
    price = ((cost * x) / 100);   /* now we know what to charge them fuckers! */
    price *= 4;                   /* he likes to charge more..  */
                                  /*  for armor... cuz he's a crook..  */
  } else if (obj->obj_flags.type_flag == ITEM_WEAPON ||
             obj->obj_flags.type_flag == ITEM_CONTAINER ||
		obj->obj_flags.type_flag == ITEM_STAFF ||
		obj->obj_flags.type_flag == ITEM_WAND)
  {

    percent = ((100* eqdam) / (value0 + value2));
    x = (100 - percent);          /* now we know what percent to repair ..  */
    price = ((cost * x) / 100);   /* now we know what to charge them fuckers! */
    price *= 5;
  }
  else
  {
    // Dunno how to repair non-weapons/armor
    do_say(owner, "I can't repair this.", 9);
    act("$N gives you $p.", ch, obj, owner, TO_CHAR, 0);
    act("$N gives $n $p.", ch, obj, owner, TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
    price *= 2;

  if (price < 50000)
    price = 50000;     /* Welp.. Repair Guy needs to feed the kids somehow.. :) */
  
  if (cmd == 65) 
  {
    repair_shop_price_check(ch, owner, price, obj);
    return eSUCCESS;
  }

  if (GET_GOLD(ch) < (uint32)price) {
    repair_shop_complain_no_cash(ch, owner, price, obj);
    return eSUCCESS;
  } 
  else 
  {
    repair_shop_fix_eq(ch, owner, price, obj);
    return eSUCCESS;
  }
}

