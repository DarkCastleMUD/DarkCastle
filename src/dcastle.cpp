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

extern char* const* orig_argv;
uint16_t DFLT_PORT = 6667, DFLT_PORT2 = 6666, DFLT_PORT3 = 4000, DFLT_PORT4 = 6669;
CVoteData *DCVote;

void init_random();
void init_game(void);
void boot_zones(void);
void boot_world(void);
void backup_executable(char * const argv[]);
DC::config parse_arguments(int argc, char * const argv[]);

/**********************************************************************
 *  main game loop and related stuff                                  *
 **********************************************************************/

int main(int argc, char *const argv[])
{
  struct stat stat_buffer;
  char char_buffer[512] = { '\0' };

  DC &dc = DC::instance();
  dc.cf = parse_arguments(argc, argv);

  logf(0, LOG_MISC, "Executable: %s Version: %s Build date: %s\n", argv[0], DC::getVersion().c_str(), DC::getBuildTime().c_str());
  backup_executable(argv);

  // If no ports specified then set default ports
  if (dc.cf.ports.size() == 0)
  {
    dc.cf.ports.push_back(DFLT_PORT);
    dc.cf.ports.push_back(DFLT_PORT2);
    dc.cf.ports.push_back(DFLT_PORT3);
    dc.cf.ports.push_back(DFLT_PORT4);
  }

  orig_argv = argv;

  init_random();

  logf(0, LOG_MISC, "Using %s as data directory.", dc.cf.dir.c_str());

  if (stat(dc.cf.dir.c_str(), &stat_buffer) == -1)
  {
    if (errno == ENOENT)
    {
      logf(0, LOG_MISC, "Data directory %s is missing.", dc.cf.dir.c_str());
    } else
    {
      perror("stat");
    }
    exit(EXIT_FAILURE);
  }

  if (chdir(dc.cf.dir.c_str()) < 0)
  {
    const char *strerror_result = strerror_r(errno, char_buffer, sizeof(char_buffer));
    logf(0, LOG_MISC, "Error changing current directory to %s: %s", dc.cf.dir, strerror_result);
    exit(EXIT_FAILURE);
  }

  DCVote = new CVoteData();

  if (dc.cf.check_syntax)
  {
    boot_zones();
    boot_world();
    log("Done.", 0, LOG_MISC);
    exit(EXIT_SUCCESS);
  } else
  {
    dc.init_game();
  }

  delete DCVote;

  return 0;
}

void backup_executable(char * const argv[])
{
#ifndef __CYGWIN__
	struct stat stat_buffer;
	char char_buffer[512] = { '\0' };
	
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
	return;
}

DC::config parse_arguments(int argc, char *const argv[])
{
  char opt;
  uint32_t port;
  DC::config cf;

  while ((opt = getopt(argc, argv, "vp:Pmbd:shwcn?")) != -1)
  {
    switch (opt) {
    case 'v':
      cf.verbose_mode = true;
      break;
    case 'p':
      port = atoi(optarg);
      cf.ports.push_back(port);
      break;
    case 'P':
      cf.allow_imp_password = true;
      break;
    case 'm':
      cf.test_mobs = 1;
      cf.test_objs = 1;
      log("Mud in testing mode. TinyTinyworld being used. (MOB,OBJ)", 0, LOG_MISC);
      break;
    case 'n': //inhibits printing timeout on LOG_MISC messages normally sent to STDERR
      cf.stderr_timestamp = false;
      break;
    case 'b': // Buildin' port.
      cf.bport = 1;
      cf.ports.push_back(7000);
      cf.ports.push_back(7001);
      cf.ports.push_back(7002);
      cf.ports.push_back(7003);
      break;
    case 'd':
      cf.dir = optarg;
      break;
    case 's':
      cf.check_syntax = true;
      break;
    case 'w':
      cf.test_world = 1;
      log("Mud in world checking mode. TinyTinyworld being used. (WLD)", 0, LOG_MISC);
      log("Do NOT have mortals login when in world checking mode.", 0,
      LOG_MISC);
      break;
    case 'c':
      cf.test_objs = 1;
      log("Mud in testing mode. TinyTinyworld being used. (OBJ)", 0, LOG_MISC);
      break;
    default:
    case 'h':
    case '?':
      fprintf(stderr, "Usage: %s [-v] [-h] [-w] [-c] [-m] [-d directory] [-p port#] [-P]\n"
          "-v\t\tVerbose mode\n"
          "-h\t\tUsage information\n"
          "-w\t\tWorld testing mode\n"
          "-c\t\tObj testing mode\n"
          "-m\t\tObj & Mob testing mode\n"
          "-n\t\tinhibits printing timestamps on STDOUT/STDERR\n"
          "-b\t\tBuilders' port (7000-7003)\n"
          "-d directory\tData directory\n"
          "-p [port#]\tCan be repeated to listen on multiple ports\n"
          "-P\t\tAllow imps to use their password\n\n"
          "-s\t\tSyntax checking mode\n", argv[0]);

      exit(EXIT_FAILURE);
      break;
    }
  }

  return cf;
}
