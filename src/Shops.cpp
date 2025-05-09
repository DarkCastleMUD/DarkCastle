// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//

#include <QSqlError>
#include <QSqlRelationalTableModel>
#include <QSqlQuery>
#include <QVariant>

#include "DC/shop.h" // legacy shop
#include "DC/Shops.h"
#include "DC/utility.h"
#include "DC/db.h"

Shops::Shops(QObject *parent)
{
}

Legacy::Legacy(QString filename)
{
    file_.setFileName(filename);
    file_.open(QIODevice::ReadOnly);
}

QStringList Legacy::toList(void)
{
    QStringList list;
    while (!file_.atEnd())
    {
        QByteArray line = file_.readLine();
        list.push_back(line);
    }

    return list;
}

void Shop::setStatus(Status status)
{
    status_ = status;
}

auto Shop::status() -> Status
{
    validate();
    return status_;
}

void Shop::validate(void)
{
    auto rnum = real_mobile(keeper_vnum_);
    if (rnum == -1)
    {
        qWarning("%s", qUtf8Printable(QStringLiteral("Shop %1 has shopkeeper mob VNUM set to %2 which does not exist.").arg(shop_nr_).arg(keeper_vnum_)));
        status_ = Status::Broken;
    }
    keeper_rnum_ = rnum;

    if (real_room(in_room) == DC::NOWHERE)
    {
        qWarning("%s", qUtf8Printable(QStringLiteral("Shop %1 has in_room set to %2 which does not exist.").arg(shop_nr_).arg(in_room)));
        status_ = Status::Broken;
    }

    if (shop_nr_ == 0)
    {
        status_ = Status::Broken;
    }
    status_ = Status::Ok;
}

uint64_t Shop::shop_nr(void) const
{
    return shop_nr_;
}

void Shop::setShopNR(uint64_t nr)
{
    shop_nr_ = nr;
}

vnum_t Shop::keeper_vnum(void) const
{
    return keeper_vnum_;
}

void Shop::setKeeperVNUM(vnum_t vnum)
{
    keeper_vnum_ = vnum;
}

vnum_t Shop::keeper_rnum(void) const
{
    return keeper_rnum_;
}

void Shop::setKeeperRNUM(vnum_t rnum)
{
    keeper_rnum_ = rnum;
}
