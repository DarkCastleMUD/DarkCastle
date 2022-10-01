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
/* $Id: event.h,v 1.2 2002/06/13 04:41:15 dcastle Exp $ */
#include "character.h"

#define MAX_NUM_EVT     1024


typedef struct  event_params  eventParams;
typedef struct  event_data    eventData;
typedef struct  event_brief   eventBrief;
typedef struct  event_bucket  eventBucket;
typedef struct  event_handler eventHandler;
typedef void                  (*eventFunc)(eventParams*);
typedef int                   (*eventKiller)(struct char_data*, eventData*);

#define EVT_NORMAL       0
#define EVT_DEAD         1

struct event_brief {          /* Stick ths in a char_data                 */
   eventKiller killfunc;      /* Function to call event dies unexpectedly  */
   eventData*  event;
   eventBrief* next;
};

struct event_params {
   char_data*   ch;
   obj_data*    obj;
   char_data*   vict;
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
void killBrief(char_data* ch, eventFunc func);
eventBrief* addBrief(char_data* ch, eventData* event);
eventBrief* foundBrief(char_data* ch, eventFunc func);
void killCharEvents(char_data* ch);

#endif
