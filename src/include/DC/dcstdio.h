#pragma once
#include <QSharedPointer>

using FILEPtr = QSharedPointer<std::FILE>;

FILEPtr dc_fopen(QString path, QString mode);
int dc_fprintf(FILEPtr stream, QString format, ...);
char *dc_fgets(char *s, int size, FILEPtr stream);
int dc_fgetc(FILEPtr stream);
int dc_ungetc(int c, FILEPtr stream);
int dc_fscanf(FILEPtr stream, QString format, ...);
int dc_fseek(FILEPtr stream, long offset, int whence);
long dc_ftell(FILEPtr stream);
void dc_clearerr(FILEPtr stream);
int dc_feof(FILEPtr stream);
int dc_ferror(FILEPtr stream);
size_t dc_fread(void *ptr, size_t size, size_t n, FILEPtr stream);
size_t dc_fwrite(const void *ptr, size_t size, size_t n, FILEPtr stream);
ssize_t dc_getline(char **lineptr, size_t *n, FILEPtr stream);
ssize_t dc_getdelim(char **lineptr, size_t *n, int delim, FILEPtr stream);
int dc_fputc(int c, FILEPtr stream);
int dc_putc(int c, FILEPtr stream);
int dc_fputs(const char *s, FILEPtr stream);
