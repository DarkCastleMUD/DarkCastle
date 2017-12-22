/*
 * Timer.cpp
 *
 *  Created on: Nov 13, 2011
 *      Author: jhhudso
 */

#include <assert.h>
#include "Timer.h"

using namespace std;

TimeVal::TimeVal(time_t sec, suseconds_t usec) {
	tv_sec = sec;
	tv_usec = usec;
}

TimeVal TimeVal::operator+(TimeVal t1) {
	TimeVal t;
	t.tv_sec = tv_sec + t1.tv_sec;
	t.tv_usec = tv_usec + t1.tv_usec;
	return t;
}

TimeVal TimeVal::operator-(TimeVal t1) {
	TimeVal t;
	t.tv_sec = tv_sec - t1.tv_sec;
	t.tv_usec = tv_usec - t1.tv_usec;
	return t;
}

TimeVal TimeVal::operator/(int value) {
	TimeVal t;
	t.tv_sec = tv_sec/value;
	t.tv_usec = tv_usec/value;
	return t;
}


bool TimeVal::operator<(TimeVal t1) {
	if (tv_sec < t1.tv_sec) {
		return true;
	} else if (tv_sec == t1.tv_sec && tv_usec < t1.tv_usec) {
		return true;
	}
	return false;
}

bool TimeVal::operator>(TimeVal t1) {
	if (tv_sec > t1.tv_sec) {
		return true;
	} else if (tv_sec == t1.tv_sec && tv_usec > t1.tv_usec) {
		return true;
	}
	return false;
}

bool TimeVal::operator>=(TimeVal t1) {
	if (tv_sec > t1.tv_sec) {
		return true;
	} else if (tv_sec == t1.tv_sec && tv_usec > t1.tv_usec) {
		return true;
	} else if (tv_sec == t1.tv_sec && tv_usec == t1.tv_usec) {
		return true;
	}
	return false;
}

void TimeVal::gettime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	tv_sec = tv.tv_sec;
	tv_usec = tv.tv_usec;
}


Timer::Timer()
	: stopCount(0), totalTime(0) {
}

Timer::~Timer() {

}

void Timer::start() {
	starttv.gettime();
}

void Timer::stop() {
	TimeVal stoptv;
	stoptv.gettime();

	if (stoptv < starttv) {
		cerr << "Timer::stop() time went backwards." << endl;
		stoptv = starttv;
	}
	diff_cur = stoptv-starttv;

	// Initialize minimum and average values on first run
	if (stopCount++ == 0) {
		diff_min = diff_cur;
		diff_avg = diff_cur;
	}

	if (diff_cur < diff_min) {
		diff_min = diff_cur;
	}

	if (diff_cur > diff_max) {
		diff_max = diff_cur;
	}

	diff_avg = (diff_avg + diff_cur)/2;
	if (stopCount == 1) {
		cerr << totalTime << endl;
	}
	totalTime += (diff_cur.tv_usec/1000);
}

ostream& operator<<(ostream &out, Timer t) {
	out << "last=" << t.diff_cur << " min=" << t.diff_min
		<< " max=" << t.diff_max << " avg=" << t.diff_avg
		<< " count=" << t.stopCount << " total=" << t.totalTime << "ms";
	return out;
}
ostream& operator<<(ostream &out, TimeVal tv) {
//	out << static_cast<unsigned __time_t>(tv.tv_sec) << "s " << static_cast<unsigned __suseconds_t>(tv.tv_usec) << "µ";
	out << tv.tv_sec << "s " << tv.tv_usec << "µ";
	return out;
}
