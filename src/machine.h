#ifndef MACHINE_H_
#define MACHINE_H_
/************************************************************************
| $Id: machine.h,v 1.2 2002/06/13 04:41:15 dcastle Exp $
| machine.h
| Description:  This file contains all of the machine-specific information
|   and prototypes.
*/
extern "C"
{
#include <time.h>
}

/*
 * Function prototypes.
 * 09 Nov 1992  Furey
 */

#if    defined(linux)
time_t time(time_t *t);
#ifndef LINUX
  void srandom(unsigned int);
  int32_t random	(void);
#endif
#endif

/* #if	defined(sun) */

#ifdef SUN
extern "C"
{
  /* void	perror		(char *s); */
  /* int	fread(void *ptr, int size, int nitems, FILE *stream); */
  /* int  fwrite	(void *ptr, int size, int nitems, FILE *stream);*/
  char *	index		(const char *s, int c);
  /* void	fclose		(FILE *stream); */
  int	fseek		(FILE *stream, int32_t offset, int ptrname);
  /* int	fputs		(char *s, FILE *stream); */
  /* int	fscanf		(FILE *stream, char *format, ...); */
  /* int	sscanf		(char *s, char *format, ...); */
  /* int	fprintf		(FILE *stream, char *format, ...); */
  int32_t	random		(void);
  void	srandom		(int seed);
  /* int	printf		(char *format, ...); */
  int	sigsetmask	(int mask);
  int	gethostname	(char *name, int namelen);
  time_t	time		(time_t *tloc);
}
#endif

#if	(0)	//defined(ultrix)
int	gethostname	(char *name, int namelen);
char *	index		(char *s, int c);
int32_t	random		(void);
int	sigsetmask	(int mask);
void	srandom		(int seed);
time_t	time		(time_t *tloc);
#endif

#endif /* MACHINE_H_ */
