/***************************************************************************
 *  file: shop.c , Shop module.                            Part of DIKUMUD *
 *  Usage: Procedures handling shops and shopkeepers.                      *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: shop.cpp,v 1.33 2014/07/04 22:00:04 jhhudso Exp $ */

#include <cstdio>
#include <cstring>
#include <fmt/format.h>

#include "affect.h"
#include "character.h"
#include "utility.h"
#include "interp.h"
#include "obj.h"
#include "levels.h"
#include "player.h"
#include "handler.h"
#include "mobile.h"
#include "room.h"
#include "fileinfo.h"
#include "db.h"
#include "act.h"
#include "returnvals.h"
#include "shop.h"
#include "spells.h"
#include "inventory.h"
#include "const.h"
#include "wizard.h"

extern struct index_data *mob_index;

struct player_shop * g_playershops;

extern CWorld world;
extern struct index_data *obj_index;
 
extern struct time_info_data time_info;

struct shop_data shop_index[MAX_SHOP];
int max_shop;

// extern function
int fwrite_string(char *buf, FILE *fl);

/*
 * See if a shop keeper wants to trade.
 */
int is_ok( CHAR_DATA *keeper, CHAR_DATA *ch, int shop_nr )
{
    char buf[240];

    // Undesirables.
    // TODO - Figure out if KILLER does anything we want to kill...
    // If not, let's use the AFF bit for something useful....
    if ( ISSET(ch->affected_by, AFF_KILLER) )
    {
        do_say( keeper, "Go away before I call the guards!!", 0 );
        sprintf( buf, "%s the KILLER is over here!\n\r", GET_SHORT(ch) );
        do_shout( keeper, buf, 0 );
        return FALSE;
    }

    /*
     * Invisible people.
     */
    if ( !CAN_SEE( keeper, ch ) )
    {
        do_say( keeper, "I don't trade with someone I can't see!", 0 );
        return FALSE;
    }

    /*
     * Shop hours.
     */
    if ( time_info.hours < shop_index[shop_nr].open1 )
    {
        do_say( keeper, "Come back later!", 0 );
        return FALSE;
    }
    
    else if ( time_info.hours <= shop_index[shop_nr].close1 ) {
        return TRUE;
    } else if ( time_info.hours < shop_index[shop_nr].open2 ) {
        do_say( keeper, "Come back later!", 0 );
        return FALSE;
    } else if ( time_info.hours <= shop_index[shop_nr].close2 ) {
    	return TRUE;
    } else {
        do_say( keeper, "Sorry, come back tomorrow.", 0 );
        return FALSE;
    }

    return TRUE;
}



/*
 * See if a shop will buy an item.
 */
int trade_with( struct obj_data *item, int shop_nr )
{
    int counter;

    if ( item->obj_flags.cost < 1 )
        return FALSE;

    for ( counter = 0; counter < MAX_TRADE; counter++ )
    {
        if ( GET_ITEM_TYPE(item) == shop_index[shop_nr].type[counter] )
            return TRUE;
    }

    return FALSE;
}


int unlimited_supply (struct obj_data *item, int shop_nr )
{
    struct obj_data *obj;

    for ( obj = shop_index[shop_nr].inventory; obj; obj = obj->next_content ) 
    {
        if ( item->item_number == obj->item_number )
            return TRUE;
    }

    return FALSE;
}

void restock_keeper (CHAR_DATA *keeper, int shop_nr)
{
  struct obj_data *obj, *obj2;
  char buf[50];
    
  sprintf(buf, "Restocking shop keeper: %d", shop_nr);
  log(buf, OVERSEER, LOG_MISC);

  for(obj = shop_index[shop_nr].inventory; obj; obj = obj->next_content) {
    obj2 = clone_object (obj->item_number);
    obj_to_char(obj2, keeper);
  }
}


/*
 * Buy an item from a shop.
 */
