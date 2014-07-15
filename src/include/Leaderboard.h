/*
 * Leaderboard.h
 *
 *  Created on: Jul 13, 2014
 *      Author: jhhudso
 */

#ifndef LEADERBOARD_H_
#define LEADERBOARD_H_

extern "C" {
#include <stdio.h>
}

#include <sstream>

#include <player.h>

class Leaderboard {
public:
	Leaderboard();
	virtual ~Leaderboard();
	void check(void);
	void read_file(void);
	void write_file(std::string &filename);
	void write_file(std::stringstream &filename);
	void write_file(const char filename[]);
	int pdscore(CHAR_DATA * ch);
	void rename(char *oldname, char *newname);
private:
	char *hpactivename[5];
	char *mnactivename[5];
	char *kiactivename[5];
	char *pkactivename[5];
	char *pdactivename[5];
	char *rdactivename[5];
	char *mvactivename[5];
	int hpactive[5];
	int mnactive[5];
	int kiactive[5];
	int pkactive[5];
	int pdactive[5];
	int rdactive[5];
	int mvactive[5];
	char *hpactiveclassname[CLASS_MAX - 2][5];
	char *mnactiveclassname[CLASS_MAX - 2][5];
	char *kiactiveclassname[CLASS_MAX - 2][5];
	char *pkactiveclassname[CLASS_MAX - 2][5];
	char *pdactiveclassname[CLASS_MAX - 2][5];
	char *rdactiveclassname[CLASS_MAX - 2][5];
	char *mvactiveclassname[CLASS_MAX - 2][5];
	int hpactiveclass[CLASS_MAX - 2][5];
	int mnactiveclass[CLASS_MAX - 2][5];
	int kiactiveclass[CLASS_MAX - 2][5];
	int pkactiveclass[CLASS_MAX - 2][5];
	int pdactiveclass[CLASS_MAX - 2][5];
	int rdactiveclass[CLASS_MAX - 2][5];
	int mvactiveclass[CLASS_MAX - 2][5];
};

extern Leaderboard leaderboard;

#endif /* LEADERBOARD_H_ */
