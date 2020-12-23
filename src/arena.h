#ifndef ARENA_H_
#define ARENA_H_

/************************************************************************
| $Id: arena.h,v 1.6 2009/04/24 21:50:17 shane Exp $
| arena.h
| This contains the arena stuff
*/

#define ARENA_LOW  14600
#define ARENA_HIGH 14680
#define ARENA_DEATHTRAP 14680

enum ARENA_TYPE { NORMAL, CHAOS, POTATO, PRIZE, HP };
enum ARENA_STATUS { CLOSED, OPENED };

struct _arena {
  int low;
  int high;
  int num;
  int cur_num;
  int hplimit;
  ARENA_TYPE type;
  ARENA_STATUS status;
};

extern struct _arena arena;

#endif
