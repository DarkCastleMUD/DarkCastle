#ifndef TIMEINFO_H_
#define TIMEINFO_H_
/************************************************************************
| $Id: timeinfo.h,v 1.2 2002/06/13 04:41:15 dcastle Exp $
| timeinfo.h
| Description:  Declares the information for handling time in the game.
*/
extern "C"
{
  #include <time.h>
}

#define SECS_PER_REAL_MIN    60
#define SECS_PER_REAL_HOUR   (60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY    (24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR   (365*SECS_PER_REAL_DAY)

#define SECS_PER_MUD_HOUR    65
#define SECS_PER_MUD_DAY     (24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH   (35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR    (17*SECS_PER_MUD_MONTH)


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data
{
    int hours;
    int day;
    int month;
    int year;
};

/* These data contain information about a players time data */
struct time_data
{
  time_t birth;    /* This represents the characters age                */
  time_t logon;    /* Time of the last logon (used to calculate played) */
  long played;      /* This is the total accumulated time played in secs */
};

#endif
