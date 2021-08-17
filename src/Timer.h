/*
 * Timer.h
 *
 *  Created on: Nov 13, 2011
 *      Author: jhhudso
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <string>
#include <map>
#include <sys/time.h>
#include <stdint.h>
#include <iostream>

using namespace std;

struct TimeVal {
	TimeVal(time_t sec = 0, suseconds_t usec = 0);
	TimeVal operator+(TimeVal t);
	TimeVal operator-(TimeVal t);
	TimeVal operator/(int value);
	bool operator<(TimeVal t1);
	bool operator>(TimeVal t1);
	bool operator>=(TimeVal t1);
	void gettime(void);
    uint_fast64_t tv_sec;		/* Seconds.  */
    uint_fast64_t tv_usec;	/* Microseconds.  */
};

class Timer {
public:
	Timer();
	virtual ~Timer();
	void start();
	void stop();
	void setCount(uint16_t c) { stopCount = c; }
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
	friend ostream & operator<< (ostream &, Timer t);
};

extern map<string, Timer> PerfTimers;

ostream& operator<<(ostream &out, Timer t);
ostream& operator<<(ostream &out, TimeVal tv);

#endif /* TIMER_H_ */
