// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//
#ifndef SHOPS_H
#define SHOPS_H

#include <QObject>

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr) : QObject(parent) {}

private:
};

class Shop : public Database
{
    Q_OBJECT
public:
    explicit Shop(QObject *parent = nullptr) : Database(parent) {}

private:
};
class Shops : public Database
{
    Q_OBJECT
public:
    explicit Shops(QObject *parent = nullptr);

private:
};

#endif
