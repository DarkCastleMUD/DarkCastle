// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//

#include "DC/DC.h"

Shops::Shops(QObject *parent)
    : QObject(parent), dc_(qobject_cast<DC *>(parent))
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
