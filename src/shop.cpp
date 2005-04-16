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
/* $Id: shop.cpp,v 1.14 2005/04/16 20:18:02 shane Exp $ */

extern "C"
{
#include <stdio.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <affect.h>
#include <character.h>
#include <utility.h>
#include <interp.h>
#include <obj.h>
#include <levels.h>
#include <player.h>
#include <handler.h>
#include <mobile.h>
#include <room.h>
#include <fileinfo.h>
#include <db.h>
#include <act.h>
#include <returnvals.h>
#include <shop.h>
#include <spells.h>

extern struct index_data *mob_index;

struct player_shop * g_playershops;

extern CWorld world;
extern struct index_data *obj_index;
extern int top_of_world;
extern struct room_data ** world_array;
 
extern struct time_info_data time_info;

struct shop_data shop_index[MAX_SHOP];
int max_shop;

// extern function
struct obj_data * search_char_for_item(char_data * ch, sh_int item_number);
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
    if ( IS_SET(ch->affected_by, AFF_KILLER) )
    {
        do_say( keeper, "Go away before I call the guards!!", 0 );
        sprintf( buf, "%s the KILLER is over here!\n\r", GET_SHORT(ch) );
        do_shout( keeper, buf, 0 );
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
    
    else if ( time_info.hours <= shop_index[shop_nr].close1 )
        ;

    else if ( time_info.hours < shop_index[shop_nr].open2 )
    {
        do_say( keeper, "Come back later!", 0 );
        return FALSE;
    }

    else if ( time_info.hours <= shop_index[shop_nr].close2 )
        ;

    else
    {
        do_say( keeper, "Sorry, come back tomorrow.", 0 );
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
void shopping_buy( char *arg, CHAR_DATA *ch,
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
      if(search_char_for_item(ch, obj->item_number)) {
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
void shopping_sell( char *arg, CHAR_DATA *ch,
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
void shopping_value( char *arg, CHAR_DATA *ch, 
    CHAR_DATA *keeper, int shop_nr )
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    char argm[MAX_INPUT_LENGTH+1];
    struct obj_data *obj;
    int cost;
    extern char *extra_bits[];
    extern char *size_bits[];
    extern char *spells[];

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
        sprintf( buf, shop_index[shop_nr].no_such_item2, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }
///////////////////////
    if(mob_index[keeper->mobdata->nr].virt == 3003) { //if the weaponsmith in town
       act("You hold up $p for the Weaponsmith to examine.", ch, obj, 0, TO_CHAR, 0);
       act("$n holds up $p for the Weaponsmith to examine..", ch, obj, 0, TO_ROOM, 0);
       do_emote(keeper, "looks carefully at the item.", 9);
       if(GET_ITEM_TYPE(obj) == ITEM_WEAPON) {
          if(obj->obj_flags.eq_level < 20) {
             sprintf(buf, "Well, %s is able to be used by ", obj->name);
             sprintbit(obj->obj_flags.size, size_bits, buf2);
             strcat(buf, buf2);
             do_say(keeper, buf, 9);
             sprintf(buf, "and it is ");
             sprintbit(obj->obj_flags.extra_flags, extra_bits, buf2);
             strcat(buf, buf2);
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
       act("You hold up $p for the Armourer to examine.", ch, obj, 0, TO_CHAR, 0);
       act("$n holds up $p to the Armourer to examine.", ch, obj, 0, TO_ROOM, 0);
       do_emote(keeper, "looks carefully at the item.", 9);
       if(GET_ITEM_TYPE(obj) == ITEM_ARMOR) {
          if(obj->obj_flags.eq_level < 20) {
             sprintf(buf, "Ah yes, %s can be worn by ", obj->name);
             sprintbit(obj->obj_flags.size, size_bits, buf2);
             strcat(buf, buf2);
             do_say(keeper, buf, 9);
             sprintf(buf, "and it is ");
             sprintbit(obj->obj_flags.extra_flags, extra_bits, buf2);
             strcat(buf, buf2);
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
       act("You hold up $p for the Wizard to examine.", ch, obj, 0, TO_CHAR, 0);
       act("$n holds up $p for the Wizard to examine..", ch, obj, 0, TO_ROOM, 0);
       do_emote(keeper, "looks carefully at the item.", 9);
       if(GET_ITEM_TYPE(obj) == ITEM_SCROLL || GET_ITEM_TYPE(obj) == ITEM_WAND || GET_ITEM_TYPE(obj) == ITEM_POTION || GET_ITEM_TYPE(obj) == ITEM_STAFF) {
          if(obj->obj_flags.value[0] < 20) {
             sprintf(buf, "Excellent, %s has been imbued with energies of the %dth level.", obj->name, obj->obj_flags.value[0]);
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
                   do_say(keeper, "It looks like it's been used some.", 9);
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
///////////////////////
    if ( !trade_with( obj, shop_nr ) || obj->obj_flags.cost < 1 )
    {
        sprintf( buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch) );
        do_tell( keeper, buf, 0 );
        return;
    }

    cost = (int) ( obj->obj_flags.cost * shop_index[shop_nr].profit_sell );
    sprintf( buf, "%s I'll give you %d gold coins for that.",
        GET_NAME(ch), cost );
    do_tell( keeper, buf, 0 );
    return;
}



/*
 * List available items.
 */
void shopping_list( char *arg, CHAR_DATA *ch,
     CHAR_DATA *keeper, int shop_nr )
{
    char buf[MAX_STRING_LENGTH];
    struct obj_data *obj,*tobj;
    int cost;
    extern char *drinks[];
    int found;
    int done[40]; // To show 'em numbered instead of a long list of duplicates
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
	if (i < 40) 
	done[i++] = obj_index[obj->item_number].virt;
	else break;
        a = 0;
	for (tobj = keeper->carrying; tobj; tobj = tobj->next_content)
          if (obj_index[tobj->item_number].virt == obj_index[obj->item_number].virt)
	    a++;
        if ( GET_ITEM_TYPE(obj) == ITEM_DRINKCON && obj->obj_flags.value[1] )
        {
            sprintf( buf, "[%3d] [%7d] %s of %s.\n\r",
                a, cost, obj->short_description,
                drinks[obj->obj_flags.value[2]] );
        }
        else
        {
            sprintf( buf, "[%3d] [%7d] %s.\n\r",
                a, cost, obj->short_description );
        }
        send_to_char(buf, ch);
    }

    if ( !found )
        send_to_char( "You can't buy anything here!\n\r", ch );

    return;
}



// Spec proc for shop keepers.
// TODO - Remove goto's from this....I hate goto's.  This is C, not BASIC....
int shop_keeper( CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA * invoker )
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
    if ( ch->in_room != shop_index[shop_nr].in_room )
        return eFAILURE;

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

        fscanf( fp, "%f \n", &shop_index[max_shop].profit_buy  );
        fscanf( fp, "%f \n", &shop_index[max_shop].profit_sell );
        for( count = 0; count < MAX_TRADE; count++ )
        {
            fscanf( fp, "%d \n", &shop_index[max_shop].type[count] );
        }

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

        shop_index[max_shop].in_room    = real_room( temp );

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
  fread(&shop->room_num, sizeof(long), 1, fp);
  fread(&shop->sell_message, sizeof(char), PC_SHOP_SELL_MESS_SIZE, fp);
  fread(&shop->money_on_hand, sizeof(long), 1, fp);

  code[3] = '\0';
  fread(&code, sizeof(char), 3, fp);

  while(strcmp(code, "END"))
  {
    // add future stuff here

    logf(IMMORTAL, LOG_BUG, "Illegal code in player shop %s", shop->owner);
    logf(IMMORTAL, LOG_WORLD, "Illegal code in player shop %s", shop->owner);
    exit(1);
  }

  fread(&count, sizeof(long), 1, fp);

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
  fwrite(&(shop->room_num), sizeof(long), 1, fp);
  fwrite(&(shop->sell_message), sizeof(char), PC_SHOP_SELL_MESS_SIZE, fp);
  fwrite(&(shop->money_on_hand), sizeof(long), 1, fp);

  // add stuff later here with 3 digit code
  // end of variable data
  fwrite("END", sizeof(char), 3, fp);

  for(item = shop->sale_list; item; item = item->next)
    count++;

  fwrite(&(count), sizeof(long), 1, fp);

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
void player_shopping_stock(char * arg, char_data * ch, char_data * keeper)
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

void player_shopping_buy(char * arg, char_data * ch, char_data * keeper)
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

void player_shopping_withdraw(char * arg, char_data * ch, char_data * keeper)
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

void player_shopping_design(char * arg, char_data * ch, char_data * keeper)
{
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  sh_int skill;

  if(IS_NPC(ch))   
    return;

  char * pdesign_values [] = {
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

void player_shopping_sell(char * arg, char_data * ch, char_data * keeper)
{
  send_to_char("These shop keeper's don't buy stuff.\r\n", ch);
}

void player_shopping_value(char * arg, char_data * ch, char_data * keeper)
{
  send_to_char("These shop keeper's don't buy stuff.\r\n", ch);
}

void player_shopping_list(char * arg, char_data * ch, char_data * keeper)
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

int player_shop_keeper( CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA * invoker )
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

int do_pshopedit(char_data * ch, char * arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char select[MAX_INPUT_LENGTH];
  char text[MAX_INPUT_LENGTH];
  sh_int skill, i;
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

void assign_the_player_shopkeepers( )
{
   mob_index[real_mobile(PLAYER_SHOP_KEEPER)].non_combat_func = player_shop_keeper;
}


