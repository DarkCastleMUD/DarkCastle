#include <cstdio>
#include <QSharedPointer>
void close_file(std::FILE *fp);
using FILEPtr = QSharedPointer<std::FILE>;