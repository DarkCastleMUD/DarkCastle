// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//

#include <qbytearray.h>
#include <qcontainerfwd.h>
#include <qfile.h>
#include <qiodevice.h>
#include <qlist.h>
#include <qminmax.h>
#include <qstring.h>
#include <qswap.h>

#include "DC/Shops.h"

class QObject;

Shops::Shops(QObject *parent)
{
}

Legacy::Legacy(QString filename)
{
  file_.setFileName(filename);
  open_status_ = file_.open(QIODevice::ReadOnly);
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
