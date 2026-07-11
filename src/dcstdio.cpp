#include "DC/dcstdio.h"
#include <cstdio>
#include <cstdarg>

void close_file(std::FILE *stream)
{
  if (stream)
    std::fclose(stream);
}

FILEPtr dc_fopen(const QString &path, const QString &modes)
{
  auto stream = fopen(qPrintable(path), qPrintable(modes));
  return FILEPtr(stream, close_file);
}

int dc_fprintf(FILEPtr stream, QString format, ...)
{
  va_list ap;
  va_start(ap, format);
  auto result = vfprintf(stream.data(), qPrintable(format), ap);
  va_end(ap);
  return result;
}
