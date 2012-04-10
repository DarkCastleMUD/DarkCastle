#include <sys/stat.h>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include <utility.h>
#include <fileinfo.h>

/**********************************************************************
 *  main game loop and related stuff                                  *
 **********************************************************************/

extern char **orig_argv;
extern short code_testing_mode;
extern short code_testing_mode_mob;
extern short code_testing_mode_world;
bool verbose_mode = FALSE;
extern short bport;
extern bool allow_imp_password;
CVoteData *DCVote;

int scheck = 0;			/* for syntax checking mode */

int port, port2, port3, port4;
int DFLT_PORT = 6667, DFLT_PORT2 = 6666, DFLT_PORT3 = 4000, DFLT_PORT4 = 6669;

using namespace std;

void init_random();
void init_game(int port, int port2, int port3, int port4);
void boot_world(void);

int main(int argc, char **argv)
{
#ifndef __CYGWIN__
  // Make a copy of our executable so that in the event of a crash we have a
  // known good copy to debug with.
  stringstream backup_filename, cmd;
  backup_filename << argv[0] << "." << getpid();

  struct stat sbuf;
  // If backup file does not exist already then link to it
  if (stat(backup_filename.str().c_str(), &sbuf) == -1) {
	if (link(argv[0], backup_filename.str().c_str()) == -1) {
		perror("link");
	}
  }
#endif

  orig_argv = argv;

  char buf[512];
  int pos = 1;
  char dir[256];
  strcpy(dir, (char *)DFLT_DIR);
  
  init_random();

  port = DFLT_PORT;
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
		    "-P\t\tAllow imps to use their password\n\n",		    
		argv[0]);

	    return 0;
	    break;
    case 'w':
       code_testing_mode = 1;
       code_testing_mode_world = 1;
       log("Mud in world checking mode. TinyTinyworld being used. (WLD)", 0, LOG_MISC);
       log("Do NOT have mortals login when in world checking mode.", 0, LOG_MISC);
       break;

    case 'c':
       code_testing_mode = 1;
       log("Mud in testing mode. TinyTinyworld being used. (OBJ)", 0, LOG_MISC);
       break;
                                                
    case 'm':
       code_testing_mode = 1;
       code_testing_mode_mob = 1;
       log("Mud in testing mode. TinyTinyworld being used. (MOB,OBJ)", 0, LOG_MISC);
       break;
    case 'b': // Buildin' port.
	bport = 1;
	port = 7000;
	port2 = 7001;
	port3 = 7002;
	port4 = 7003;
	break;                                                             
    case 'd':
       if(argv[pos][2] != '\0')
           strcpy(dir, &argv[pos][2]);
	   else if(++pos < argc)
           strcpy(dir, &argv[pos][0]);
	   else {
           fprintf(stderr, "Directory arg expected after -d.\n\r");
           exit(1);
       }
       break;
    case 'p':
       port = 1500;
       port2 = 1501;
       port3 = 1502;
       port4 = 1503;
       break;
    case 'P':
      allow_imp_password = true;
      break;
    default:
      sprintf(buf, "SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
      log(buf, 0, LOG_MISC);
      break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit(*argv[pos])) {
      fprintf(stderr, "Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      fprintf(stderr, "Illegal port number.\n");
      exit(1);
    }
  }

   if (port != DFLT_PORT) { 
     port2 = port + 1; 
     port3 = port + 2;
     port4 = port + 3;
     }
#ifndef WIN32
  if (chdir(dir) < 0) {
    perror("Fatal error changing to data directory");
    exit(1);
  }
#else
  for(unsigned i = 0; i < strlen(dir); i++)
  {
	  if(dir[i] == '/')
	  {
		  dir[i] = '\\';
	  }
  }
  if(_chdir(dir) < 0) {
	  perror("Fatal error changing to data directory");
	  exit(1);
  }
#endif
  sprintf(buf, "Using %s as data directory.", dir);
  log(buf, 0, LOG_MISC);

  DCVote = new CVoteData();

  if (scheck) {
    boot_world();
    log("Done.", 0, LOG_MISC);
    exit(0);
  } else {
    sprintf(buf, "Running game on port %d, %d and %d.", port, port2, port3);
    log(buf, 0, LOG_MISC);
    init_game(port, port2, port3, port4);
  }

  delete DCVote;

  return 0;
}

