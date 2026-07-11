#pragma once
#include <QSharedPointer>

using FILEPtr = QSharedPointer<std::FILE>;

FILEPtr dc_fopen(const QString &path, const QString &mode);
int dc_fprintf(FILEPtr stream, const QString &format, ...);
char *dc_fgets(char *s, int size, FILEPtr stream);
int dc_fgetc(FILEPtr stream);
int dc_ungetc(int c, FILEPtr stream);
int dc_fscanf(FILEPtr stream, const QString &format, ...);