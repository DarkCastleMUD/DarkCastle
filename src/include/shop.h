/*
 * Contains the structures used in shops.
 */

#ifndef SHOP_H_
#define SHOP_H_

#include <structs.h> // byte, ubyte, etc..

struct player_shop_item {
   int item_vnum;               // id of item for sale
   uint32 price;                   // asking price of item
   player_shop_item * next;     // next item in list
};

#define PC_SHOP_OWNER_SIZE      20
#define PC_SHOP_SELL_MESS_SIZE  120
#define PLAYER_SHOP_KEEPER      23000

struct player_shop {
   char owner[PC_SHOP_OWNER_SIZE];              // name of player that owns shop (max is 12, but oh well)
   long   room_num;                             // number of room players shop is in
   char sell_message[PC_SHOP_SELL_MESS_SIZE];   // special message (if any) when someone buys something
   long   money_on_hand;                        // cash the player has in the bank right now
   
   player_shop_item * sale_list; // list of items player has for sale

   player_shop * next;
};


#endif
