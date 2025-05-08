// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//
#ifndef SHOPS_H
#define SHOPS_H

#include <QFile>
#include "DC/obj.h"

typedef uint64_t room_t;

class Legacy
{
public:
    Legacy(QString filename);
    QStringList toList(void);

private:
    QFile file_;
};

class Shop
{
    Q_GADGET
public:
    enum Status
    {
        Broken,
        Ok
    };
    Q_ENUM(Status)
    explicit Shop(DC *dc);
    void setStatus(Status status);
    auto status() -> Status;
    void validate(void);

    uint64_t shop_nr(void) const;
    void setShopNR(uint64_t nr);

    vnum_t keeper_vnum(void) const;
    void setKeeperVNUM(vnum_t vnum);

    vnum_t keeper_rnum(void) const;
    void setKeeperRNUM(vnum_t rnum);

    QMap<uint64_t, int> types{}; /* Types of things shop will buy.       */
    float profit_buy{};          /* Factor to multiply cost with.        */
    float profit_sell{};         /* Factor to multiply cost with.        */
    float profit_buy_base{};
    QString no_such_item1{}; /* Message if keeper hasn't got an item */
    QString no_such_item2{}; /* Message if player hasn't got an item */
    QString missing_cash1{}; /* Message if keeper hasn't got cash    */
    QString missing_cash2{}; /* Message if player hasn't got cash    */
    QString do_not_buy{};    /* If keeper doesn't buy such things.   */
    QString message_buy{};   /* Message when player buys item        */
    QString message_sell{};  /* Message when player sells item       */
    room_t in_room{};        /* Where is the shop?                   */
    int open1{}, open2{};    /* When does the shop open?             */
    int close1{}, close2{};  /* When does the shop close?            */
    Object *inventory{};

    /* list of things shop never runs out of*/
    void shopping_buy(const char *arg, Character *ch, Character *keeper);
    void shopping_sell(const char *arg, Character *ch, Character *keeper);
    void shopping_value(const char *arg, Character *ch, Character *keeper);
    void shopping_list(const char *arg, Character *ch, Character *keeper);

private:
    DC *dc_{};
    Status status_{};
    uint64_t shop_nr_{};
    vnum_t keeper_vnum_{}; /* The mob who owns the shop (virt)     */
    vnum_t keeper_rnum_{}; /* The mob who owns the shop (real)     */
    bool is_ok(Character *shopkeeper_ch, Character *ch);
    bool trade_with(Object *item);
    bool unlimited_supply(Object *item);
    void restock_keeper(Character *shopkeeper_ch);
};

class Shops
{
public:
    explicit Shops(QObject *parent = nullptr);

private:
};

#endif
