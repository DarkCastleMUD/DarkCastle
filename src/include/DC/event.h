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
#include "DC/character.h"

#define MAX_NUM_EVT 1024

typedef event_params eventParams;
typedef event_data eventData;
typedef event_brief eventBrief;
typedef event_bucket eventBucket;
typedef event_handler eventHandler;
typedef void (*eventFunc)(eventParams *);
typedef int (*eventKiller)(Character *, eventData *);

#define EVT_NORMAL 0
#define EVT_DEAD 1

class event_brief
{ /* Stick ths in a Character                 */
public:
  eventKiller killfunc; /* Function to call event dies unexpectedly  */
  eventData *event;
  eventBrief *next;
};

class event_params
{
  public:
  Character *ch;
  class Object *obj;
  Character *vict;
  char str[1024];
  int num;
  void *extra;
};

class event_data
{
  public:
  eventFunc func;     /* Function to call when popped              */
  char state;         /* To be used to kill/pend a bucket          */
  char timer;         /* Number of loops through till execution    */
  eventParams params; /* List of params to use for the function    */
  eventData *next;    /* Next event in the linked list             */
};

class event_bucket
{
  public:
  eventData *head;
  int len;
};

class event_handler
{
  public:
  eventBucket events[MAX_NUM_EVT];
  int nextBucket;
};

void *allocMem(int size);
eventData *getNewEvent();
void initHandler();
void queueEvent(eventData *event, int time);
void processEvents();
void killBrief(Character *ch, eventFunc func);
eventBrief *addBrief(Character *ch, eventData *event);
eventBrief *foundBrief(Character *ch, eventFunc func);
void killCharEvents(Character *ch);

#endif
