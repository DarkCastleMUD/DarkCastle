/*
 * Timer.h
 *
 *  Created on: Nov 13, 2011
 *      Author: jhhudso
 */

#pragma once

#include <QString>
#include <QMap>
#include <ctime>
#include <cstdint>
#include <iostream>
#include <QtTypes>
class TimeVal
{
public:
  TimeVal(time_t sec = 0, suseconds_t usec = 0);
  TimeVal operator+(TimeVal t);
  TimeVal operator-(TimeVal t);
  TimeVal operator/(qint32 value);
  bool operator<(TimeVal t1);
  bool operator>(TimeVal t1);
  bool operator>=(TimeVal t1);
  void gettime(void);
  uint_fast64_t tv_sec;  /* Seconds.  */
  uint_fast64_t tv_usec; /* Microseconds.  */
};

class Timer
{
public:
  Timer();
  virtual ~Timer();
  void start();
  void stop();
  void setCount(quint16 c) { stopCount = c; }
  TimeVal getDiff();
  TimeVal getDiffMin();
  TimeVal getDiffMax();
  TimeVal getDiffAvg();

private:
  TimeVal starttv;
  TimeVal diff_cur;
  TimeVal diff_min;
  TimeVal diff_max;
  TimeVal diff_avg;
  uint_fast64_t stopCount;
  uint_fast64_t totalTime;
  friend std::ostream &operator<<(std::ostream &, Timer t);
};

extern QMap<QString, Timer> PerfTimers;

std::ostream &operator<<(std::ostream &out, Timer t);
std::ostream &operator<<(std::ostream &out, TimeVal tv);
