#pragma once
#include <QSharedPointer>

using FILEPtr = QSharedPointer<std::FILE>;

FILEPtr dc_fopen(const QString &path, const QString &mode);
int dc_fprintf(FILEPtr stream, const QString &format, ...);