void shopping_buy(const char *arg, CHAR_DATA *ch,
     CHAR_DATA *keeper, int shop_nr )
{
    char buf[MAX_STRING_LENGTH];
    char argm[MAX_INPUT_LENGTH+1];
    struct obj_data *obj;
    uint32 cost;

    if ( !is_ok( keeper, ch, shop_nr ) )
        return;


    one_argument( arg, argm );
    if ( *argm == '\0' )
    {
        sprintf( buf, "%s What do you want to buy?", GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return;
    }


    if ( ( obj = get_obj_in_list_vis( ch, argm, keeper->carrying ) ) == NULL )
    {
        sprintf( buf, shop_index[shop_nr].no_such_item1, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
        send_to_char("The shop keeper changes his mind and refuses to sell such a special item.\r\n", ch);
        return;
    }

    if ( obj->obj_flags.cost <= 0 )
    {
        extract_obj( obj );
        sprintf( buf, shop_index[shop_nr].no_such_item1, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    cost = (int) (obj->obj_flags.cost * shop_index[shop_nr].profit_buy);

    if(cost < 1)
      cost = 1;

    if ( GET_GOLD(ch) < cost )
    {
        sprintf( buf, shop_index[shop_nr].missing_cash2, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }
    
    if ( IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch) )
    {
        send_to_char( "You can't carry that many items.\n\r", ch );
        return;
    }

    if ( IS_CARRYING_W(ch) + obj->obj_flags.weight > CAN_CARRY_W(ch) )
    {
        send_to_char( "You can't carry that much weight.\n\r", ch );
        return;
    }

    if(IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE)) {
      if(search_char_for_item(ch, obj->item_number, false)) {
         send_to_char("The item's uniqueness prevents it!\r\n", ch);
         return;
      }
    }

    act( "$n buys $p.", ch, obj, 0, TO_ROOM , 0);
    sprintf( buf, shop_index[shop_nr].message_buy, GET_NAME(ch), cost );
    do_tell( keeper, buf, 0 );
    sprintf( buf, "You now have %s.\n\r", obj->short_description );
    send_to_char( buf, ch );
    GET_GOLD(ch)     -= cost;
    GET_GOLD(keeper) += cost;

    if(GET_GOLD(keeper) > 3000000)
       GET_GOLD(keeper) = 3000000;

    // Wormhole to map_eq_level
    /*
    if( obj->obj_flags.eq_level == 1000 ) 
         obj = clone_object(obj->item_number);
    else
        obj_from_char( obj );
    */ 

    if (unlimited_supply (obj, shop_nr))
      obj = clone_object(obj->item_number);
    else
      obj_from_char ( obj );

    obj_to_char( obj, ch );
    do_save(ch, "", 666);

    return; 
}



/*
 * Sell an item to a shop keeper.
 */
void shopping_sell(const char *arg, CHAR_DATA *ch,
     CHAR_DATA *keeper, int shop_nr )
{
    char buf[MAX_STRING_LENGTH];
    char argm[MAX_INPUT_LENGTH+1];
    struct obj_data *obj;
    uint32 cost;

    if ( !is_ok( keeper, ch, shop_nr ) )
        return;

    one_argument( arg, argm );

    if ( *argm == '\0' )
    {
        sprintf( buf, "%s What do you want to sell?", GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return;
    }

    if ( ( obj = get_obj_in_list_vis( ch, argm, ch->carrying ) ) == NULL )
    {
        sprintf( buf, shop_index[shop_nr].no_such_item2, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE)) { 
        send_to_char("It seems magically attached to you.\r\n", ch);
        return;
    }

    if(contains_no_trade_item(obj)) {
      if(GET_LEVEL(ch) > IMMORTAL)
         send_to_char("That was a NO_TRADE item btw....\r\n", ch);
      else {
        send_to_char("Something inside it seems magically attached to you.\r\n", ch);
        return;
      }
    } 

    if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
        send_to_char("That would be really fucking smart.\r\n", ch);
        return;
    }

    if ( !trade_with( obj, shop_nr ) || obj->obj_flags.cost < 1 )
    {
        sprintf( buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    int virt = obj_index[obj->item_number].virt;
    if (virt >= 13400 && virt <= 13707 &&
	  mob_index[keeper->mobdata->nr].virt != 13416)
    {
	sprintf(buf, "%s There is only one merchant in the land that deals with such fine jewels.", GET_NAME(ch));
	do_tell(keeper,buf,0);
        return;
    }

    // don't allow non-empty containers to be sold
    if (obj->obj_flags.type_flag == ITEM_CONTAINER && obj->contains)
    {
        sprintf(buf, "%s %s$B$2 needs to be emptied first.", GET_NAME(ch), GET_OBJ_SHORT(obj));
        do_tell(keeper,buf,0);     
        return;
    }

    cost = (int) ( obj->obj_flags.cost * shop_index[shop_nr].profit_sell );
    if ( GET_GOLD(keeper) < cost )
    {
        sprintf( buf, shop_index[shop_nr].missing_cash1, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    act( "$n sells $p.", ch, obj, 0, TO_ROOM , 0);
    sprintf( buf, shop_index[shop_nr].message_sell, GET_NAME(ch), cost );
    do_tell( keeper, buf, 0 );
    sprintf( buf, "The shopkeeper now has %s.\n\r", obj->short_description );
    send_to_char( buf,ch );
    GET_GOLD(ch)     += cost;
    GET_GOLD(keeper) -= cost;

    strcpy(argm, obj->name);

    if ( get_obj_in_list( argm, keeper->carrying )
    || GET_ITEM_TYPE(obj) == ITEM_TRASH || unlimited_supply (obj, shop_nr))
    {
        extract_obj( obj );
    }
    else
      move_obj( obj, keeper );

    do_save(ch, "", 666);
     
    return;
}



/*
 * Value an item.
 */
void shopping_value(const char *arg, CHAR_DATA *ch, 
    CHAR_DATA *keeper, int shop_nr )
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    char argm[MAX_INPUT_LENGTH+1];
    struct obj_data *obj;
    int cost;
    bool keeperhas = FALSE;

    if ( !is_ok( keeper, ch, shop_nr ) )
        return;

    one_argument( arg, argm );

    if ( *argm == '\0' )
    {
        sprintf( buf, "%s What do you want to value?", GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    if ( ( obj = get_obj_in_list_vis( ch, argm, ch->carrying ) ) == NULL )
    {
       if ( ( obj = get_obj_in_list_vis( keeper, argm, keeper->carrying ) ) != NULL )
          keeperhas = TRUE;
       else {
          sprintf( buf, shop_index[shop_nr].no_such_item2, GET_NAME(ch) );
          do_tell( keeper, buf, 0 );
          return;
       }
    }

    if(mob_index[keeper->mobdata->nr].virt == 3003) { //if the weaponsmith in town
       if(keeperhas) {
          act("The Weaponsmith holds up his $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
          act("The Weaponsmith holds up his $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
       } else {
          act("You hold up $p for the Weaponsmith to examine.", ch, obj, 0, TO_CHAR, 0);
          act("$n holds up $p for the Weaponsmith to examine.", ch, obj, 0, TO_ROOM, 0);
          do_emote(keeper, "looks carefully at the item.", 9);
       }
       if(GET_ITEM_TYPE(obj) == ITEM_WEAPON) {
          if(obj->obj_flags.eq_level < 20) {
             sprintf(buf, "Well, %s is able to be used by ", obj->short_description);
             sprintbit(obj->obj_flags.size, size_bits, buf2);
             strcat(buf, buf2);
             do_say(keeper, buf, 9);
             sprintf(buf, "and it can be wielded by these classes: ");
             sprintbit(obj->obj_flags.extra_flags, extra_bits, buf2);
             strcat(buf, buf2);
             do_say(keeper, buf, 9);
             sprintf(buf, "The minimum level necessary to use it is %d.", obj->obj_flags.eq_level);
             do_say(keeper, buf, 9);
             sprintf(buf, "The damage dice are '%dD%d'", obj->obj_flags.value[1], obj->obj_flags.value[2]);
             do_say(keeper, buf, 9);
             for(int i=0;i<obj->num_affects;i++) {
                if(obj->affected[i].location == APPLY_HITROLL && obj->affected[i].modifier != 0) {
                   sprintf(buf, "It increases your hit roll by %d.", obj->affected[i].modifier);
                   do_say(keeper, buf, 9);
                }
                if(obj->affected[i].location == APPLY_DAMROLL && obj->affected[i].modifier != 0) {
                   sprintf(buf, "It increases your damage by %d.", obj->affected[i].modifier);
                   do_say(keeper, buf, 9);
                }
             }
          }
          else
             do_say(keeper, "This weapon is unknown to me.", 9); 
       }
       else
          do_say(keeper, "I'm a weapons expert, that is all.", 9);
    }
    if(mob_index[keeper->mobdata->nr].virt == 3004) { //if the armourer in town
       if(keeperhas) {
          act("The Armourer holds up his $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
          act("The Armourer holds up his $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
       } else {
          act("You hold up $p for the Armourer to examine.", ch, obj, 0, TO_CHAR, 0);
          act("$n holds up $p to the Armourer to examine.", ch, obj, 0, TO_ROOM, 0);
          do_emote(keeper, "looks carefully at the item.", 9);
       }
       if(GET_ITEM_TYPE(obj) == ITEM_ARMOR) {
          if(obj->obj_flags.eq_level < 20) {
             sprintf(buf, "Ah yes, %s can be worn by ", obj->short_description);
             sprintbit(obj->obj_flags.size, size_bits, buf2);
             strcat(buf, buf2);
             do_say(keeper, buf, 9);
             sprintf(buf, "and it can be worn by these classes: ");
             sprintbit(obj->obj_flags.extra_flags, extra_bits, buf2);
             strcat(buf, buf2);
             do_say(keeper, buf, 9); 
             sprintf(buf, "The minimum level necessary to use it is %d.", obj->obj_flags.eq_level);
             do_say(keeper, buf, 9);
             for(int i=0;i<obj->num_affects;i++) {
                if(obj->affected[i].location == APPLY_AC && obj->affected[i].modifier != 0) {
                   sprintf(buf, "Your armor class will change by %d.", obj->affected[i].modifier);
                   do_say(keeper, buf, 9);
                   if(obj->affected[i].modifier < 0)
                      do_say(keeper, "Don't worry, this is a good thing.", 9);
                }
             }
          }
          else
             do_say(keeper, "This armor is crafted using too advanced techniques for me.", 9); 
       }
       else
          do_say(keeper, "I deal with armor exclusively.", 9);
    }
    if(mob_index[keeper->mobdata->nr].virt == 3000) { //if the wizard in town
       if(keeperhas) {
          act("The Wizard holds up $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
          act("The Wizard holds up $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
       } else {
          act("You hold up $p for the Wizard to examine.", ch, obj, 0, TO_CHAR, 0);
          act("$n holds up $p for the Wizard to examine.", ch, obj, 0, TO_ROOM, 0);
          do_emote(keeper, "looks carefully at the item.", 9);
       }
       if(GET_ITEM_TYPE(obj) == ITEM_SCROLL || GET_ITEM_TYPE(obj) == ITEM_WAND || GET_ITEM_TYPE(obj) == ITEM_POTION || GET_ITEM_TYPE(obj) == ITEM_STAFF) {
          if(obj->obj_flags.value[0] < 20) {
             sprintf(buf, "Excellent, %s has been imbued with energies of the %dth level.", obj->short_description, obj->obj_flags.value[0]);
             do_say(keeper, buf, 9);
             if(GET_ITEM_TYPE(obj) == ITEM_WAND || GET_ITEM_TYPE(obj) == ITEM_STAFF) {
                if(obj->obj_flags.value[3] >= 1) {
                   sprintf(buf, "It is eminating the aura of ");
                   sprinttype(obj->obj_flags.value[3]-1, spells, buf2);
                   strcat(buf, buf2);
                   do_say(keeper, buf, 9);
                }
                if(obj->obj_flags.value[1] == obj->obj_flags.value[2])
                   do_say(keeper, "It's fully charged as well.", 9);
                else if(obj->obj_flags.value[2] == 0)
                   do_say(keeper, "Though unfortunately, there are no more charges left.", 9);
                else
                   do_say(keeper, "It looks like it has been used some.", 9);
             }
             else {
                if(obj->obj_flags.value[1] >= 1) {
                   sprintf(buf, "I can easily identify the signatures of ");
                   sprinttype(obj->obj_flags.value[1]-1, spells, buf2);
                   strcat(buf, buf2);
                   do_say(keeper, buf, 9);
                }
                if(obj->obj_flags.value[2] >= 1) {
                   do_say(keeper, "There are more enchantments held within, but I'm rather busy.", 9);
                }
             }
          }
          else
             do_say(keeper, "This item contains magics too powerful for me to discern.", 9); 
       }
       else
          do_say(keeper, "I only know the properties of scrolls, potions, staves, and wands.", 9);
    }

    if(mob_index[keeper->mobdata->nr].virt == 3010 && keeperhas) { //if the leather worker in town
       act("The Leather Worker holds up $p for you to examine.", ch, obj, 0, TO_CHAR, 0);
       act("The Leather Worker holds up $p for $n to examine.", ch, obj, 0, TO_ROOM, 0);
       if(GET_ITEM_TYPE(obj) == ITEM_ARMOR) {
          if(obj->obj_flags.eq_level < 20) {
             sprintf(buf, "Ah yes, %s can be worn by ", obj->short_description);
             sprintbit(obj->obj_flags.size, size_bits, buf2);
             strcat(buf, buf2);
             do_say(keeper, buf, 9);
             sprintf(buf, "and it can be worn by these classes: ");
             sprintbit(obj->obj_flags.extra_flags, extra_bits, buf2);
             strcat(buf, buf2);
             do_say(keeper, buf, 9);
             sprintf(buf, "The minimum level necessary to use it is %d.", obj->obj_flags.eq_level);
             do_say(keeper, buf, 9);
             for(int i=0;i<obj->num_affects;i++) {
                if(obj->affected[i].location == APPLY_AC && obj->affected[i].modifier != 0) {
                   sprintf(buf, "Your armor class will change by %d.", obj->affected[i].modifier);
                   do_say(keeper, buf, 9);
                   if(obj->affected[i].modifier < 0)
                      do_say(keeper, "Don't worry, this is a good thing.", 9);
                }
             }
          }
          else
             do_say(keeper, "This armor is crafted using too advanced techniques for me.", 9);
       } else do_say(keeper, "I don't know anything about this item, actually.", 9);
    }

    if ( !trade_with( obj, shop_nr ) || obj->obj_flags.cost < 1 )
    {
        sprintf( buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }
    if(!keeperhas) {
       cost = (int) ( obj->obj_flags.cost * shop_index[shop_nr].profit_sell );
       sprintf( buf, "%s I'll give you %d gold coins for that.", GET_NAME(ch), cost );
       do_tell( keeper, buf, 0 );
    }
    return;
}



/*
 * List available items.
 */
void shopping_list(const char *arg, CHAR_DATA *ch,
     CHAR_DATA *keeper, int shop_nr )
{
    char buf[MAX_STRING_LENGTH];
    struct obj_data *obj,*tobj;
    int cost;
    //    extern char *drinks[];
    int found;
    int done[100]; // To show 'em numbered instead of a long list of duplicates
    int i,a;
    if ( !is_ok( keeper, ch, shop_nr ) )
        return;

    if (!keeper->carrying && shop_index[shop_nr].inventory)
    {
        sprintf(buf, "%s Oops, I seem to be out of inventory.", GET_NAME(ch));
        do_tell (keeper, buf, 0);
        sprintf(buf, "%s One minute while I restock", GET_NAME(ch));
        do_tell (keeper, buf, 0);
        restock_keeper (keeper, shop_nr);
    }
    i = 0;
    send_to_char( "[Amt] [ Price ] Item\n\r", ch );
    found = FALSE;
    for ( obj = keeper->carrying; obj; obj = obj->next_content )
    {
        if ( !CAN_SEE_OBJ( ch, obj ) || obj->obj_flags.cost <= 0 )
            continue;

        found = TRUE;

        cost = (int) ( obj->obj_flags.cost * shop_index[shop_nr].profit_buy );

        int vnum = obj_index[obj->item_number].virt;
	bool loop = FALSE;
        for (a = 0; a < i; a++)
           if (done[a] == vnum)
              loop = TRUE;
        if (loop) continue;
	if (i < 100) 
	done[i++] = obj_index[obj->item_number].virt;
	else break;
        a = 0;
	for (tobj = keeper->carrying; tobj; tobj = tobj->next_content)
          if (obj_index[tobj->item_number].virt == obj_index[obj->item_number].virt)
	    a++;
/*        if ( GET_ITEM_TYPE(obj) == ITEM_DRINKCON && obj->obj_flags.value[1] )
        {
            sprintf( buf, "[%3d] [%7d] %s of %s.\n\r",
                a, cost, obj->short_description,
                drinks[obj->obj_flags.value[2]] );
        }
        else
        {*/
            sprintf( buf, "[%3d] [%7d] %s.\n\r",
                a, cost, obj->short_description );
//        }
        send_to_char(buf, ch);
    }

    if ( !found )
        send_to_char( "You can't buy anything here!\n\r", ch );

    return;
}



// Spec proc for shop keepers.
// TODO - Remove goto's from this....I hate goto's.  This is C, not BASIC....
int shop_keeper( CHAR_DATA *ch, struct obj_data *obj, int cmd, const char *arg, CHAR_DATA * invoker )
{
    CHAR_DATA *keeper;
    int shop_nr;

    /*
     * Find a shop keeper in the room.
     */
//    for ( keeper = world[ch->in_room].people;
//        keeper != NULL;
//        keeper = keeper->next_in_room )
//    {
//        if ( IS_MOB(keeper) && mob_index[keeper->mobdata->nr].non_combat_func == shop_keeper )
//            goto LFound1;
//    }

// This (using original log from above stuff) should take care of finding the keeper
// instead of looping through.  Should allow for multiple keepers too:)
  if(!(keeper = invoker)) {
    log( "Shop_keeper: keeper not found.", ANGEL, LOG_BUG );
    return eFAILURE;
  }

// LFound1:
    for ( shop_nr = 0; shop_nr < max_shop; shop_nr++ )
    {
        if ( shop_index[shop_nr].keeper == keeper->mobdata->nr )
            goto LFound2;
    }
    log( "Shop_keeper: shop_nr not found.", ANGEL, LOG_BUG );
    return eFAILURE;

 LFound2:
//    if ( ch->in_room != shop_index[shop_nr].in_room )
  //      return eFAILURE;

    switch ( cmd )
    {
    default: return eFAILURE;
    case 56: shopping_buy   ( arg, ch, keeper, shop_nr ); break;
    case 57: shopping_sell  ( arg, ch, keeper, shop_nr ); break;
    case 58: shopping_value ( arg, ch, keeper, shop_nr ); break;
    case 59: shopping_list  ( arg, ch, keeper, shop_nr ); break;
    }

    return eSUCCESS;
}


void free_shops_from_memory()
{
  for(int i = 0; i < MAX_SHOP; i++)
  {
    if(shop_index[i].no_such_item1)
      dc_free(shop_index[i].no_such_item1);
    if(shop_index[i].no_such_item2)
      dc_free(shop_index[i].no_such_item2);
    if(shop_index[i].do_not_buy)
      dc_free(shop_index[i].do_not_buy);
    if(shop_index[i].missing_cash1)
      dc_free(shop_index[i].missing_cash1);
    if(shop_index[i].missing_cash2)
      dc_free(shop_index[i].missing_cash2);
    if(shop_index[i].message_buy)
      dc_free(shop_index[i].message_buy);
    if(shop_index[i].message_sell)
      dc_free(shop_index[i].message_sell);
  } 
}


void boot_the_shops()
{
    char *buf;
    int temp;
    int count;
    FILE *fp;

    if ( ( fp = dc_fopen( SHOP_FILE, "r" ) ) == NULL )
    {
        perror( SHOP_FILE );
        exit( 1 );
    }

    max_shop = 0;

    for ( ;; )
    {
        buf = fread_string( fp , 0);
        if ( *buf == '$' )
        {
            dc_free(buf);
            break;
        }
        if ( *buf != '#' )
        {
            dc_free(buf);
            continue;
        }

        // we don't seem to use buff after this point, so I'm going to free it
        // otherise, we're leaking memory
        dc_free(buf);

        if ( max_shop >= MAX_SHOP )
        {
            perror( "Too many shops.\n" );
            exit( 1 );
        }

        /*
         * Ignore "producing" list.
         */
        for ( count = 0; count < 6; count++ )
            fscanf( fp, "%d \n", &temp );

        fscanf( fp, "%f \n", &shop_index[max_shop].profit_buy_base  );
        fscanf( fp, "%f \n", &shop_index[max_shop].profit_sell );
        for( count = 0; count < MAX_TRADE; count++ )
        {
            fscanf( fp, "%d \n", &shop_index[max_shop].type[count] );
        }

        shop_index[max_shop].profit_buy = shop_index[max_shop].profit_buy_base;
        shop_index[max_shop].no_such_item1      = fread_string( fp , 0);
        shop_index[max_shop].no_such_item2      = fread_string( fp, 0 );
        shop_index[max_shop].do_not_buy         = fread_string( fp , 0);
        shop_index[max_shop].missing_cash1      = fread_string( fp, 0 );
        shop_index[max_shop].missing_cash2      = fread_string( fp , 0);
        shop_index[max_shop].message_buy        = fread_string( fp , 0);
        shop_index[max_shop].message_sell       = fread_string( fp, 0 );

        fscanf( fp, "%d \n", &temp );           /* Temper       */
        fscanf( fp, "%d \n", &temp );           /* Temper       */

        fscanf( fp, "%d \n", &temp );
        shop_index[max_shop].keeper     = real_mobile( temp );

        fscanf( fp, "%d \n", &temp );           /* With_whom    */

        fscanf( fp, "%d \n", &temp );

        int room_nr = real_room( temp );
        if (room_nr < 0 || room_nr > top_of_world)
        {
          logf(100, LOG_BUG, "shopkeeper %d loaded with in_room set to %d. Setting to 0.", max_shop, room_nr);
          room_nr = 0;
        }

        shop_index[max_shop].in_room = room_nr;

        fscanf( fp, "%d \n", &shop_index[max_shop].open1   );
        fscanf( fp, "%d \n", &shop_index[max_shop].close1  );
        fscanf( fp, "%d \n", &shop_index[max_shop].open2   );
        fscanf( fp, "%d \n", &shop_index[max_shop].close2  );
		
		  shop_index[max_shop].inventory = 0;	

	if(real_room( temp ) == NOWHERE)
	{
	  sprintf(log_buf, "BAD SHOP IN ROOM %d -- FIX THIS!", temp);
	  log(log_buf, 0, LOG_MISC);
          /* Free the memory from bad shops */
          dc_free(shop_index[max_shop].no_such_item1);
          dc_free(shop_index[max_shop].no_such_item2);
          dc_free(shop_index[max_shop].do_not_buy);
          dc_free(shop_index[max_shop].missing_cash1);
          dc_free(shop_index[max_shop].missing_cash2);
          dc_free(shop_index[max_shop].message_buy);
          dc_free(shop_index[max_shop].message_sell);

	  continue;
	  /* This way we don't increment if it was bad */
	}
        max_shop++;
    }

    dc_fclose( fp );

}


void assign_the_shopkeepers( )
{
    int shop_nr;

    for ( shop_nr = 0; shop_nr < max_shop; shop_nr++ )
        mob_index[shop_index[shop_nr].keeper].non_combat_func = shop_keeper;

    return;
}


void fix_shopkeepers_inventory( )
{
  int shop_nr;

  CHAR_DATA* keeper = 0;
  struct obj_data *obj, *last_obj, *cloned;

  // set up the unlimited supply items. Those the shop_keeper has on start up.

  for ( shop_nr = 0; shop_nr < max_shop; shop_nr++ )
    for (keeper= world[shop_index[shop_nr].in_room].people; keeper != NULL;
	 keeper = keeper->next_in_room )
    {
      if(IS_MOB(keeper) && mob_index[keeper->mobdata->nr].non_combat_func == shop_keeper) {
        if(keeper->carrying) {
          last_obj = clone_object(keeper->carrying->item_number);
          shop_index [shop_nr].inventory = last_obj;
          for(obj = keeper->carrying->next_content; obj;
	      obj = obj->next_content)
	  {
            cloned = clone_object(obj->item_number);
            last_obj->next_content = cloned;
            last_obj = cloned;
          }
        }
        else
          shop_index[shop_nr].inventory = 0;
      }
    }

  return;
}

// return NULL for failure
// return pointer to new shop on success
player_shop * read_one_player_shop(FILE *fp)
{
  long count;
  char code[4];

  player_shop_item * item = NULL;
  player_shop * shop = (player_shop *)dc_alloc(1, sizeof(player_shop));

  fread(&shop->owner, sizeof(char), PC_SHOP_OWNER_SIZE, fp);
  fread(&shop->room_num, sizeof(int32), 1, fp);
  fread(&shop->sell_message, sizeof(char), PC_SHOP_SELL_MESS_SIZE, fp);
  fread(&shop->money_on_hand, sizeof(int32), 1, fp);

  code[3] = '\0';
  fread(&code, sizeof(char), 3, fp);

  while(strcmp(code, "END"))
  {
    // add future stuff here

    logf(IMMORTAL, LOG_BUG, "Illegal code in player shop %s", shop->owner);
    logf(IMMORTAL, LOG_WORLD, "Illegal code in player shop %s", shop->owner);
    exit(1);
  }

  fread(&count, sizeof(int32), 1, fp);

  shop->sale_list = NULL;
  for(int i = 0; i < count; i++)
  {
    item = (player_shop_item *)dc_alloc(1, sizeof(player_shop_item));
    
    fread(&item->item_vnum, sizeof(int), 1, fp);
    fread(&item->price, sizeof(int), 1, fp);
    fread(&code, sizeof(char), 3, fp);
    // code junk right now.  Add future stuff before it if needed
    item->next = shop->sale_list;
    shop->sale_list = item;
  }

  return shop;
}

// on failure, warn player if online, log, then keep going
// assumes valid shop
void write_one_player_shop(player_shop * shop)
{
  FILE * fp;
  player_shop_item * item;
  char buf[80];
  long count = 0;
  
  sprintf(buf, "%s/%s", PLAYER_SHOP_DIR, shop->owner);

  if ( ( fp = dc_fopen( buf, "w" ) ) == NULL )
  {
    logf(IMMORTAL, LOG_WORLD, "Could not open %s for writing.", buf);
    return;
  }

  fwrite(&(shop->owner), sizeof(char), PC_SHOP_OWNER_SIZE, fp);
  fwrite(&(shop->room_num), sizeof(int32), 1, fp);
  fwrite(&(shop->sell_message), sizeof(char), PC_SHOP_SELL_MESS_SIZE, fp);
  fwrite(&(shop->money_on_hand), sizeof(int32), 1, fp);

  // add stuff later here with 3 digit code
  // end of variable data
  fwrite("END", sizeof(char), 3, fp);

  for(item = shop->sale_list; item; item = item->next)
    count++;

  fwrite(&(count), sizeof(int32), 1, fp);

  for(item = shop->sale_list; item; item = item->next)
  {
     fwrite(&(item->item_vnum), sizeof(int), 1, fp);
     fwrite(&(item->price), sizeof(int), 1, fp);
     fwrite("END", sizeof(char), 3, fp);
  }

  dc_fclose(fp);
}

// save the list of shopfiles (not an individual shop)
// this only needs to be done when a shop is created or deleted
void save_shop_list()
{
    FILE * fp;

    if ( ( fp = dc_fopen( PLAYER_SHOP_INDEX, "w" ) ) == NULL )
    {
        perror( PLAYER_SHOP_INDEX );
        exit( 1 );
    }

    for(player_shop * shop = g_playershops; shop; shop = shop->next)
      fwrite_string(shop->owner, fp);

    fwrite_string("$", fp);
    dc_fclose(fp);
}

void save_player_shop_world_range()
{
  FILE *f = (FILE *)NULL;
  world_file_list_item * curr;
  char buf[180];
  
  extern world_file_list_item * world_file_list;
  
  curr = world_file_list;
  while(curr && curr->firstnum != 23000 )
    curr = curr->next;
    
  if(!curr) {
    // panic!
    logf(LOG_BUG, IMMORTAL, "Could not find player shop range to save files.");
    exit(1);
  }   
  sprintf(buf, "world/%s", curr->filename);  
  if((f = dc_fopen(buf, "w")) == NULL) {
    fprintf(stderr,"Couldn't open room save file %s for player shops.\n\r",
            curr->filename);
    return;
  }
    
  for(int x = curr->firstnum; x <= curr->lastnum; x++)
     write_one_room(f, x);
  fprintf(f, "$~\n");
  dc_fclose(f);  
}

void boot_player_shops()
{
    FILE *fp;
    FILE *shopfp;
    player_shop * shop;
    char * filename;
    char buf[80];

    g_playershops = NULL;

    if ( ( fp = dc_fopen( PLAYER_SHOP_INDEX, "r" ) ) == NULL )
    {
        perror( PLAYER_SHOP_INDEX );
        exit( 1 );
    }

    // read list of player owned shops

    filename = fread_string(fp, 0);
    while( strcmp(filename, "$") )
    {
      sprintf(buf, "%s/%s", PLAYER_SHOP_DIR, filename);
      if ( ( shopfp = dc_fopen( buf, "r" ) ) == NULL )
      {
        perror( buf );
        exit(1);
      }

      if(!(shop = read_one_player_shop(shopfp)))
      {
        perror( buf );
        exit(1);
      }
      shop->next = g_playershops;
      g_playershops = shop;

      dc_fclose( shopfp );
      filename = fread_string(fp, 0);
    }
    dc_fclose( fp );
}

player_shop * find_player_shop(char_data * keeper)
{
  player_shop * shop = g_playershops;

  for(;shop;shop = shop->next)
    if(real_room(shop->room_num) == keeper->in_room)
      break;

  return shop;
}

// put an item up for sale
void player_shopping_stock(const char * arg, char_data * ch, char_data * keeper)
{
   player_shop * shop = find_player_shop(keeper);
   if(!shop) {
      send_to_char("Invalid player shop keeper.  Let a god know.\r\n", ch);
      return;
   }

   if(strcmp(shop->owner, GET_NAME(ch))) {
      send_to_char("You don't own this shop, you can't stock the shelves!\r\n", ch);
      return;
   }

   char item[MAX_INPUT_LENGTH];
   char price[MAX_INPUT_LENGTH];

   half_chop(arg, item, price);

   if(!*item || !*price) {
      send_to_char("Syntax:  stock <item> <price>\r\n", ch);
      return;
   }

   long value;
   value = atol(price);
   if(value < 1 || value > 20000000) {
      send_to_char("Invalid price.  The price must be between 1 gold and 20 million gold.\r\n", ch);
      return;
   }

   // find item
   obj_data * obj;

   if ( ( obj = get_obj_in_list_vis( ch, item, ch->carrying ) ) == NULL )
   {
      send_to_char("Stock what item?\r\n", ch);
      return;
   }

   // make sure it isn't NO_DROP, NO_TRADE, etc
   if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) ||
      IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
      IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP) ||
      IS_SET(obj->obj_flags.extra_flags, ITEM_NOSAVE))
   {
      send_to_char("You can't give that to the shop keeper to sell!\r\n", ch);
      return;
   }

   if(IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE)) {
      send_to_char("For now you can't sell unique items.\r\n", ch);
      return;
   }

   // add it to list
   player_shop_item * newitem = (player_shop_item *)dc_alloc(1, sizeof(player_shop_item));
   newitem->item_vnum = obj_index[obj->item_number].virt;
   newitem->price = value;
   newitem->next = shop->sale_list;
   shop->sale_list = newitem;
   extract_obj(obj);
   send_to_char("You put the item up for sale.\r\n", ch);
   write_one_player_shop(shop);
}

void player_shopping_buy(const char * arg, char_data * ch, char_data * keeper)
{
   player_shop * shop = find_player_shop(keeper);
   if(!shop) {
      send_to_char("Invalid player shop keeper.  Let a god know.\r\n", ch);
      return;
   }

   char buf[MAX_INPUT_LENGTH];

   one_argument( arg, buf );
   if ( !*buf )
   {
      sprintf( buf, "%s Which do you want to buy?", GET_NAME(ch) );
      do_tell( keeper, buf, 0 );
      return;
   }

   int item_pos = atoi(buf);
   player_shop_item * item = shop->sale_list;
   for(int j = 1; (item && j < item_pos); item = item->next, j++)
      ;

   if(!item || item_pos < 1) {
      send_to_char("Choose a valid number!\r\n", ch);
      return;
   }

   // make sure they can afford it
   if(GET_GOLD(ch) < item->price) {
      send_to_char("You can't afford that!\r\n", ch);
      return;
   }

   int robj = real_object(item->item_vnum);
   if(robj < 0) {
      send_to_char("Error, that is not a valid item.  Let a god know.\r\n", ch);
      return;
   }

   // give it to them, thank them, take the money
   obj_data * obj = clone_object(robj);
   obj_to_char(obj, ch);
   GET_GOLD(ch) -= item->price;
   shop->money_on_hand += item->price;

   if(*shop->sell_message)
      sprintf(buf, "%s %s", GET_NAME(ch), shop->sell_message);
   else sprintf(buf, "%s Thank you, come again!", GET_NAME(ch));
   do_tell(keeper, buf, 9);

   // update inventory
   for(player_shop_item * curr = shop->sale_list; curr; curr = curr->next) {
      if(curr == item) { // first item
        shop->sale_list = curr->next;
        dc_free(curr);
        break;
      }
      if(curr->next == item) {
        curr->next = item->next;
        dc_free(item);
        break;
      }
   }

   // save the shop
   write_one_player_shop(shop);
}

void player_shopping_withdraw(const char * arg, char_data * ch, char_data * keeper)
{
  player_shop * shop = find_player_shop(keeper);
  if(!shop) {
    send_to_char("Invalid player shop keeper.  Let a god know.\r\n", ch);
    return;
  }
  
  if(strcmp(shop->owner, GET_NAME(ch))) {
    send_to_char("You don't own this shop!  Go rob a bank or something.\r\n", ch);
    return;
  }

  char price[MAX_INPUT_LENGTH];
  one_argument(arg, price);

  if(!*price) {
    send_to_char("Withdraw how much from your store?\r\n", ch);
    return;
  }

  long value;
  value = atol(price);
  if(value < 1 || value > 20000000) {
    send_to_char("Invalid amount.  The amount must be between 1 gold and 20 million gold.\r\n", ch);
    return;
  }

  if(value > shop->money_on_hand) {
    send_to_char("You don't have that much in the till!\r\n", ch);
    return;
  }  

  shop->money_on_hand -= value;
  GET_GOLD(ch) += value;
  csendf(ch, "You take %ld gold out of the till.\r\n", value);
  write_one_player_shop(shop);
}

void player_shopping_design(const char * arg, char_data * ch, char_data * keeper)
{
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  int16 skill;

  if(IS_NPC(ch))   
    return;

  const char * pdesign_values [] = {
    "sellmessage",
    "roomname",
    "roomdesc",
    "\n"
  };

  player_shop * shop = find_player_shop(keeper);
  if(!shop) {
    send_to_char("Invalid player shop keeper.  Let a god know.\r\n", ch);
    return;
  }

  if(strcmp(shop->owner, GET_NAME(ch))) {
    send_to_char("You don't own this shop, you can't change the design!\r\n", ch);
    return;
  }

  half_chop(arg, select, text);
  
  if(!(*select)) {
    send_to_char("$3Usage$R: design <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n"
                 , ch);
    display_string_list(pdesign_values, ch);
    return;
  }

  for(skill = 0 ;; skill++)
  {
    if(pdesign_values[skill][0] == '\n')
    {
      send_to_char("Invalid field.\n\r", ch);
      return;
    }
    if(is_abbrev(select, pdesign_values[skill]))
      break;
  }

  switch(skill) {
    case 0: // sellmessage
      if(!*text) {
         send_to_char("$3Syntax$R: design sellmessage <message>\r\n"
                      "  Put 'none' if you want no special message.\r\n", ch);
         return;
      }
      if(strlen(text) > (PC_SHOP_SELL_MESS_SIZE - 20)) {
         send_to_char("That sell message is too long.\r\n", ch);
         return;
      }
      if(!strcmp(text, "none"))
        *shop->sell_message = '\0';
      else strcpy(shop->sell_message, text);
      csendf(ch, "Shop sell message changed to '%s'.\r\n", shop->sell_message);
      write_one_player_shop(shop); // save it
      break;

    case 1: // roomname
      if(!*text) {
         send_to_char("$3Syntax$R: design roomname <name>\r\n", ch);
         return;
      }
      if(strlen(text) > 60) {
         send_to_char("That room name is too long (60 chars max).\r\n", ch);
         return;
      }
      dc_free(world[shop->room_num].name);
      world[shop->room_num].name = str_dup(text);
      csendf(ch, "Room name set to '%s'.\r\n", world[shop->room_num].name);
      save_player_shop_world_range();
      break;

    case 2: // roomdesc
      send_to_char("Not active yet.\r\n", ch);
      break;

    default:
      send_to_char("$3Usage$R: design <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n"
                 , ch);
      display_string_list(pdesign_values, ch);
      break;
  }
}

void player_shopping_sell(const char * arg, char_data * ch, char_data * keeper)
{
  send_to_char("These shop keeper's don't buy stuff.\r\n", ch);
}

void player_shopping_value(const char * arg, char_data * ch, char_data * keeper)
{
  send_to_char("These shop keeper's don't buy stuff.\r\n", ch);
}

void player_shopping_list(const char * arg, char_data * ch, char_data * keeper)
{
   int count = 0;
   int robj;
   player_shop * shop = find_player_shop(keeper);
   if(!shop) {
      send_to_char("Invalid player shop keeper.  Let a god know.\r\n", ch);
      return;
   }
   send_to_char("Item                                          Price\r\n"
                "------------------------------------------------------\r\n", ch);
   
   if(!shop->sale_list)
      send_to_char("There is nothing for sale here :(\r\n", ch);

   else for(player_shop_item * item = shop->sale_list; item; item = item->next)
   {
     count++;
     robj = real_object(item->item_vnum);
     if(robj < 0)
       csendf(ch, "%-3d$3)$R %-40s %d\r\n", count, "INVALID ITEM NUMBER", item->price);
     else csendf(ch, "%-3d$3)$R %-40s %d\r\n", count, ((obj_data *)obj_index[robj].item)->short_description, item->price);
   }

   if(!strcmp(shop->owner, GET_NAME(ch)))
     csendf(ch, "\r\nYour shop has %ld cash in the till.\r\n", shop->money_on_hand);
}

int player_shop_keeper( CHAR_DATA *ch, struct obj_data *obj, int cmd, const char *arg, CHAR_DATA * invoker )
{
    CHAR_DATA *keeper;

    if(!(keeper = invoker)) {
      log( "Shop_keeper: keeper not found.", ANGEL, LOG_BUG );
      return eFAILURE;
    }

    if(IS_MOB(ch))
      return eFAILURE;

    switch ( cmd )
    {
      case 174: player_shopping_withdraw(arg, ch, keeper ); break;
      case 62: player_shopping_design( arg, ch, keeper ); break;
      case 61: player_shopping_stock ( arg, ch, keeper ); break;
      case 56: player_shopping_buy   ( arg, ch, keeper ); break;
      case 57: player_shopping_sell  ( arg, ch, keeper ); break;
      case 58: player_shopping_value ( arg, ch, keeper ); break;
      case 59: player_shopping_list  ( arg, ch, keeper ); break;
      default: return eFAILURE;
    }

    return eSUCCESS;
}
/*
int do_pshopedit(char_data * ch, char * arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  int16 skill, i;
  player_shop * shop;

  if(IS_NPC(ch))   
    return eFAILURE;

  if(!has_skill(ch, COMMAND_PSHOPEDIT)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }

  char * pshopedit_values [] = {
    "new",
    "delete",
    "list",
    "\n"
  };

  arg = one_argumentnolow(arg, select);
  
  if(!(*select)) {
    send_to_char("$3Usage$R: pshopedit <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n"
                 , ch);
    display_string_list(pshopedit_values, ch);
    return eFAILURE;
  }

  for(skill = 0 ;; skill++)
  {
    if(pshopedit_values[skill][0] == '\n')
    {
      send_to_char("Invalid field.\n\r", ch);
      return eFAILURE;
    }
    if(is_abbrev(select, pshopedit_values[skill]))
      break;
  }

  switch(skill) {
    case 0: // new
      half_chop(arg, buf, text);
      if(!*buf || !*text) {
         send_to_char("$3Syntax$R: pshopedit new <Playername> <roomnum>\r\n"
                      "  Make sure that your CAPITALIZE the player name or\r\n"
                      "  they won't be able to use it.\r\n", ch);
         return eFAILURE;
      }
      i = atoi(text);
      if(i < 1 || i > top_of_world || !world_array[i]) {
         send_to_char("You must choose a valid room number.\r\n", ch);
         return eFAILURE;
      }
      shop = (player_shop *)dc_alloc(1, sizeof(player_shop));
      strcpy(shop->owner, buf);
      *shop->sell_message = '\0';
      shop->room_num = i;
      shop->money_on_hand = 0;
      shop->sale_list = NULL;
      shop->next = g_playershops;
      g_playershops = shop;
      save_shop_list();
      write_one_player_shop(shop);
      csendf(ch, "Shop created for player '%s' in room %d.\r\n", shop->owner, shop->room_num);
      break;

    case 1: // delete
      
      save_shop_list();
      break;

    case 2: // list
      send_to_char("Player Shops\r\n-------------------------\r\n", ch);
      if(!g_playershops)
         send_to_char("No current shops.\r\n", ch);
      else for(i = 1, shop = g_playershops; shop; shop = shop->next, i++)
        csendf(ch, "%2d$3)$R %-15s $3Room$R: %5d\r\n", i, shop->owner, shop->room_num);
      break;

    default:
      send_to_char("$3Usage$R: pshopedit <field> <correct arguments>\r\n"
                 "Fields are the following.\r\n"
                 , ch);
      display_string_list(pshopedit_values, ch);
      break;
  }

  return eSUCCESS;
}
*/
void assign_the_player_shopkeepers( )
{
   mob_index[real_mobile(PLAYER_SHOP_KEEPER)].non_combat_func = player_shop_keeper;
}

void redo_shop_profit()
{
  switch(number(0,3)) {
    case 0:
      break;
    case 1:
      for(int i = 0; i < 58; i++) {
        shop_index[i].profit_buy = shop_index[i].profit_buy_base;
      }
      break;
    case 2:
      for(int i = 0; i < 58; i++)
        shop_index[i].profit_buy *= 1.0 + number(10, 50) / 100.0;
      break;
    case 3:
      for(int i = 0; i < 58; i++) {
        shop_index[i].profit_buy *= 1.0 - number(10, 50) / 100.0;
        shop_index[i].profit_buy = MAX(shop_index[i].profit_buy, shop_index[i].profit_sell + 0.1);
      }
      break;
    default:
      break;
  }
}

struct obj_exchange
{
  int item_qty;
  int item_vnum;
  int cost_qty;
  int cost_vnum;
  uint64_t cost_exp;
};

const int OBJ_APOCALYPSE = 27905;
const int OBJ_BROWNIE = 27906;
const int OBJ_MEATBALL = 27908;
const int OBJ_WINGDING = 27909;
const int OBJ_CLOVERLEAF = 27910;

const int MAX_EDDIE_ITEMS = 15;

obj_exchange eddie[MAX_EDDIE_ITEMS] = {
    {1, OBJ_CLOVERLEAF, 2, OBJ_WINGDING, 0},
    {1, OBJ_CLOVERLEAF, 2, OBJ_MEATBALL, 0},
    {1, OBJ_CLOVERLEAF, 2, OBJ_APOCALYPSE, 0},
    {1, OBJ_CLOVERLEAF, 0, 0, 5000000000},
    {2, OBJ_CLOVERLEAF, 1, OBJ_BROWNIE, 0},
    {1, OBJ_WINGDING, 3, OBJ_CLOVERLEAF, 0},
    {1, OBJ_WINGDING, 2, OBJ_MEATBALL, 0},
    {1, OBJ_WINGDING, 2, OBJ_APOCALYPSE, 0},
    {1, OBJ_MEATBALL, 3, OBJ_CLOVERLEAF, 0},
    {1, OBJ_MEATBALL, 2, OBJ_WINGDING, 0},
    {1, OBJ_MEATBALL, 2, OBJ_APOCALYPSE, 0},
    {1, OBJ_APOCALYPSE, 3, OBJ_CLOVERLEAF, 0},
    {1, OBJ_APOCALYPSE, 2, OBJ_WINGDING, 0},
    {1, OBJ_APOCALYPSE, 2, OBJ_MEATBALL, 0},
    {1, OBJ_BROWNIE, 10, OBJ_CLOVERLEAF, 0}

};

int eddie_shopkeeper(struct char_data *ch, struct obj_data *obj, int cmd, const char *arg, struct char_data *owner)
{
  if (cmd != CMD_LIST && cmd != CMD_BUY)
    return eFAILURE;

  if (IS_AFFECTED(ch, AFF_BLIND))
  {
    ch->send("You're too blind to do that!\r\n");
    return eFAILURE;
  }

  if (IS_NPC(ch))
    return eFAILURE;

  if (cmd == CMD_LIST)
  {
    csendf(ch, "$B$2%s tells you, 'This is what I can do for you...\n\r$R", GET_SHORT(owner));
    csendf(ch, "  | Item                              | Cost                                   |\n\r");
    csendf(ch, "--------------------------------------------------------------------------------\n\r");
    int last_vnum = 0;
    for (int i = 0; i < MAX_EDDIE_ITEMS; i++)
    {
      char buf[1024] = {};
      char item_buf[1024] = {};
      char cost_buf[1024] = {};
      if (eddie[i].item_vnum > 0)
      {
        strncpy(item_buf, ((obj_data *)obj_index[real_object(eddie[i].item_vnum)].item)->short_description, 1024);
      }
      else
      {
        continue;
      }

      if (eddie[i].cost_vnum > 0)
      {
        strncpy(cost_buf, ((obj_data *)obj_index[real_object(eddie[i].cost_vnum)].item)->short_description, 1024);
      }
      else if (eddie[i].cost_exp > 0)
      {
        string cost_buf_str = fmt::format(locale("en_US.UTF-8"), "{:L} experience", eddie[i].cost_exp);
        snprintf(cost_buf, 1024, "%s", cost_buf_str.c_str());
      }

      if (last_vnum != 0 && last_vnum != eddie[i].item_vnum)
      {
        csendf(ch, "--------------------------------------------------------------------------------\n\r");
      }

      last_vnum = eddie[i].item_vnum;
      int item_qty = eddie[i].item_qty;
      int cost_qty = eddie[i].cost_qty;

      // setup format specifier based length of item short descriptions
      if (cost_qty > 0)
      {
        snprintf(buf, 1024, "$B$3%%2d$R|%%2d x %%-%ds|%%2d x %%-%ds|\n\r",
                 30 + (strlen(item_buf) - nocolor_strlen(item_buf)),
                 35 + (strlen(cost_buf) - nocolor_strlen(cost_buf)));
        csendf(ch, buf, i + 1, item_qty, item_buf, cost_qty, cost_buf);
      }
      else
      {
        snprintf(buf, 1024, "$B$3%%2d$R|%%2d x %%-%ds|     %%-%ds|\n\r",
                 30 + (strlen(item_buf) - nocolor_strlen(item_buf)),
                 35 + (strlen(cost_buf) - nocolor_strlen(cost_buf)));
        csendf(ch, buf, i + 1, item_qty, item_buf, cost_buf);
      }
    }
    csendf(ch, "--------------------------------------------------------------------------------\n\r");

    /*
    send_to_char(" $B$31)$R Cloverleaf Token      Cost: 2 Wingding tokens.\n\r", ch);
    send_to_char(" $B$32)$R Cloverleaf Token      Cost: 2 Meatball tokens.\n\r", ch);
    send_to_char(" $B$33)$R Cloverleaf Token      Cost: 2 Apocalypse tokens.\n\r", ch);

    send_to_char(" $B$34)$R Wingding Token        Cost: 3 Cloverleaf tokens.\n\r", ch);
    send_to_char(" $B$35)$R Wingding Token        Cost: 2 Meatball tokens.\n\r", ch);
    send_to_char(" $B$36)$R Wingding Token        Cost: 2 Apocalypse tokens.\n\r", ch);

    send_to_char(" $B$37)$R Meatball Token        Cost: 3 Cloverleaf tokens.\n\r", ch);
    send_to_char(" $B$38)$R Meatball Token        Cost: 2 Wingding tokens.\n\r", ch);
    send_to_char(" $B$39)$R Meatball Token        Cost: 2 Apocalypse tokens.\n\r", ch);

    send_to_char("$B$310)$R Apocalypse Token      Cost: 2 Wingding tokens.\n\r", ch);
    send_to_char("$B$311)$R Apocalypse Token      Cost: 2 Meatball tokens.\n\r", ch);

    send_to_char("$B$312)$R Brownie Point         Cost: 10 Cloverleaf tokens.\n\r", ch);
    */
    return eSUCCESS;
  }
  else if (cmd == CMD_BUY)
  {
    char arg1[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    one_argument(arg, arg1);

    if (*arg1 == 0)
    {
      send_to_char("Buy what?\n\r", ch);
      return eSUCCESS;
    }

    int choice = atoi(arg1);
    if (choice < 1 || choice > MAX_EDDIE_ITEMS)
    {
      csendf(ch, "Invalid number. Choose between 1 and %d.\n\r", MAX_EDDIE_ITEMS);
      return eSUCCESS;
    }

    if ((eddie[choice - 1].cost_qty == 0 || eddie[choice - 1].cost_vnum < 1) && eddie[choice - 1].cost_exp > 0)
    {
      if (GET_EXP(ch) >= eddie[choice - 1].cost_exp)
      {
        GET_EXP(ch) -= eddie[choice - 1].cost_exp;
        ch->send(fmt::format(locale("en_US.UTF-8"), "Eddie takes {:L} experience from you, leaving you with {:L} experience.\r\n", eddie[choice - 1].cost_exp, GET_EXP(ch)));
      }
      else
      {
        ch->send(fmt::format(locale("en_US.UTF-8"), "You do not have the {:L} experience to pay for that item. You need {:L} more experience.\r\n", eddie[choice - 1].cost_exp, eddie[choice - 1].cost_exp - GET_EXP(ch)));
        return eSUCCESS;
      }
    }
    else
    {

      int count = search_char_for_item_count(ch, real_object(eddie[choice - 1].cost_vnum), false);

      if (count < eddie[choice - 1].cost_qty)
      {
        send_to_char("You don't have enough to trade.\n\r", ch);
        return eSUCCESS;
      }

      for (int i = 0; i < eddie[choice - 1].cost_qty; i++)
      {
        obj_data *obj = search_char_for_item(ch, real_object(eddie[choice - 1].cost_vnum), false);
        if (obj != 0)
        {
          if (obj->in_obj)
          {
            obj_from_obj(obj);
          }
          else
          {
            obj_from_char(obj);
          }

          act("$n gives $p to $N.", ch, obj, owner, TO_ROOM, INVIS_NULL | NOTVICT);
          act("$n gives you $p.", ch, obj, owner, TO_VICT, 0);
          act("You give $p to $N.", ch, obj, owner, TO_CHAR, 0);

          sprintf(buf, "%s gives %s to %s (removed)", GET_NAME(ch), obj->name,
                  GET_NAME(owner));
          log(buf, IMP, LOG_OBJECTS);
        }
        else
        {
          csendf(ch, "An error occured.\n\r");
          do_save(ch, "", 666);
          return eSUCCESS;
        }
      }
    }

    for (int i = 0; i < eddie[choice - 1].item_qty; i++)
    {
      obj_data *item = clone_object(real_object(eddie[choice - 1].item_vnum));
      if (item != 0)
      {
        obj_to_char(item, ch);

        act("$n gives $p to $N.", owner, item, ch, TO_ROOM, INVIS_NULL | NOTVICT);
        act("$n gives you $p.", owner, item, ch, TO_VICT, 0);
        act("You give $p to $N.", owner, item, ch, TO_CHAR, 0);

        sprintf(buf, "%s gives %s to %s (created)", GET_NAME(owner), item->name,
                GET_NAME(ch));
        log(buf, IMP, LOG_OBJECTS);
      }
      else
      {
        csendf(ch, "An error occured.\n\r");
        do_save(ch, "", 666);
        return eSUCCESS;
      }
    }
    do_save(ch, "", 0);
  }

  return eSUCCESS;
}


struct reroll_t
{
  string playerName = {};
  uint64_t choice1_rnum = {};
  obj_data* choice1_obj = nullptr;
  uint64_t choice2_rnum = {};
  obj_data* choice2_obj = nullptr;
  uint64_t orig_rnum = {};
  obj_data* orig_obj = nullptr;
  vnum_t vnum = {};
};

map<string, reroll_t> reroll_sessions = {};

int reroll_trader(char_data *ch, obj_data *obj, int cmd, const char *arg, char_data *owner)
{
  if (ch == nullptr || IS_MOB(ch))
  {
    return eFAILURE;
  }

  string arg1, remainder_args;
  tie(arg1, remainder_args) = half_chop(arg);

  reroll_t r = {};
  if (reroll_sessions.contains(GET_NAME(ch)))
  {
    r = reroll_sessions[GET_NAME(ch)];
  }
  
  obj_list_t obj_list = {};
  switch(cmd)
  {
    case CMD_LIST:
    if (r.orig_obj != nullptr && GET_OBJ_RNUM(r.orig_obj) == r.orig_rnum)
    {
      do_say(owner, fmt::format("You need to confirm or cancel rerolling {}.", GET_OBJ_SHORT(r.orig_obj)), CMD_SAY);
      return eSUCCESS;
    }

    do_say(owner, "Type reroll <object keyword> to reroll that object.");
    do_say(owner, "The cost is 1 Cloverleaf token.");
    do_say(owner, "You will get two choices or the original to pick from.");
    do_say(owner, "Type choose 1, 2 or 3 to choose either one of the two rerolls or the original.");
    return eSUCCESS;
    break;

    case CMD_REROLL:
    if (r.orig_obj != nullptr && GET_OBJ_RNUM(r.orig_obj) == r.orig_rnum)
    {
      do_say(owner, fmt::format("You need to confirm or cancel rerolling {}.", GET_OBJ_SHORT(r.orig_obj)), CMD_SAY);
      return eSUCCESS;
    }

    if (arg1.empty())
    {
      do_say(owner, fmt::format("You have to type reroll <object keyword> to reroll that object."), CMD_SAY);
      return eSUCCESS;
    }
    else
    {
      obj = get_obj_in_list_vis(ch, arg1.c_str(), ch->carrying);
      if (obj == nullptr)
      {
        do_say(owner, fmt::format("I don't see anything on you matching \'{}\'", arg1), CMD_SAY);
        return eSUCCESS;
      }
      else
      {
        r = {};
        r.orig_obj = obj;
        r.orig_rnum = GET_OBJ_RNUM(obj);
        reroll_sessions[GET_NAME(ch)] = r;
        do_say(owner, fmt::format("Are you sure you want me to reroll {} for you?", GET_OBJ_SHORT(obj)), CMD_SAY);
        do_say(owner, "Type confirm and I'll reroll it otherwise type cancel if you changed your mind.", CMD_SAY);
      }
    }
    break;
    
    case CMD_CONFIRM:
    if (r.orig_obj != nullptr && GET_OBJ_RNUM(r.orig_obj) == r.orig_rnum)
    {
      if (r.choice1_obj == nullptr)
      {
        obj_list = oload(owner, GET_OBJ_RNUM(obj), 2, true);
        for (auto& o : obj_list)
        {
          ch->send(fmt::format("n:{} s:{} v:{} r:{} {} {}\r\n", GET_OBJ_NAME(o), GET_OBJ_SHORT(o), GET_OBJ_VNUM(o), GET_OBJ_RNUM(o), fmt::ptr(o), GET_NAME(o->carried_by)));
          if (r.choice1_obj == nullptr)
          {
            r.choice1_obj = o;
            r.choice1_rnum = GET_OBJ_RNUM(o);
          }
          else if (r.choice2_obj == nullptr)
          {
            r.choice2_obj = o;
            r.choice2_rnum = GET_OBJ_RNUM(o);
          }
          reroll_sessions[GET_NAME(ch)] = r;
        }
      }
    }

    break;

    case CMD_CHOOSE:
    ch->send("Choosing...\r\n");

    break;

    default:
    return eFAILURE;
    break;
  }

  return eSUCCESS;
}