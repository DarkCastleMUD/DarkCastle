/*
 * We kept having trouble with accessing the hard drive, too many open
 * files.  Now we only use dc_fopen() and dc_fclose(), which use a NULL
 * file.  This way there is always one file open.  It is important that
 * the calls NOT be nested...if ths happens, I'll have to add another
 * function for each level of nesting.
 *
 * -Sadus
 */
/* $Id: nullfile.cpp,v 1.1 2002/06/13 04:32:18 dcastle Exp $ */

extern "C"
{
#include <stdio.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <structs.h>
#include <levels.h>

/* External functions */
void    log             (char * str, int god_level, long type);
int fclose(FILE *);
#ifndef LINUX
#ifndef SUN
#ifndef FreeBSD
  void perror(char *);
#endif
#endif
#endif

#ifndef LOG_BUG
#define LOG_BUG           1
#define LOG_PRAYER        1<<1
#define LOG_GOD           1<<2
#define LOG_MORTAL        1<<3
#define LOG_SOCKET        1<<4
#define LOG_MISC          1<<5
#define LOG_PLAYER        1<<6
#endif

FILE * NULL_FILE;

FILE * dc_fopen(const char *filename, const char *type)
{
  FILE *x;

  if(NULL_FILE) {
    fclose(NULL_FILE);
    NULL_FILE = 0;
  }
  
  if(!(x = fopen(filename, type)))
    if(!(NULL_FILE = fopen("../lib/whassup", "w"))) {
      log("Unable to open NULL_FILE in dc_fopen.", ANGEL, LOG_BUG);
      perror("Unable to open NULL_FILE in dc_fopen.");
    }
    
  return x;
}

int dc_fclose(FILE * fl)
{
  int x;
  
  if(!fl) return(0);

  x = fclose(fl);
  
  if(!(NULL_FILE))
    if(!(NULL_FILE = fopen("../lib/whassup", "w"))) {
      log("Unable to open NULL_FILE in dc_fclose.", ANGEL, LOG_BUG);
      perror("Unable to open NULL_FILE in dc_fclose.");
    }
    
  return x;
}

