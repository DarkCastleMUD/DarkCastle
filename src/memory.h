#ifndef MEMORY_H_
#define MEMORY_H_
/************************************************************************
| $Id: memory.h,v 1.3 2002/07/28 02:04:19 pirahna Exp $
| memory.h
| Description:  This file should be included in all .C files wishing to
|   allocate/free memory.  It declares dc_alloc() for allocating new
|   memory and dc_free() for freeing memory.
*/
#include <stdlib.h>

void *dc_alloc(size_t nmemb, size_t size);
void *dc_realloc(void * oldptr, size_t size);
void *dc_free(void * ptr);


#endif /* MEMORY_H_ */
