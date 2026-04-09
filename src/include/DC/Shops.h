// Copyright 2023-2026 Jared H. Hudson
// Licensed under LGPL
//
#pragma once

#include "DC/common.h"
#include <QFile>
#include <QMap>

typedef quint64 room_t;

class Legacy
{
public:
  Legacy(QString filename);
  QStringList toList(void);

private:
  QFile file_;
  bool open_status_ = {};
};
