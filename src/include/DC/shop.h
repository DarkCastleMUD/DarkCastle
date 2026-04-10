#pragma once
#include <QtTypes>
#include <QString>

typedef quint64 vnum_t;

constexpr auto MAX_SHOP = 70;
constexpr auto MAX_TRADE = 5;
constexpr auto PC_SHOP_OWNER_SIZE = 20;
constexpr auto PC_SHOP_SELL_MESS_SIZE = 120;
constexpr auto PLAYER_SHOP_KEEPER = 23000;

class player_shop_item
{
public:
  qint32 item_vnum;       // id of item for sale
  quint32 price;          // asking price of item
  player_shop_item *next; // next item in list
};

class player_shop
{
public:
  QString owner;               // name of player that owns shop (max is 12, but oh well)
  qint32 room_num;             // number of room players shop is in
  QString sell_message;        // special message (if any) when someone buys something
  qint32 money_on_hand;        // cash the player has in the bank right now
  player_shop_item *sale_list; // list of items player has for sale
  player_shop *next;
};

void redo_shop_profit(void);
