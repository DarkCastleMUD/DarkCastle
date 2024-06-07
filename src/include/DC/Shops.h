// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//
#ifndef SHOPS_H
#define SHOPS_H

#include <QFile>
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
public:
    QMap<uint64_t, int> type; /* Types of things shop will buy.       */
    float profit_buy{};       /* Factor to multiply cost with.        */
    float profit_sell{};      /* Factor to multiply cost with.        */
    float profit_buy_base{};
    QString no_such_item1{};   /* Message if keeper hasn't got an item */
    QString no_such_item2{};   /* Message if player hasn't got an item */
    QString missing_cash1{};   /* Message if keeper hasn't got cash    */
    QString missing_cash2{};   /* Message if player hasn't got cash    */
    QString do_not_buy{};      /* If keeper doesn't buy such things.   */
    QString message_buy{};     /* Message when player buys item        */
    QString message_sell{};    /* Message when player sells item       */
    vnum_t keeper{};           /* The mob who owns the shop (virt)  */
    room_t in_room{};          /* Where is the shop?                   */
    int open1{}, open2{};      /* When does the shop open?             */
    int close1{}, close2{};    /* When does the shop close?            */
    class Object *inventory{}; /* list of things shop never runs out of
                                */
};

class Shops
{
public:
    explicit Shops(QObject *parent = nullptr);

private:
};

#endif
