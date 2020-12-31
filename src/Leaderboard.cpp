/*
 * Leaderboard.cpp
 *
 *  Created on: Jul 13, 2014
 *      Author: jhhudso
 */

extern "C" {
#include <string.h>
}
#include <sstream>

#include "Leaderboard.h"
#include "utility.h"
#include "structs.h"
#include "fileinfo.h"
#include "connect.h"
#include "levels.h"
#include <limits.h>
#include "db.h"
#include "interp.h"
#include "returnvals.h"

extern struct descriptor_data *descriptor_list;

using namespace std;

Leaderboard::Leaderboard() {
	memset(hpactivename, 5, sizeof(char));
	memset(mnactivename, 5, sizeof(char));
	memset(kiactivename, 5, sizeof(char));
	memset(pkactivename, 5, sizeof(char));
	memset(pdactivename, 5, sizeof(char));
	memset(rdactivename, 5, sizeof(char));
	memset(mvactivename, 5, sizeof(char));
	memset(&hpactive, 5, sizeof(int));
	memset(&mnactive, 5, sizeof(int));
	memset(&kiactive, 5, sizeof(int));
	memset(&pkactive, 5, sizeof(int));
	memset(&pdactive, 5, sizeof(int));
	memset(&rdactive, 5, sizeof(int));
	memset(&mvactive, 5, sizeof(int));
	memset(hpactiveclassname, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(mnactiveclassname, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(kiactiveclassname, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(pkactiveclassname, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(pdactiveclassname, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(rdactiveclassname, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(mvactiveclassname, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(hpactiveclass, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(mnactiveclass, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(kiactiveclass, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(pkactiveclass, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(pdactiveclass, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(rdactiveclass, (CLASS_MAX - 2) * 5, sizeof(char));
	memset(mvactiveclass, (CLASS_MAX - 2) * 5, sizeof(char));
}

Leaderboard::~Leaderboard() {
	// TODO Auto-generated destructor stub
}

void Leaderboard::check(void) {
	// check online players to the file and make sure the file is up to date
	struct descriptor_data *d;
	int i, j, k;
	extern char* curr_type;
	extern char* curr_name;
	extern int curr_virtno;

	curr_type = "leaderboard";
	curr_name = "NA";
	curr_virtno = 0;

	read_file();

	for (d = descriptor_list; d; d = d->next) {
		if (!d->character || GET_LEVEL(d->character) >= IMMORTAL
				|| IS_NPC(d->character))
			continue;
		if (!d->connected == CON_PLAYING)
			continue;
		if (!d->character->pcdata)
			continue;

		k = MIN(CLASS_DRUID - 1, GET_CLASS(d->character) - 1);

		for (i = 0; i < 5; i++) {
			if (hpactivename[i]
					&& !strcmp(hpactivename[i], GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					hpactive[j] = hpactive[j + 1];
					dc_free(hpactivename[j]);
					hpactivename[j] = str_dup0(hpactivename[j + 1]);
				}
				hpactive[4] = 0;
				dc_free(hpactivename[4]);
				hpactivename[4] = str_dup(" ");
			}
			if (mnactivename[i]
					&& !strcmp(mnactivename[i], GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					mnactive[j] = mnactive[j + 1];
					dc_free(mnactivename[j]);
					mnactivename[j] = str_dup0(mnactivename[j + 1]);
				}
				mnactive[4] = 0;
				dc_free(mnactivename[4]);
				mnactivename[4] = str_dup(" ");
			}
			if (kiactivename[i]
					&& !strcmp(kiactivename[i], GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					kiactive[j] = kiactive[j + 1];
					dc_free(kiactivename[j]);
					kiactivename[j] = str_dup0(kiactivename[j + 1]);
				}
				kiactive[4] = 0;
				dc_free(kiactivename[4]);
				kiactivename[4] = str_dup(" ");
			}
			if (pkactivename[i]
					&& !strcmp(pkactivename[i], GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					pkactive[j] = pkactive[j + 1];
					dc_free(pkactivename[j]);
					pkactivename[j] = str_dup0(pkactivename[j + 1]);
				}
				pkactive[4] = 0;
				dc_free(pkactivename[4]);
				pkactivename[4] = str_dup(" ");
			}
			if (pdactivename[i]
					&& !strcmp(pdactivename[i], GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					pdactive[j] = pdactive[j + 1];
					dc_free(pdactivename[j]);
					pdactivename[j] = str_dup0(pdactivename[j + 1]);
				}
				pdactive[4] = 0;
				dc_free(pdactivename[4]);
				pdactivename[4] = str_dup(" ");
			}
			if (rdactivename[i]
					&& !strcmp(rdactivename[i], GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					rdactive[j] = rdactive[j + 1];
					dc_free(rdactivename[j]);
					rdactivename[j] = str_dup0(rdactivename[j + 1]);
				}
				rdactive[4] = 0;
				dc_free(rdactivename[4]);
				rdactivename[4] = str_dup(" ");
			}
			if (mvactivename[i]
					&& !strcmp(mvactivename[i], GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					mvactive[j] = mvactive[j + 1];
					dc_free(mvactivename[j]);
					mvactivename[j] = str_dup0(mvactivename[j + 1]);
				}
				mvactive[4] = 0;
				dc_free(mvactivename[4]);
				mvactivename[4] = str_dup(" ");
			}
		}

		for (i = 0; i < 5; i++) {
			if (hpactiveclassname[k][i]
					&& !strcmp(hpactiveclassname[k][i],
							GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					hpactiveclass[k][j] = hpactiveclass[k][j + 1];
					dc_free(hpactiveclassname[k][j]);
					hpactiveclassname[k][j] = str_dup0(
							hpactiveclassname[k][j + 1]);
				}
				hpactiveclass[k][4] = 0;
				dc_free(hpactiveclassname[k][4]);
				hpactiveclassname[k][4] = str_dup(" ");
			}
			if (mnactiveclassname[k][i]
					&& !strcmp(mnactiveclassname[k][i],
							GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					mnactiveclass[k][j] = mnactiveclass[k][j + 1];
					dc_free(mnactiveclassname[k][j]);
					mnactiveclassname[k][j] = str_dup0(
							mnactiveclassname[k][j + 1]);
				}
				mnactiveclass[k][4] = 0;
				dc_free(mnactiveclassname[k][4]);
				mnactiveclassname[k][4] = str_dup(" ");
			}
			if (kiactiveclassname[k][i]
					&& !strcmp(kiactiveclassname[k][i],
							GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					kiactiveclass[k][j] = kiactiveclass[k][j + 1];
					dc_free(kiactiveclassname[k][j]);
					kiactiveclassname[k][j] = str_dup0(
							kiactiveclassname[k][j + 1]);
				}
				kiactiveclass[k][4] = 0;
				dc_free(kiactiveclassname[k][4]);
				kiactiveclassname[k][4] = str_dup(" ");
			}
			if (pkactiveclassname[k][i]
					&& !strcmp(pkactiveclassname[k][i],
							GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					pkactiveclass[k][j] = pkactiveclass[k][j + 1];
					dc_free(pkactiveclassname[k][j]);
					pkactiveclassname[k][j] = str_dup0(
							pkactiveclassname[k][j + 1]);
				}
				pkactiveclass[k][4] = 0;
				dc_free(pkactiveclassname[k][4]);
				pkactiveclassname[k][4] = str_dup(" ");
			}
			if (pdactiveclassname[k][i]
					&& !strcmp(pdactiveclassname[k][i],
							GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					pdactiveclass[k][j] = pdactiveclass[k][j + 1];
					dc_free(pdactiveclassname[k][j]);
					pdactiveclassname[k][j] = str_dup0(
							pdactiveclassname[k][j + 1]);
				}
				pdactiveclass[k][4] = 0;
				dc_free(pdactiveclassname[k][4]);
				pdactiveclassname[k][4] = str_dup(" ");
			}
			if (rdactiveclassname[k][i]
					&& !strcmp(rdactiveclassname[k][i],
							GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					rdactiveclass[k][j] = rdactiveclass[k][j + 1];
					dc_free(rdactiveclassname[k][j]);
					rdactiveclassname[k][j] = str_dup0(
							rdactiveclassname[k][j + 1]);
				}
				rdactiveclass[k][4] = 0;
				dc_free(rdactiveclassname[k][4]);
				rdactiveclassname[k][4] = str_dup(" ");
			}
			if (mvactiveclassname[k][i]
					&& !strcmp(mvactiveclassname[k][i],
							GET_NAME(d->character))) {
				for (j = i; j < 4; j++) {
					mvactiveclass[k][j] = mvactiveclass[k][j + 1];
					dc_free(mvactiveclassname[k][j]);
					mvactiveclassname[k][j] = str_dup0(
							mvactiveclassname[k][j + 1]);
				}
				mvactiveclass[k][4] = 0;
				dc_free(mvactiveclassname[k][4]);
				mvactiveclassname[k][4] = str_dup(" ");
			}
		}

		for (i = 0; i < 5; i++) {
			if (GET_MAX_HIT(d->character) > hpactive[i]) {
				for (j = 4; j > i; j--) {
					hpactive[j] = hpactive[j - 1];
					dc_free(hpactivename[j]);
					hpactivename[j] = str_dup0(hpactivename[j - 1]);
				}
				hpactive[i] = GET_MAX_HIT(d->character);
				dc_free(hpactivename[i]);
				hpactivename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_MANA(d->character) > mnactive[i]) {
				for (j = 4; j > i; j--) {
					mnactive[j] = mnactive[j - 1];
					dc_free(mnactivename[j]);
					mnactivename[j] = str_dup0(mnactivename[j - 1]);
				}
				mnactive[i] = GET_MAX_MANA(d->character);
				dc_free(mnactivename[i]);
				mnactivename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_KI(d->character) > kiactive[i]) {
				for (j = 4; j > i; j--) {
					kiactive[j] = kiactive[j - 1];
					dc_free(kiactivename[j]);
					kiactivename[j] = str_dup0(kiactivename[j - 1]);
				}
				kiactive[i] = GET_MAX_KI(d->character);
				dc_free(kiactivename[i]);
				kiactivename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if ((int) GET_PKILLS(d->character) > pkactive[i]) {
				for (j = 4; j > i; j--) {
					pkactive[j] = pkactive[j - 1];
					dc_free(pkactivename[j]);
					pkactivename[j] = str_dup0(pkactivename[j - 1]);
				}
				pkactive[i] = (int) GET_PKILLS(d->character);
				dc_free(pkactivename[i]);
				pkactivename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (pdscore(d->character) > pdactive[i]) {
				for (j = 4; j > i; j--) {
					pdactive[j] = pdactive[j - 1];
					dc_free(pdactivename[j]);
					pdactivename[j] = str_dup0(pdactivename[j - 1]);
				}
				pdactive[i] = pdscore(d->character);
				dc_free(pdactivename[i]);
				pdactivename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_LEVEL(d->character) < MAX_MORTAL)
				break;
			if ((int) GET_RDEATHS(d->character) > rdactive[i]) {
				for (j = 4; j > i; j--) {
					rdactive[j] = rdactive[j - 1];
					dc_free(rdactivename[j]);
					rdactivename[j] = str_dup0(rdactivename[j - 1]);
				}
				rdactive[i] = (int) GET_RDEATHS(d->character);
				dc_free(rdactivename[i]);
				rdactivename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_MOVE(d->character) > mvactive[i]) {
				for (j = 4; j > i; j--) {
					mvactive[j] = mvactive[j - 1];
					dc_free(mvactivename[j]);
					mvactivename[j] = str_dup0(mvactivename[j - 1]);
				}
				mvactive[i] = GET_MAX_MOVE(d->character);
				dc_free(mvactivename[i]);
				mvactivename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_HIT(d->character) > hpactiveclass[k][i]) {
				for (j = 4; j > i; j--) {
					hpactiveclass[k][j] = hpactiveclass[k][j - 1];
					dc_free(hpactiveclassname[k][j]);
					hpactiveclassname[k][j] = str_dup0(
							hpactiveclassname[k][j - 1]);
				}
				hpactiveclass[k][i] = GET_MAX_HIT(d->character);
				dc_free(hpactiveclassname[k][i]);
				hpactiveclassname[k][i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_MANA(d->character) > mnactiveclass[k][i]) {
				for (j = 4; j > i; j--) {
					mnactiveclass[k][j] = mnactiveclass[k][j - 1];
					dc_free(mnactiveclassname[k][j]);
					mnactiveclassname[k][j] = str_dup0(
							mnactiveclassname[k][j - 1]);
				}
				mnactiveclass[k][i] = GET_MAX_MANA(d->character);
				dc_free(mnactiveclassname[k][i]);
				mnactiveclassname[k][i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_KI(d->character) > kiactiveclass[k][i]) {
				for (j = 4; j > i; j--) {
					kiactiveclass[k][j] = kiactiveclass[k][j - 1];
					dc_free(kiactiveclassname[k][j]);
					kiactiveclassname[k][j] = str_dup0(
							kiactiveclassname[k][j - 1]);
				}
				kiactiveclass[k][i] = GET_MAX_KI(d->character);
				dc_free(kiactiveclassname[k][i]);
				kiactiveclassname[k][i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if ((int) GET_PKILLS(d->character) > pkactiveclass[k][i]) {
				for (j = 4; j > i; j--) {
					pkactiveclass[k][j] = pkactiveclass[k][j - 1];
					dc_free(pkactiveclassname[k][j]);
					pkactiveclassname[k][j] = str_dup0(
							pkactiveclassname[k][j - 1]);
				}
				pkactiveclass[k][i] = (int) GET_PKILLS(d->character);
				dc_free(pkactiveclassname[k][i]);
				pkactiveclassname[k][i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (pdscore(d->character) > pdactiveclass[k][i]) {
				for (j = 4; j > i; j--) {
					pdactiveclass[k][j] = pdactiveclass[k][j - 1];
					dc_free(pdactiveclassname[k][j]);
					pdactiveclassname[k][j] = str_dup0(
							pdactiveclassname[k][j - 1]);
				}
				pdactiveclass[k][i] = pdscore(d->character);
				dc_free(pdactiveclassname[k][i]);
				pdactiveclassname[k][i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_LEVEL(d->character) < MAX_MORTAL)
				break;
			if ((int) GET_RDEATHS(d->character) > rdactiveclass[k][i]) {
				for (j = 4; j > i; j--) {
					rdactiveclass[k][j] = rdactiveclass[k][j - 1];
					dc_free(rdactiveclassname[k][j]);
					rdactiveclassname[k][j] = str_dup0(
							rdactiveclassname[k][j - 1]);
				}
				rdactiveclass[k][i] = (int) GET_RDEATHS(d->character);
				dc_free(rdactiveclassname[k][i]);
				rdactiveclassname[k][i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_MOVE(d->character) > mvactiveclass[k][i]) {
				for (j = 4; j > i; j--) {
					mvactiveclass[k][j] = mvactiveclass[k][j - 1];
					dc_free(mvactiveclassname[k][j]);
					mvactiveclassname[k][j] = str_dup0(
							mvactiveclassname[k][j - 1]);
				}
				mvactiveclass[k][i] = GET_MAX_MOVE(d->character);
				dc_free(mvactiveclassname[k][i]);
				mvactiveclassname[k][i] = str_dup(GET_NAME(d->character));
				break;
			}
		}

	}

	write_file(LEADERBOARD_FILE);

  in_port_t port1 = 0;
  if (DC::instance().cf.ports.size() > 0)
  {
    port1 = DC::instance().cf.ports[0];
  }

  stringstream ssbuffer;
	ssbuffer << HTDOCS_DIR << port1 << "/" << LEADERBOARD_FILE;
	write_file(ssbuffer);

	for (i = 0; i < 5; i++)
		dc_free(hpactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(mnactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(kiactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(pkactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(pdactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(rdactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(mvactivename[i]);
	for (j = 0; j < CLASS_MAX - 2; j++) {
		for (i = 0; i < 5; i++)
			dc_free(hpactiveclassname[j][i]);
		for (i = 0; i < 5; i++)
			dc_free(mnactiveclassname[j][i]);
		for (i = 0; i < 5; i++)
			dc_free(kiactiveclassname[j][i]);
		for (i = 0; i < 5; i++)
			dc_free(pkactiveclassname[j][i]);
		for (i = 0; i < 5; i++)
			dc_free(pdactiveclassname[j][i]);
		for (i = 0; i < 5; i++)
			dc_free(rdactiveclassname[j][i]);
		for (i = 0; i < 5; i++)
			dc_free(mvactiveclassname[j][i]);
	}
}

void Leaderboard::read_file(void) {
	FILE *fl;
	int i, j;

	if (!(fl = dc_fopen(LEADERBOARD_FILE, "r"))) {
		logf(0, LOG_BUG, "Cannot open leaderboard file '%s'", LEADERBOARD_FILE);
	} else {
		try {
			for (i = 0; i < 5; i++) {
				hpactivename[i] = fread_string(fl, 0);
				hpactive[i] = fread_int(fl, 0, LONG_MAX);
				if (char_file_exists(hpactivename[i]) == false) {
					hpactivename[i] = str_dup("UNKNOWN");
				}
			}
			for (i = 0; i < 5; i++) {
				mnactivename[i] = fread_string(fl, 0);
				mnactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				kiactivename[i] = fread_string(fl, 0);
				kiactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				pkactivename[i] = fread_string(fl, 0);
				pkactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				pdactivename[i] = fread_string(fl, 0);
				pdactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				rdactivename[i] = fread_string(fl, 0);
				rdactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				mvactivename[i] = fread_string(fl, 0);
				mvactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (j = 0; j < CLASS_MAX - 2; j++) {
				for (i = 0; i < 5; i++) {
					hpactiveclassname[j][i] = fread_string(fl, 0);
					hpactiveclass[j][i] = fread_int(fl, 0, LONG_MAX);
				}
				for (i = 0; i < 5; i++) {
					mnactiveclassname[j][i] = fread_string(fl, 0);
					mnactiveclass[j][i] = fread_int(fl, 0, LONG_MAX);
				}
				for (i = 0; i < 5; i++) {
					kiactiveclassname[j][i] = fread_string(fl, 0);
					kiactiveclass[j][i] = fread_int(fl, 0, LONG_MAX);
				}
				for (i = 0; i < 5; i++) {
					pkactiveclassname[j][i] = fread_string(fl, 0);
					pkactiveclass[j][i] = fread_int(fl, 0, LONG_MAX);
				}
				for (i = 0; i < 5; i++) {
					pdactiveclassname[j][i] = fread_string(fl, 0);
					pdactiveclass[j][i] = fread_int(fl, 0, LONG_MAX);
				}
				for (i = 0; i < 5; i++) {
					rdactiveclassname[j][i] = fread_string(fl, 0);
					rdactiveclass[j][i] = fread_int(fl, 0, LONG_MAX);
				}
				for (i = 0; i < 5; i++) {
					mvactiveclassname[j][i] = fread_string(fl, 0);
					mvactiveclass[j][i] = fread_int(fl, 0, LONG_MAX);
				}
			}
		} catch (error_eof &) {
			logf(0, LOG_BUG, "Corrupt leaderboard file '%s': eof reached prematurely", LEADERBOARD_FILE);
		} catch (error_negative_int &) {
			logf(0, LOG_BUG, "Corrupt leaderboard file '%s': negative int found where positive expected", LEADERBOARD_FILE);
		}

		dc_fclose(fl);
	}
}

void Leaderboard::write_file(std::stringstream &filename) {
	write_file(filename.str().c_str());
}

void Leaderboard::write_file(std::string &filename) {
	write_file(filename.c_str());
}

void Leaderboard::write_file(const char filename[]) {
	FILE *fl;
	int i, j;

	if (!(fl = dc_fopen(filename, "w"))) {
		logf(0, LOG_BUG, "Cannot open leaderboard file '%s'", filename);
		return;
	}
	for (i = 0; i < 5; i++)
		fprintf(fl, "%s~ %d\n", hpactivename[i], hpactive[i]);
	for (i = 0; i < 5; i++)
		fprintf(fl, "%s~ %d\n", mnactivename[i], mnactive[i]);
	for (i = 0; i < 5; i++)
		fprintf(fl, "%s~ %d\n", kiactivename[i], kiactive[i]);
	for (i = 0; i < 5; i++)
		fprintf(fl, "%s~ %d\n", pkactivename[i], pkactive[i]);
	for (i = 0; i < 5; i++)
		fprintf(fl, "%s~ %d\n", pdactivename[i], pdactive[i]);
	for (i = 0; i < 5; i++)
		fprintf(fl, "%s~ %d\n", rdactivename[i], rdactive[i]);
	for (i = 0; i < 5; i++)
		fprintf(fl, "%s~ %d\n", mvactivename[i], mvactive[i]);
	for (j = 0; j < CLASS_MAX - 2; j++) {
		for (i = 0; i < 5; i++)
			fprintf(fl, "%s~ %d\n", hpactiveclassname[j][i],
					hpactiveclass[j][i]);
		for (i = 0; i < 5; i++)
			fprintf(fl, "%s~ %d\n", mnactiveclassname[j][i],
					mnactiveclass[j][i]);
		for (i = 0; i < 5; i++)
			fprintf(fl, "%s~ %d\n", kiactiveclassname[j][i],
					kiactiveclass[j][i]);
		for (i = 0; i < 5; i++)
			fprintf(fl, "%s~ %d\n", pkactiveclassname[j][i],
					pkactiveclass[j][i]);
		for (i = 0; i < 5; i++)
			fprintf(fl, "%s~ %d\n", pdactiveclassname[j][i],
					pdactiveclass[j][i]);
		for (i = 0; i < 5; i++)
			fprintf(fl, "%s~ %d\n", rdactiveclassname[j][i],
					rdactiveclass[j][i]);
		for (i = 0; i < 5; i++)
			fprintf(fl, "%s~ %d\n", mvactiveclassname[j][i],
					mvactiveclass[j][i]);
	}
	dc_fclose(fl);
}

int Leaderboard::pdscore(CHAR_DATA * ch) {
	return ch->pcdata->pdeaths;
}

/*
 If you add anything to this function (more displays) make sure you change
 the lines = ## * (CLASS_MAX-1) to equal the number of added leaderboard
 things, otherwise renames will crash the server hard.
 */

int do_leaderboard(struct char_data *ch, char *argument, int cmd) {
	struct descriptor_data *d;
	FILE *fl;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int i, j, k, validclass = 0;
	char *hponlinename[5], *mnonlinename[5], *kionlinename[5], *pkonlinename[5],
			*pdonlinename[5], *rdonlinename[5], *mvonlinename[5];
	int hponline[] = { 0, 0, 0, 0, 0 }, mnonline[] = { 0, 0, 0, 0, 0 },
			kionline[] = { 0, 0, 0, 0, 0 }, pkonline[] = { 0, 0, 0, 0, 0 },
			pdonline[] = { 0, 0, 0, 0, 0 }, rdonline[] = { 0, 0, 0, 0, 0 },
			mvonline[] = { 0, 0, 0, 0, 0 };
	char *hpactivename[5], *mnactivename[5], *kiactivename[5], *pkactivename[5],
			*pdactivename[5], *rdactivename[5], *mvactivename[5];
	int hpactive[5], mnactive[5], kiactive[5], pkactive[5], pdactive[5],
			rdactive[5], mvactive[5];
	int placea = 1, placeb = 1, placec = 1, placed = 1;
	int skippeda = 0, skippedb = 0, skippedc = 0, skippedd = 0;
	char *clss_types[] = { "mage", "cleric", "thief", "warrior", "antipaladin",
			"paladin", "barbarian", "monk", "ranger", "bard", "druid", "\n" };

	leaderboard.check();

	for (i = 0; i < 5; i++)
		hponlinename[i] = str_dup(" ");
	for (i = 0; i < 5; i++)
		mnonlinename[i] = str_dup(" ");
	for (i = 0; i < 5; i++)
		kionlinename[i] = str_dup(" ");
	for (i = 0; i < 5; i++)
		pkonlinename[i] = str_dup(" ");
	for (i = 0; i < 5; i++)
		pdonlinename[i] = str_dup(" ");
	for (i = 0; i < 5; i++)
		rdonlinename[i] = str_dup(" ");
	for (i = 0; i < 5; i++)
		mvonlinename[i] = str_dup(" ");

	one_argument(argument, buf);
	for (k = 0; k < 11; k++) {
		if (is_abbrev(buf, clss_types[k])) {
			validclass = 1;
			break;
		}
	}

	if (!(fl = dc_fopen(LEADERBOARD_FILE, "r"))) {
		logf(0, LOG_BUG, "Cannot open leaderboard file '%s'", LEADERBOARD_FILE);
		return eFAILURE;
	}
	for (i = 0; i < 5; i++) {
		hpactivename[i] = fread_string(fl, 0);
		hpactive[i] = fread_int(fl, 0, LONG_MAX);
	}
	for (i = 0; i < 5; i++) {
		mnactivename[i] = fread_string(fl, 0);
		mnactive[i] = fread_int(fl, 0, LONG_MAX);
	}
	for (i = 0; i < 5; i++) {
		kiactivename[i] = fread_string(fl, 0);
		kiactive[i] = fread_int(fl, 0, LONG_MAX);
	}
	for (i = 0; i < 5; i++) {
		pkactivename[i] = fread_string(fl, 0);
		pkactive[i] = fread_int(fl, 0, LONG_MAX);
	}
	for (i = 0; i < 5; i++) {
		pdactivename[i] = fread_string(fl, 0);
		pdactive[i] = fread_int(fl, 0, LONG_MAX);
	}
	for (i = 0; i < 5; i++) {
		rdactivename[i] = fread_string(fl, 0);
		rdactive[i] = fread_int(fl, 0, LONG_MAX);
	}
	for (i = 0; i < 5; i++) {
		mvactivename[i] = fread_string(fl, 0);
		mvactive[i] = fread_int(fl, 0, LONG_MAX);
	}
	if (validclass) {
		for (j = 0; j < k + 1; j++) {
			for (i = 0; i < 5; i++)
				dc_free(hpactivename[i]);
			for (i = 0; i < 5; i++)
				dc_free(mnactivename[i]);
			for (i = 0; i < 5; i++)
				dc_free(kiactivename[i]);
			for (i = 0; i < 5; i++)
				dc_free(pkactivename[i]);
			for (i = 0; i < 5; i++)
				dc_free(pdactivename[i]);
			for (i = 0; i < 5; i++)
				dc_free(rdactivename[i]);
			for (i = 0; i < 5; i++)
				dc_free(mvactivename[i]);
			for (i = 0; i < 5; i++) {
				hpactivename[i] = fread_string(fl, 0);
				hpactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				mnactivename[i] = fread_string(fl, 0);
				mnactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				kiactivename[i] = fread_string(fl, 0);
				kiactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				pkactivename[i] = fread_string(fl, 0);
				pkactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				pdactivename[i] = fread_string(fl, 0);
				pdactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				rdactivename[i] = fread_string(fl, 0);
				rdactive[i] = fread_int(fl, 0, LONG_MAX);
			}
			for (i = 0; i < 5; i++) {
				mvactivename[i] = fread_string(fl, 0);
				mvactive[i] = fread_int(fl, 0, LONG_MAX);
			}
		}
	}
	dc_fclose(fl);

	//top 5 online
	for (d = descriptor_list; d; d = d->next) {

		if (!d->character || GET_LEVEL(d->character) >= IMMORTAL)
			continue;
		if (!d->connected == CON_PLAYING)
			continue;
		if (!d->character->pcdata)
			continue;
		if (!CAN_SEE(ch, d->character))
			continue;

		if (validclass && GET_CLASS(d->character) != k + 1)
			continue;

		for (i = 0; i < 5; i++) {
			if (GET_MAX_HIT(d->character) > hponline[i]) {
				for (j = 4; j > i; j--) {
					hponline[j] = hponline[j - 1];
					dc_free(hponlinename[j]);
					hponlinename[j] = str_dup(hponlinename[j - 1]);
				}
				hponline[i] = GET_MAX_HIT(d->character);
				dc_free(hponlinename[i]);
				hponlinename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_MANA(d->character) > mnonline[i]) {
				for (j = 4; j > i; j--) {
					mnonline[j] = mnonline[j - 1];
					dc_free(mnonlinename[j]);
					mnonlinename[j] = str_dup(mnonlinename[j - 1]);
				}
				mnonline[i] = GET_MAX_MANA(d->character);
				dc_free(mnonlinename[i]);
				mnonlinename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_KI(d->character) > kionline[i]) {
				for (j = 4; j > i; j--) {
					kionline[j] = kionline[j - 1];
					dc_free(kionlinename[j]);
					kionlinename[j] = str_dup(kionlinename[j - 1]);
				}
				kionline[i] = GET_MAX_KI(d->character);
				dc_free(kionlinename[i]);
				kionlinename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if ((int) GET_PKILLS(d->character) > pkonline[i]) {
				for (j = 4; j > i; j--) {
					pkonline[j] = pkonline[j - 1];
					dc_free(pkonlinename[j]);
					pkonlinename[j] = str_dup(pkonlinename[j - 1]);
				}
				pkonline[i] = (int) GET_PKILLS(d->character);
				dc_free(pkonlinename[i]);
				pkonlinename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (leaderboard.pdscore(d->character) > pdonline[i]) {
				for (j = 4; j > i; j--) {
					pdonline[j] = pdonline[j - 1];
					dc_free(pdonlinename[j]);
					pdonlinename[j] = str_dup(pdonlinename[j - 1]);
				}
				pdonline[i] = leaderboard.pdscore(d->character);
				dc_free(pdonlinename[i]);
				pdonlinename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_LEVEL(d->character) < MAX_MORTAL)
				break;
			if ((int) GET_RDEATHS(d->character) > rdonline[i]) {
				for (j = 4; j > i; j--) {
					rdonline[j] = rdonline[j - 1];
					dc_free(rdonlinename[j]);
					rdonlinename[j] = str_dup(rdonlinename[j - 1]);
				}
				rdonline[i] = (int) GET_RDEATHS(d->character);
				dc_free(rdonlinename[i]);
				rdonlinename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
		for (i = 0; i < 5; i++) {
			if (GET_MAX_MOVE(d->character) > mvonline[i]) {
				for (j = 4; j > i; j--) {
					mvonline[j] = mvonline[j - 1];
					dc_free(mvonlinename[j]);
					mvonlinename[j] = str_dup(mvonlinename[j - 1]);
				}
				mvonline[i] = GET_MAX_MOVE(d->character);
				dc_free(mvonlinename[i]);
				mvonlinename[i] = str_dup(GET_NAME(d->character));
				break;
			}
		}
	}
	sprintf(buf,
			"(*)**************************************************************************(*)\r\n");
	strcat(buf,
			"(*)                          $BDark Castle Leaderboard$R                         (*)\r\n");
	if (validclass) {
		k != 2 ?
				sprintf(buf2,
						"(*)                             $Bfor %11ss$R                             (*)\r\n",
						clss_types[k]) :
				sprintf(buf2,
						"(*)                             $Bfor      thieves$R                             (*)\r\n");
		strcat(buf, buf2);
	}
	strcat(buf,
			"(*)--------------------------------------------------------------------------(*)\r\n");
	strcat(buf,
			"(*)                                                                          (*)\r\n");
	strcat(buf,
			"(*)    Online         All Time                Online        All Time         (*)\r\n");
	strcat(buf,
			"(*)                                                                          (*)\r\n");
	strcat(buf,
			"(*)            $2$BHit Points                               Mana$R                 (*)\r\n");
	sprintf(buf2,
			"(*) 1) $5$B%-12s$R1) $5$B%-12s$R        1) $5$B%-12s$R1) $5$B%-12s$R     (*)\r\n",
			hponlinename[0], hpactivename[0], mnonlinename[0], mnactivename[0]);
	strcat(buf, buf2);
	for (i = 1; i < 5; i++) {
		if (hponline[i] != hponline[i - 1]) {
			placea += ++skippeda;
			skippeda = 0;
		} else
			skippeda++;
		if (hpactive[i] != hpactive[i - 1]) {
			placeb += ++skippedb;
			skippedb = 0;
		} else
			skippedb++;
		if (mnonline[i] != mnonline[i - 1]) {
			placec += ++skippedc;
			skippedc = 0;
		} else
			skippedc++;
		if (mnactive[i] != mnactive[i - 1]) {
			placed += ++skippedd;
			skippedd = 0;
		} else
			skippedd++;
		sprintf(buf2,
				"(*) %d) $B%-12s$R%d) $B%-12s$R-%-4d   %d) $B%-12s$R%d) $B%-12s$R-%-4d(*)\r\n",
				placea, hponlinename[i], placeb, hpactivename[i],
				hpactive[0] - hpactive[i], placec, mnonlinename[i], placed,
				mnactivename[i], mnactive[0] - mnactive[i]);
		strcat(buf, buf2);
	}
	placea = 1;
	placeb = 1;
	placec = 1;
	placed = 1;
	skippeda = 0;
	skippedb = 0;
	skippedc = 0;
	skippedd = 0;
	strcat(buf,
			"(*)                                                                          (*)\r\n");
	strcat(buf,
			"(*)                $2$BKi                                Movement        $R        (*)\r\n");
	sprintf(buf2,
			"(*) 1) $5$B%-12s$R1) $5$B%-12s$R        1) $5$B%-12s$R1) $5$B%-12s$R     (*)\r\n",
			kionlinename[0], kiactivename[0], mvonlinename[0], mvactivename[0]);
	strcat(buf, buf2);
	for (i = 1; i < 5; i++) {
		if (kionline[i] != kionline[i - 1]) {
			placea += ++skippeda;
			skippeda = 0;
		} else
			skippeda++;
		if (kiactive[i] != kiactive[i - 1]) {
			placeb += ++skippedb;
			skippedb = 0;
		} else
			skippedb++;
		if (mvonline[i] != mvonline[i - 1]) {
			placec += ++skippedc;
			skippedc = 0;
		} else
			skippedc++;
		if (mvactive[i] != mvactive[i - 1]) {
			placed += ++skippedd;
			skippedd = 0;
		} else
			skippedd++;
		sprintf(buf2,
				"(*) %d) $B%-12s$R%d) $B%-12s$R-%-4d   %d) $B%-12s$R%d) $B%-12s$R-%-4d(*)\r\n",
				placea, kionlinename[i], placeb, kiactivename[i],
				kiactive[0] - kiactive[i], placec, mvonlinename[i], placed,
				mvactivename[i], mvactive[0] - mvactive[i]);
		strcat(buf, buf2);
	}
	placea = 1;
	placeb = 1;
	placec = 1;
	placed = 1;
	skippeda = 0;
	skippedb = 0;
	skippedc = 0;
	skippedd = 0;
	strcat(buf,
			"(*)                                                                          (*)\r\n");
	strcat(buf,
			"(*)         $2$BPlayer Kill Score                   Player Death Score       $R    (*)\r\n");
	sprintf(buf2,
			"(*) 1) $5$B%-12s$R1) $5$B%-12s$R        1) $5$B%-12s$R1) $5$B%-12s$R     (*)\r\n",
			pkonlinename[0], pkactivename[0], pdonlinename[0], pdactivename[0]);
	strcat(buf, buf2);
	for (i = 1; i < 5; i++) {
		if (pkonline[i] != pkonline[i - 1]) {
			placea += ++skippeda;
			skippeda = 0;
		} else
			skippeda++;
		if (pkactive[i] != pkactive[i - 1]) {
			placeb += ++skippedb;
			skippedb = 0;
		} else
			skippedb++;
		if (pdonline[i] != pdonline[i - 1]) {
			placec += ++skippedc;
			skippedc = 0;
		} else
			skippedc++;
		if (pdactive[i] != pdactive[i - 1]) {
			placed += ++skippedd;
			skippedd = 0;
		} else
			skippedd++;
		sprintf(buf2,
				"(*) %d) $B%-12s$R%d) $B%-12s$R-%-4d   %d) $B%-12s$R%d) $B%-12s$R-%-4d(*)\r\n",
				placea, pkonlinename[i], placeb, pkactivename[i],
				pkactive[0] - pkactive[i], placec, pdonlinename[i], placed,
				pdactivename[i], pdactive[0] - pdactive[i]);
		strcat(buf, buf2);
	}
//FROM HERE
	placea = 1;
	placeb = 1;
	placec = 1;
	placed = 1;
	skippeda = 0;
	skippedb = 0;
	skippedc = 0;
	skippedd = 0;
	strcat(buf,
			"(*)                                                                          (*)\r\n");
	strcat(buf,
			"(*)        $2$BReal Deaths (Level 60)                                      $R      (*)\r\n");
	sprintf(buf2,
			"(*) 1) $5$B%-12s$R1) $5$B%-12s$R                                           (*)\r\n",
			rdonlinename[0], rdactivename[0]);
	strcat(buf, buf2);
	for (i = 1; i < 5; i++) {
		if (rdonline[i] != rdonline[i - 1]) {
			placea += ++skippeda;
			skippeda = 0;
		} else
			skippeda++;
		if (rdactive[i] != rdactive[i - 1]) {
			placeb += ++skippedb;
			skippedb = 0;
		} else
			skippedb++;
		//   if(rdonline[i] != rdonline[i-1]) {
		//    placec += ++skippedc;
		//    skippedc = 0;
		// }
		// else skippedc++;
		//  if(rdactive[i] != rdactive[i-1]) {
		//      placed += ++skippedd;
		//       skippedd = 0;
		//     }
//      else skippedd++;
		sprintf(buf2,
				"(*) %d) $B%-12s$R%d) $B%-12s$R-%-4d                                      (*)\r\n",
				placea, rdonlinename[i], placeb, rdactivename[i],
				rdactive[0] - rdactive[i]);
		strcat(buf, buf2);
	}

//TO HERE
	strcat(buf,
			"(*)                                                                          (*)\r\n");
	strcat(buf,
			"(*)--------------------------------------------------------------------------(*)\r\n");
	strcat(buf,
			"(*)**************************************************************************(*)\r\n");
	page_string(ch->desc, buf, 1);
	for (i = 0; i < 5; i++)
		dc_free(hponlinename[i]);
	for (i = 0; i < 5; i++)
		dc_free(mnonlinename[i]);
	for (i = 0; i < 5; i++)
		dc_free(kionlinename[i]);
	for (i = 0; i < 5; i++)
		dc_free(pkonlinename[i]);
	for (i = 0; i < 5; i++)
		dc_free(pdonlinename[i]);
	for (i = 0; i < 5; i++)
		dc_free(rdonlinename[i]);
	for (i = 0; i < 5; i++)
		dc_free(mvonlinename[i]);
	for (i = 0; i < 5; i++)
		dc_free(hpactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(mnactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(kiactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(pkactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(pdactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(rdactivename[i]);
	for (i = 0; i < 5; i++)
		dc_free(mvactivename[i]);

	return eSUCCESS;
}

void Leaderboard::rename(char *oldname, char *newname) {
	FILE *fl;
	//lines is the number of lines rewritten back to leaderboard file
	//after a rename.. must sync up with # of outputs
	int lines = 35 * (CLASS_MAX - 1);
	int value[lines], i;
	char *name[lines];

	if (DC::instance().cf.bport) {
		return;
	}

	if (!(fl = dc_fopen(LEADERBOARD_FILE, "r"))) {
		logf(0, LOG_BUG, "Cannot open leaderboard file: %s", LEADERBOARD_FILE);
		abort();
	}
	for (i = 0; i < lines; i++) {
		name[i] = fread_string(fl, 0);
		value[i] = fread_int(fl, 0, LONG_MAX);
	}
	dc_fclose(fl);

	for (i = 0; i < lines; i++) {
		if (!strcmp(name[i], oldname)) {
			dc_free(name[i]);
			name[i] = str_dup(newname);
		}
	}

	if (!(fl = dc_fopen(LEADERBOARD_FILE, "w"))) {
		logf(0, LOG_BUG, "Cannot open leaderboard file: %s", LEADERBOARD_FILE);
		abort();
	}

	for (i = 0; i < lines; i++)
		fprintf(fl, "%s~ %d\n", name[i], value[i]);
	dc_fclose(fl);

	for (i = 0; i < lines; i++)
		dc_free(name[i]);
}

Leaderboard leaderboard;
