#ifndef MEMORY_H_
#define MEMORY_H_
/************************************************************************
| $Id: memory.h,v 1.2 2002/06/13 04:41:15 dcastle Exp $
| memory.h
| Description:  This file should be included in all .C files wishing to
|   allocate/free memory.  It declares dc_alloc() for allocating new
|   memory and dc_free() for freeing memory.
*/
extern "C"
{
  #include <stdlib.h>
}

void *dc_alloc(size_t nmemb, size_t size);
void *dc_realloc(void * oldptr, size_t size);
void *dc_free(void *ptr);


#endif /* MEMORY_H_ */
