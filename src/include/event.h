#ifndef _EVENT_H
#define _EVENT_H
/*
 * Important!!!!
 *
 * If you EVER.. EVER!!! queue an event such that it will end up
 * in it's own bucket, we'll crash VERY VERY VERY hard!!!  In other
 * words, NEVER NEVER NEVER init with a timer such that  
 * timer%MAX_NUM_EVT == 0;
 *
 */
/* $Id: event.h,v 1.1 2002/06/13 04:32:22 dcastle Exp $ */

#define MAX_NUM_EVT     1024


typedef struct  room_data     roomData;
typedef struct  char_data     charData;
typedef struct  char_data     CHAR_DATA;
typedef struct  obj_data      objData;

typedef struct  event_params  eventParams;
typedef struct  event_data    eventData;
typedef struct  event_brief   eventBrief;
typedef struct  event_bucket  eventBucket;
typedef struct  event_handler eventHandler;
typedef void                  (*eventFunc)(eventParams*);
typedef int                   (*eventKiller)(CHAR_DATA*, eventData*);

#define EVT_NORMAL       0
#define EVT_DEAD         1

struct event_brief {          /* Stick ths in a char_data                 */
   eventKiller killfunc;      /* Function to call event dies unexpectedly  */
   eventData*  event;
   eventBrief* next;
};

struct event_params {
   charData*   ch;
   objData*    obj;
   charData*   vict;
   char        str[1024];
   int         num;
   void*       extra;
};

struct event_data {
   eventFunc   func;          /* Function to call when popped              */
   char        state;         /* To be used to kill/pend a bucket          */
   char        timer;         /* Number of loops through till execution    */
   eventParams params;        /* List of params to use for the function    */
   eventData*  next;          /* Next event in the linked list             */
};


struct event_bucket {
   eventData*  head;
   int         len;
};

struct event_handler {
   eventBucket events[MAX_NUM_EVT];
   int         nextBucket;
};


void* allocMem(int size);
eventData* getNewEvent();
void initHandler();
void queueEvent(eventData* event, int time);
void processEvents();
void killBrief(CHAR_DATA* ch, eventFunc func);
eventBrief* addBrief(CHAR_DATA* ch, eventData* event);
eventBrief* foundBrief(CHAR_DATA* ch, eventFunc func);
void killCharEvents(CHAR_DATA* ch);

#endif
