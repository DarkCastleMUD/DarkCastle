#ifndef ARENA_H_
#define ARENA_H_

/************************************************************************
| $Id: arena.h,v 1.3 2006/12/30 19:39:22 jhhudso Exp $
| arena.h
| This contains the arena stuff
*/

#define ARENA_LOW 14600
#define ARENA_HI  14680
#define ARENA_DEATHTRAP 14680

enum ARENA_TYPE { NORMAL, CHAOS, POTATO, PRIZE };
enum ARENA_STATUS { OPENED, CLOSED };

struct _arena {
  int low;
  int high;
  int num;
  int cur_num;
  ARENA_TYPE type;
  ARENA_STATUS status;
};

extern struct _arena arena;

#endif
