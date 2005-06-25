/*
 * Contains the structures used in shops.
 */

#ifndef SHOP_H_
#define SHOP_H_

#include <structs.h> // ubyte, ubyte, etc..

struct player_shop_item {
   int item_vnum;               // id of item for sale
   uint32 price;                   // asking price of item
   player_shop_item * next;     // next item in list
};
#define MAX_SHOP 64
#define MAX_TRADE 5
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

struct shop_data
{
    int type[MAX_TRADE];    /* Types of things shop will buy.       */
    float profit_buy;       /* Factor to multiply cost with.        */
    float profit_sell;      /* Factor to multiply cost with.        */
    char *no_such_item1;    /* Message if keeper hasn't got an item */
    char *no_such_item2;    /* Message if player hasn't got an item */
    char *missing_cash1;    /* Message if keeper hasn't got cash    */
    char *missing_cash2;    /* Message if player hasn't got cash    */
    char *do_not_buy;       /* If keeper doesn't buy such things.   */
    char *message_buy;      /* Message when player buys item        */
    char *message_sell;     /* Message when player sells item       */
    int keeper;             /* The mob who owns the shop (virt)  */
    int in_room;            /* Where is the shop?                   */
    int open1, open2;       /* When does the shop open?             */
    int close1, close2;     /* When does the shop close?            */
    struct obj_data *inventory;  /* list of things shop never runs out of  
*/
};

#endif
