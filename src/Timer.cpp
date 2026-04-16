/*
 * SystemTimer.cpp
 *
 *  Created on: Nov 13, 2011
 *      Author: jhhudso
 */

#include "DC/DC.h"

QMap<QString, SystemTimer> PerfSystemTimers;

TimeVal::TimeVal(time_t sec, suseconds_t usec)
{
  tv_sec = sec;
  tv_usec = usec;
}

TimeVal TimeVal::operator+(TimeVal t1)
{
  TimeVal t;
  t.tv_sec = tv_sec + t1.tv_sec;
  t.tv_usec = tv_usec + t1.tv_usec;
  return t;
}

TimeVal TimeVal::operator-(TimeVal t1)
{
  TimeVal t;
  assert(tv_sec >= t1.tv_sec);
  t.tv_sec = tv_sec - t1.tv_sec;

  if (tv_usec >= t1.tv_usec)
  {
    t.tv_usec = tv_usec - t1.tv_usec;
  }
  else
  {
    t.tv_sec--;
    t.tv_usec = tv_usec + 1000000 - t1.tv_usec;
  }

  return t;
}

TimeVal TimeVal::operator/(qint32 value)
{
  TimeVal t;
  t.tv_sec = tv_sec / value;
  t.tv_usec = tv_usec / value;
  return t;
}

bool TimeVal::operator<(TimeVal t1)
{
  if (tv_sec < t1.tv_sec)
  {
    return true;
  }
  else if (tv_sec == t1.tv_sec && tv_usec < t1.tv_usec)
  {
    return true;
  }
  return false;
}

bool TimeVal::operator>(TimeVal t1)
{
  if (tv_sec > t1.tv_sec)
  {
    return true;
  }
  else if (tv_sec == t1.tv_sec && tv_usec > t1.tv_usec)
  {
    return true;
  }
  return false;
}

bool TimeVal::operator>=(TimeVal t1)
{
  if (tv_sec > t1.tv_sec)
  {
    return true;
  }
  else if (tv_sec == t1.tv_sec && tv_usec > t1.tv_usec)
  {
    return true;
  }
  else if (tv_sec == t1.tv_sec && tv_usec == t1.tv_usec)
  {
    return true;
  }
  return false;
}

void TimeVal::gettime()
{
  timeval tv;
  gettimeofday(&tv, nullptr);
  tv_sec = tv.tv_sec;
  tv_usec = tv.tv_usec;
}

SystemTimer::SystemTimer()
    : stopCount(0), totalTime(0)
{
}

SystemTimer::~SystemTimer()
{
}

void SystemTimer::start()
{
  starttv.gettime();
}

void SystemTimer::stop()
{
  TimeVal stoptv;
  stoptv.gettime();

  if (stoptv < starttv)
  {
    // std::cerr << "SystemTimer::stop() time went backwards." << std::endl;
    stoptv = starttv;
  }
  diff_cur = stoptv - starttv;

  // Initialize minimum and average values on first run
  if (stopCount++ == 0)
  {
    diff_min = diff_cur;
    diff_avg = diff_cur;
  }

  if (diff_cur < diff_min)
  {
    diff_min = diff_cur;
  }

  if (diff_cur > diff_max)
  {
    diff_max = diff_cur;
  }

  diff_avg = (diff_avg + diff_cur) / 2;
  totalTime += (diff_cur.tv_usec / 1000);
}

TimeVal SystemTimer::getDiff()
{
  return diff_cur;
}

TimeVal SystemTimer::getDiffMin()
{
  return diff_min;
}

TimeVal SystemTimer::getDiffMax()
{
  return diff_max;
}

TimeVal SystemTimer::getDiffAvg()
{
  return diff_avg;
}

std::ostream &operator<<(std::ostream &out, SystemTimer t)
{
  out << "last=" << t.getDiff() << " min=" << t.getDiffMin()
      << " max=" << t.getDiffMax() << " avg=" << t.getDiffAvg()
      << " count=" << t.Count() << " total=" << t.Total() << "ms";
  return out;
}
std::ostream &operator<<(std::ostream &out, TimeVal tv)
{
  //	out << static_cast<quint32 __time_t>(tv.tv_sec) << "s " << static_cast<quint32 __suseconds_t>(tv.tv_usec) << "µ";
  out << tv.tv_sec << "s " << tv.tv_usec << "µ";
  return out;
}
