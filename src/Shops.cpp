// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//

#include <QObject>
#include <QSqlError>
#include <QSqlRelationalTableModel>
#include <QSqlQuery>

#include "shop.h" // legacy shop
#include "Shops.h"
#include "utility.h"

Shops::Shops(QObject *parent) : Database(parent)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    // db.setHostName("localhost");
    // db.setDatabaseName("dcastle");
    // db.setUserName("dcastle");
    //  db.setPassword("password");
    bool ok = db.open();

    if (!ok)
    {
        logentry(db.lastError().databaseText());
        logentry(db.lastError().driverText());
    }
    else
    {
        if (!db.tables().contains("shops"))
        {
            QSqlQuery query;
            if (query.exec("CREATE TABLE shops (id BIGINT GENERATED ALWAYS AS IDENTITY)"))
            {
                logentry(QString("Created database table %1.").arg("shops"));
                if (!db.tables().contains("shops"))
                {
                    logentry(QString("Failed to find database table %1 after creating it.").arg("shops"));
                    return;
                }
            }
            else
            {
                logentry(QString("Failed to create database table %1.").arg("shops"));
                logentry(db.lastError().databaseText());
                logentry(db.lastError().driverText());
            }
        }
        QSqlRelationalTableModel *model = new QSqlRelationalTableModel(parent);

        model->setTable("shops");
        model->setEditStrategy(QSqlTableModel::OnFieldChange);
        model->setHeaderData(0, Qt::Horizontal, tr("Type"));
        model->setHeaderData(1, Qt::Horizontal, tr("Profit Sell"));

        model->setRelation(0, QSqlRelation("city", "id", "name"));
        model->setRelation(1, QSqlRelation("country", "id", "name"));
    }
}