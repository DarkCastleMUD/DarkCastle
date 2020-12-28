// System C headers
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// System C++ headers
#include <sstream>
#include <iostream>

// DC headers
#include "fileinfo.h"
#include "utility.h"
#include "DC.h"

using namespace std;

extern char **orig_argv;
extern short code_testing_mode;
extern short code_testing_mode_mob;
extern short code_testing_mode_world;
extern short bport;
extern bool allow_imp_password;
extern uint16_t port1, port2, port3, port4;

bool verbose_mode = FALSE;
int scheck = 0; /* for syntax checking mode */
uint16_t DFLT_PORT = 6667, DFLT_PORT2 = 6666, DFLT_PORT3 = 4000, DFLT_PORT4 = 6669;
CVoteData *DCVote;

void init_random();
void init_game(void);
void boot_zones(void);
void boot_world(void);

/**********************************************************************
 *  main game loop and related stuff                                  *
 **********************************************************************/

int main(int argc, char **argv) {
	struct stat stat_buffer;
	char char_buffer[512] = { '\0' };

	logf(0, LOG_MISC, "Executable: %s Version: %s Build date: %s\n", argv[0], DC::getVersion().c_str(), DC::getBuildTime().c_str());

#ifndef __CYGWIN__
	// Make a copy of our executable so that in the event of a crash we have a
	// known good copy to debug with.
	stringstream backup_filename, cmd;
	backup_filename << argv[0] << "." << DC::getVersion().c_str() << "." << getpid();

	// If backup file does not exist already then link to it
	if (stat(backup_filename.str().c_str(), &stat_buffer) == -1) {
		if (link(argv[0], backup_filename.str().c_str()) == -1) {
			const char * strerror_result = strerror_r(errno, char_buffer, sizeof(char_buffer));
			logf(0, LOG_MISC, "Error linking %s to %s: %s", argv[0], backup_filename.str().c_str(), strerror_result);
		}
	}
#endif

	orig_argv = argv;
	int pos = 1;
	char dir[256] = { '\0' };
	strncpy(dir, (char *) DFLT_DIR, 255);

	init_random();

	port1 = DFLT_PORT;
	port2 = DFLT_PORT2;
	port3 = DFLT_PORT3;
	port4 = DFLT_PORT4;
	//  ext_argv = argv;

	while ((pos < argc) && (*(argv[pos]) == '-')) {
		switch (*(argv[pos] + 1)) {
		case 'v':
			verbose_mode = 1;
			break;
		case 'h':
		case '?':
			fprintf(stderr,
					"Usage: %s [-v] [-h] [-w] [-c] [-m] [-d directory] [-p] [-P]\n"
							"-v\t\tVerbose mode\n"
							"-h\t\tUsage information\n"
							"-w\t\tWorld testing mode\n"
							"-c\t\tObj testing mode\n"
							"-m\t\tObj & Mob testing mode\n"
							"-b\t\tBuilders' port (7000-7003)\n"
							"-d directory\tData directory\n"
							"-p\t\tUse port 1500-1503\n"
							"-P\t\tAllow imps to use their password\n\n"
							"-s\t\tSyntax checking mode\n",
					argv[0]);

			return 0;
			break;
		case 'w':
			code_testing_mode = 1;
			code_testing_mode_world = 1;
			log("Mud in world checking mode. TinyTinyworld being used. (WLD)",
					0, LOG_MISC);
			log("Do NOT have mortals login when in world checking mode.", 0,
					LOG_MISC);
			break;

		case 'c':
			code_testing_mode = 1;
			log("Mud in testing mode. TinyTinyworld being used. (OBJ)", 0,
					LOG_MISC);
			break;

		case 'm':
			code_testing_mode = 1;
			code_testing_mode_mob = 1;
			log("Mud in testing mode. TinyTinyworld being used. (MOB,OBJ)", 0,
					LOG_MISC);
			break;
		case 'b': // Buildin' port.
			bport = 1;
			port1 = 7000;
			port2 = 7001;
			port3 = 7002;
			port4 = 7003;
			break;
		case 'd':
			if (argv[pos][2] != '\0')
				strcpy(dir, &argv[pos][2]);
			else if (++pos < argc)
				strcpy(dir, &argv[pos][0]);
			else {
				fprintf(stderr, "Directory arg expected after -d.\n\r");
				exit(1);
			}
			break;
		case 'p':
			port1 = 1500;
			port2 = 1501;
			port3 = 1502;
			port4 = 1503;
			break;
		case 'P':
			allow_imp_password = true;
			break;
		case 's':
			scheck = true;
			break;
		default:
			logf(0, LOG_MISC, "SYSERR: Unknown option -%c in argument string.",
					*(argv[pos] + 1));
			break;
		}
		pos++;
	}

	if (pos < argc) {
		if (!isdigit(*argv[pos])) {
			fprintf(stderr,
					"Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n",
					argv[0]);
			exit(1);
		} else if ((port1 = atoi(argv[pos])) <= 1024) {
			fprintf(stderr, "Illegal port number.\n");
			exit(1);
		}
	}

	if (port1 != DFLT_PORT) {
		port2 = port1 + 1;
		port3 = port1 + 2;
		port4 = port1 + 3;
	}
#ifdef WIN32
	for(unsigned i = 0; i < strlen(dir); i++)
	{
		if(dir[i] == '/')
		{
			dir[i] = '\\';
		}
	}
#endif

	logf(0, LOG_MISC, "Using %s as data directory.", dir);

	if (stat(dir, &stat_buffer) == -1) {
		if (errno == ENOENT) {
			logf(0, LOG_MISC, "Data directory %s is missing.", dir);
		} else {
			perror("stat");
		}
		exit(EXIT_FAILURE);
	}

	if (chdir(dir) < 0) {
		const char *strerror_result = strerror_r(errno, char_buffer, sizeof(char_buffer));
		logf(0, LOG_MISC, "Error changing current directory to %s: %s", dir, strerror_result);
		exit(EXIT_FAILURE);
	}

	DCVote = new CVoteData();

	if (scheck) {
		boot_zones();
		boot_world();
		log("Done.", 0, LOG_MISC);
		exit(0);
	} else {
		logf(0, LOG_MISC, "Running game on port %d, %d and %d.", port1, port2, port3);
		init_game();
	}

	delete DCVote;

	return 0;
}

