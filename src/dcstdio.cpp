#include "DC/dcstdio.h"
#include <cstdio>
#include <cstdarg>

void close_file(std::FILE *stream)
{
  if (stream)
    fclose(stream);
}

FILEPtr dc_fopen(QString path, QString modes)
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

int dc_fseek(FILEPtr stream, long offset, int whence)
{
  return fseek(stream.data(), offset, whence);
}

long dc_ftell(FILEPtr stream)
{
  return ftell(stream.data());
}

void dc_clearerr(FILEPtr stream)
{
  clearerr(stream.data());
}

int dc_feof(FILEPtr stream)
{
  return feof(stream.data());
}

int dc_ferror(FILEPtr stream)
{
  return ferror(stream.data());
}

size_t dc_fread(void *ptr, size_t size, size_t n, FILEPtr stream)
{
  return fread(ptr, size, n, stream.data());
}

size_t dc_fwrite(const void *ptr, size_t size, size_t n, FILEPtr stream)
{
  return fwrite(ptr, size, n, stream.data());
}

ssize_t dc_getline(char **lineptr, size_t *n, FILEPtr stream)
{
  return getline(lineptr, n, stream.data());
}

ssize_t dc_getdelim(char **lineptr, size_t *n, int delim, FILEPtr stream)
{
  return getdelim(lineptr, n, delim, stream.data());
}

int dc_fputc(int c, FILEPtr stream)
{
  return fputc(c, stream.data());
}

int dc_putc(int c, FILEPtr stream)
{
  return putc(c, stream.data());
}

int dc_fputs(const char *s, FILEPtr stream)
{
  return fputs(s, stream.data());
}
