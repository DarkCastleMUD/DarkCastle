// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//
#ifndef SHOPS_H
#define SHOPS_H

#include <QObject>

class Shop : public QObject
{
    Q_OBJECT
public:
    explicit Shop(QObject *parent = nullptr) : QObject(parent) {}

private:
};
class Shops : public QObject
{
    Q_OBJECT
public:
    explicit Shops(QObject *parent = nullptr);

private:
};

#endif
