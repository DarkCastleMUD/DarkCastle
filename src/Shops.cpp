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
