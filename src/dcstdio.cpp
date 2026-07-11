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

int dc_fgetc(FILEPtr stream)
{
  return fgetc(stream.data());
}

char *dc_fgets(char *s, int size, FILEPtr stream)
{
  return fgets(s, size, stream.data());
}

int dc_ungetc(int c, FILEPtr stream)
{
  return ungetc(c, stream.data());
}

int dc_fscanf(FILEPtr stream, QString format, ...)
{
  va_list ap;
  va_start(ap, format);
  auto result = vfscanf(stream.data(), qPrintable(format), ap);
  va_end(ap);
  return result;
}