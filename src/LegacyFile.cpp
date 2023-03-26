#include <QDebug>
#include <cstdio>

#include "LegacyFile.h"

void LegacyFile::open(void)
{
    if ((stream_ = fopen(full_new_filename_.toStdString().c_str(), "w")) == nullptr)
    {
        qCritical() << error_message_.arg(filename_);
    }
}