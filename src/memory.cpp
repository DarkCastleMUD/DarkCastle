#include <cstdio>
#include <cstdlib>

#include <QDebug>
#include "memory.h"

/************************************************************************
| void * dc_alloc(size_t nmemb, size_t size)
| Precondition:  None
| Postcondition: nmemb*size bytes are allocated and returned if available,
|   zero is returned if not available.  The value of all returned memory is
|   zeroed.
*/
void *dc_alloc(size_t nmemb, size_t size)
{
  void *new_mem;
  new_mem = calloc(nmemb, size);
  if (new_mem == 0)
  {
    qFatal("OUT OF MEMORY in dc_alloc()!");
  }
  return (new_mem);
}

// void * dc_realloc(void * oldptr, size_t size)
void *dc_realloc(void *oldptr, size_t size)
{
  void *new_mem = nullptr;

  // if no old pointer, use dc_alloc to calloc new memory instead of
  // realloc's default, which is to use malloc() (that way memory is 0'd)
  if (!oldptr)
    return (dc_alloc(1, size));

  // realloc would handle this fine, but let's use out dc_free to it get's 0'd
  if (0 == size)
  {
    dc_free(oldptr);
    return nullptr;
  }

  if (size < 0)
  {
    qFatal("Attempt to realloc with negative size?");
  }

  new_mem = realloc(oldptr, size);

  if (0 == new_mem)
  {
    qFatal("OUT OF MEMORY in dc_realloc()!");
  }
  return (new_mem);
}

void *dc_free(void *ptr)
{
  if (ptr)
  {
    free(ptr);
  }
  return nullptr;
}
