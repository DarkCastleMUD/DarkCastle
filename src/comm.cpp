/* 
************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif


#include <errno.h>
#include <terminal.h>
#include <string.h>

#ifndef WIN32
	#include <unistd.h>
	#include <sys/wait.h>
	#include <sys/socket.h>
	#include <sys/resource.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/telnet.h>
	#include <arpa/inet.h>
#else
	#include <direct.h>
	#include <winsock2.h>
	#include <process.h>
	#include <mmsystem.h>

	// swipe some defined out of arpa telnet
	#define	IAC				255		/* interpret as command: */
	#define	WONT			252		/* I won't use option */
	#define	WILL			251		/* I will use option */
	#define TELOPT_NAOCRD	10	/* negotiate about CR disposition */
	#define TELOPT_ECHO		1	/* echo */
	#define TELOPT_NAOFFD	13	/* negotiate about formfeed disposition */

#endif
#include <fcntl.h>

#include <sys/types.h>



#include <signal.h>
#include <ctype.h>

#include <fileinfo.h>
#include <act.h>
#include <player.h>
#include <levels.h>
#include <room.h>
#include <structs.h>
#include <utility.h>
#include <connect.h>
#include <interp.h>
#include <handler.h>
#include <db.h>
#include <comm.h>
#include <returnvals.h>
#include <quest.h>


#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

extern bool MOBtrigger;
short code_testing_mode = 0;
short code_testing_mode_mob = 0;
short code_testing_mode_world = 0;
short bport = 0;
unsigned mother_desc, other_desc, third_desc, fourth_desc;

// This is turned on right before we call game_loop
int do_not_save_corpses = 1;
int try_to_hotboot_on_crash = 0;
int was_hotboot = 0;
int died_from_sigsegv = 0;

//char ** ext_argv = 0;

/* these are here for the eventual addition of ban */
int num_invalid = 0;
int restrict = 0;


/* externs */
extern int restrict;
//extern int mini_mud;
//extern int no_rent_check;
extern FILE *player_fl;
int DFLT_PORT = 6667, DFLT_PORT2 = 6666, DFLT_PORT3 =4000;
int DFLT_PORT4 =6669;

extern CWorld world;	/* In db.c */
extern int top_of_world;	/* In db.c */
extern struct time_info_data time_info;		/* In db.c */
extern char help[];
extern char * sector_types[];
extern struct room_data ** world_array;
extern struct char_data *character_list;
void check_leaderboard(void);
void check_champion(void);
void save_slot_machines(void);

/* local globals */
struct descriptor_data *descriptor_list = NULL;		/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int _shutdown = 0;		/* clean shutdown */
int tics = 0;			/* for extern checkpointing */
int scheck = 0;			/* for syntax checking mode */
//int nameserver_is_slow = 0;	/* see config.c */
//extern int auto_save;		/* see config.c */
//extern int autosave_time;	/* see config.c */
struct timeval null_time;	/* zero-valued time structure */
time_t start_time;
int port, port2, port3, port4;

// heartbeat globals
int pulse_timer;
int pulse_mobile;
int pulse_bard;
int pulse_violence;
int pulse_weather;
int pulse_regen;
int pulse_time;
int pulse_short; // short timer, for archery

/* functions in this file */
void update_mprog_throws(void);
void update_bard_singing(void);
void update_command_lag_and_poison(void);
void short_activity();
void skip_spaces(char **string);
char *any_one_arg(char *argument, char *first_arg);
char * calc_color(int hit, int max_hit);
char * calc_condition(CHAR_DATA , bool );
void generate_prompt(CHAR_DATA *ch, char *prompt);
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(int port, int port2, int port3, int port4);
void signal_setup(void);
void game_loop(unsigned mother_desc, unsigned other_desc, unsigned third_desc, unsigned fourth_desc);
int init_socket(int port);
int new_descriptor(int s);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void flush_queues(struct descriptor_data *d);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void make_prompt(struct descriptor_data *point, char *prompt);
void check_idle_passwords(void);
void init_heartbeat();
void heartbeat();
void report_debug_logging();

/* extern fcnts */
void pulse_takeover(void);
void boot_db(void);
void boot_world(void);
void zone_update(void);
void affect_update(int32 duration_type);	/* In spells.c */
void point_update(void);	/* In limits.c */
void food_update(void);		/* In limits.c */
void mobile_activity(void);
void object_activity(void);
void update_corpses_and_portals(void);
void string_add(struct descriptor_data *d, char *str);
void new_string_add(struct descriptor_data *d, char *str);
void string_hash_add(struct descriptor_data *d, char *str);
void perform_violence(void);
void show_string(struct descriptor_data *d, char *input);
int isbanned(char *hostname);
void time_update();
void weather_update();
void send_hint();

//extern char greetings1[MAX_STRING_LENGTH];
//extern char greetings2[MAX_STRING_LENGTH];
//extern char greetings3[MAX_STRING_LENGTH];
//extern char greetings4[MAX_STRING_LENGTH];

#ifdef WIN32
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
  DWORD millisec = GetTickCount();
  t->tv_sec = (int) (millisec / 1000);
  t->tv_usec = (millisec % 1000) * 1000;
}
#endif
/* *********************************************************************
*  main game loop and related stuff                                    *
********************************************************************* */

int main(int argc, char **argv)
{
  char buf[512];
  int pos = 1;
  char dir[256];
  strcpy(dir, (char *)DFLT_DIR);
  
  extern void init_random();
  init_random();

  port = DFLT_PORT;
  port2 = DFLT_PORT2;
  port3 = DFLT_PORT3;
  port4 = DFLT_PORT4;
  //  ext_argv = argv;

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {

    case 'w':
       code_testing_mode = 1;
       code_testing_mode_world = 1;
       log("Mud in world checking mode.  TinyTinyworld being used. (WLD)"
           "\r\nDo NOT have mortals login when in this mode.", 0, LOG_MISC);
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

  if (scheck) {
    boot_world();
    log("Done.", 0, LOG_MISC);
    exit(0);
  } else {
    sprintf(buf, "Running game on port %d, %d and %d.", port, port2, port3);
    log(buf, 0, LOG_MISC);
    init_game(port, port2, port3, port4);
  }
  return 0;
}

// writes all the descriptors to file so we can open them back up after
// a reboot
int write_hotboot_file() 
{
  FILE *fp;
  struct descriptor_data *d;
  struct descriptor_data *sd;
  /* Azrack -- do these need to be here?
  extern int mother_desc;
  extern int other_desc;
  extern int third_desc;
  */
  //  extern char ** ext_argv;

  if ((fp=fopen("hotboot","w"))==NULL) {   
    log("Hotboot failed, unable to open hotboot file.", 0, LOG_MISC);
    return 0; 
  }
  fprintf(fp, "%d\n%d\n%d\n%d\n", mother_desc, other_desc, third_desc, fourth_desc);
  for (d=descriptor_list;d;d=sd)
  {
    sd = d->next;  
    if (STATE(d) != CON_PLAYING || !d->character || GET_LEVEL(d->character) < 2) {
      // Kick out anyone not currently playing in the game.
      write_to_descriptor(d->descriptor,"We are rebooting, come back in a minute.");
      close_socket(d);
    } else {
      STATE(d) = CON_PLAYING; // if editors.
      if(d->original) {
        fprintf(fp,"%d\n%s\n%s\n",d->descriptor,GET_NAME(d->original),d->host);
	if (d->original->pcdata){
   	 if(d->original->pcdata->last_site)
     	 dc_free(d->original->pcdata->last_site);
#ifdef LEAK_CHECK
	    d->original->pcdata->last_site = (char *)calloc(strlen(d->host) + 1, sizeof(char));
#else
	    d->original->pcdata->last_site = (char *)dc_alloc(strlen(d->host) + 1, sizeof(char));
#endif
	    strcpy (d->original->pcdata->last_site, d->original->desc->host);
	    d->original->pcdata->time.logon = time(0);
	}
        save_char_obj(d->original);

      }
      else {
        fprintf(fp,"%d\n%s\n%s\n",d->descriptor,GET_NAME(d->character),d->host);
	if (d->character->pcdata){
   	 if(d->character->pcdata->last_site)
     	 dc_free(d->character->pcdata->last_site);
#ifdef LEAK_CHECK
	    d->character->pcdata->last_site = (char *)calloc(strlen(d->host) + 1, sizeof(char));
#else
	    d->character->pcdata->last_site = (char *)dc_alloc(strlen(d->host) + 1, sizeof(char));
#endif
	    strcpy (d->character->pcdata->last_site, d->character->desc->host);
	    d->character->pcdata->time.logon = time(0);
	}
        save_char_obj(d->character);
      }
      write_to_descriptor(d->descriptor,"Attempting to maintain your link during reboot.\r\nPlease wait..");
    }
  }
  fclose(fp);
  log("Hotboot descriptor file successfully written.", 0, LOG_MISC);
  
  // note, for debug mode, you have to put the "-c", "6969", in there
  if (!bport) {
    if(-1 == execl("../src/research1", "research1",(char*)NULL)) {
    log("Hotboot execv call failed.", 0, LOG_MISC);
    perror("../src/research1");
    unlink("hotboot"); // wipe the file since we can't use it anyway
    return 0;
  } } 
  else {
    if(-1 == execl("../src/research1.b", "research1.b","-b", (char *)NULL)) {
    log("Hotboot execv call failed.", 0, LOG_MISC);
    perror("../src/research1.b");
    unlink("hotboot"); // wipe the file since we can't use it anyway
    return 0;
  } } 

  return 1;
}

// attempts to read in the descs written to file, and reconnect their
// links to the mud.
int load_hotboot_descs() 
{
  FILE *fp;
  char chr[MAX_INPUT_LENGTH], host[MAX_INPUT_LENGTH] ,buf[MAX_STRING_LENGTH];
  int desc;
  struct descriptor_data *d;
  /* Azrack - do these need to be here
  extern int mother_desc;
  extern int other_desc;
  extern int third_desc;
*/
  if ((fp=fopen("hotboot","r"))==NULL) { // Checks if it actually *is* a hotboot
    log("Hotboot file missing/unopenable.", 0, LOG_MISC);
    return 0;
  }
  log("Hotboot, reloading characters.", 0, LOG_MISC);
  unlink("hotboot"); // remove the file, it's in memory for reading anyways

  fscanf(fp, "%d\n%d\n%d\n%d\n", &mother_desc, &other_desc, &third_desc, &fourth_desc);

  while(!feof(fp)) {
    desc =0;
    *chr = '\0';
    *host = '\0';
    fscanf(fp, "%d\n%s\n%s\n", &desc, chr, host);
    d = (struct descriptor_data *)dc_alloc(1, sizeof(struct descriptor_data));
    memset((char *) d, 0, sizeof(struct descriptor_data));
    d->idle_time = 0;
    d->idle_tics               = 0;
    d->wait                    = 1;
    d->bufptr                  = 0;
    d->prompt_mode             = 1;
    d->output                  = d->small_outbuf;
//    *d->output                 = '\0';
    d->input.head             = 0;
    strcpy(d->output, chr); // store it for later
    d->bufspace                = SMALL_BUFSIZE - 1;
    d->login_time              = time(0);

    if ( write_to_descriptor( desc, "Recovering...\r\n" ) == -1) {
      sprintf(buf,"Host %s Char %s Desc %d FAILED to recover from hotboot.",host,chr,desc);
      log(buf, 0, LOG_MISC);
      CLOSE_SOCKET(desc);
      dc_free(d);
      d = NULL;
      continue;
    }

    strcpy(d->host, host);
    d->descriptor              = desc;

    // we need a second to be sure
    if (-1 == write_to_descriptor(d->descriptor,"Link recovery successful.\n\rPlease wait while mud finishes rebooting...\n\r")) {
      sprintf(buf,"Host %s Char %s Desc %d failed to recover from hotboot.",host,chr,desc);
      log(buf, 0, LOG_MISC);
      CLOSE_SOCKET(desc);
      dc_free(d);
      d = NULL;
      continue; 
    }

    d->next                    = descriptor_list; 
    descriptor_list            = d;

  }
  fclose(fp);

  unlink("hotboot"); // if the above unlink failed somehow(?), 
                     // remove the hotboot file so that it dosen't think 
                     // next reboot is another hotboot
  log("Successful hotboot file read.", 0, LOG_MISC);
  return 1;
}

void finish_hotboot() 
{
  struct descriptor_data *d;
  char buf[MAX_STRING_LENGTH];

  void do_on_login_stuff(char_data * ch);

  for (d=descriptor_list;d;d=d->next)
  {
    write_to_descriptor( d->descriptor, "Reconnecting your link to your character...\r\n" );

    if (!load_char_obj(d, d->output)) {
      sprintf(buf, "Could not load char '%s' in hotboot.",  d->output);
      log(buf, 0, LOG_MISC);
      write_to_descriptor( d->descriptor, "Link Failed!  Tell an Immortal when you can.\n\r" );
      close_socket(d);
      continue;
    }

    write_to_descriptor( d->descriptor, "Success...May your visit continue to suck...\n\r" );

    *d->output = '\0';
    d->character->next = character_list;
    character_list = d->character;

    do_on_login_stuff(d->character);

    STATE(d) = CON_PLAYING;
  }

  for (d=descriptor_list;d;d=d->next)
 {   do_look(d->character, "", 8);
    do_save(d->character,"",666);
  }
}

/* Init sockets, run game, and cleanup sockets */
void init_game(int port, int port2, int port3, int port4)
{
	/* Azrack -- do these need to be here?
  extern int mother_desc;
  extern int other_desc;
  extern int third_desc;
  extern int was_hotboot;
  extern int try_to_hotboot_on_crash;
  */

#ifdef LEAK_CHECK
  void remove_all_mobs_from_world();
  void remove_all_objs_from_world();
  void clean_socials_from_memory();
  void free_clans_from_memory();
  void free_world_from_memory();
  void free_mobs_from_memory();
  void free_objs_from_memory();
  void free_messages_from_memory();
  void free_hsh_tree_from_memory();
  void free_wizlist_from_memory();
  void free_game_portals_from_memory();
  void free_help_from_memory();
  void free_zones_from_memory();
  void free_shops_from_memory();
  void free_emoting_objects_from_memory();
  void free_boards_from_memory();
  void free_command_radix_nodes(struct cmd_hash_info * curr);
  void free_ban_list_from_memory();
  void free_buff_pool_from_memory();
  extern struct cmd_hash_info * cmd_radix;
#endif

#ifndef WIN32
  srandom(time(0));
#else
  srand(time(0));
#endif

  FILE * fp;
  // create boot'ing lockfile
  if((fp = fopen("died_in_bootup","w")))
    fclose(fp);

  log("Attempting to load hotboot file.", 0, LOG_MISC);
  if(load_hotboot_descs()) {
    log("Hotboot Loading complete.", 0, LOG_MISC);
    was_hotboot = 1;
  }
  else {
    log("Hotboot failed.  Starting regular sockets.", 0, LOG_MISC);
    log("Opening mother connections.", 0, LOG_MISC);
    mother_desc = init_socket(port);
    // no need for the other ports rinetd handles it now. Scratch that.
       other_desc = init_socket(port2);
       third_desc = init_socket(port3);
       fourth_desc = init_socket(port4);
  }

  start_time = time(0);
  boot_db();

  if(was_hotboot) {
    log("Connecting hotboot characters to their descriptiors", 0, LOG_MISC);
    finish_hotboot();
  }

  log("Signal trapping.", 0, LOG_MISC);
  signal_setup();

  // we got all the way through, let's turn auto-hotboot back on
  try_to_hotboot_on_crash = 1;

  log("Entering game loop.", 0, LOG_MISC);

  unlink("died_in_bootup");

  do_not_save_corpses = 0;
  game_loop(mother_desc, other_desc, third_desc, fourth_desc);
  do_not_save_corpses = 1;

  log("Closing all sockets.", 0, LOG_MISC);
  while (descriptor_list)
    close_socket(descriptor_list);
  CLOSE_SOCKET(mother_desc);
  CLOSE_SOCKET(other_desc);
  CLOSE_SOCKET(third_desc);
  CLOSE_SOCKET(fourth_desc);

#ifdef LEAK_CHECK
  log("Freeing all mobs in world.", 0, LOG_MISC);
  remove_all_mobs_from_world();
  log("Freeing all objs in world.", 0, LOG_MISC);
  remove_all_objs_from_world();
  log("Freeing socials from memory.", 0, LOG_MISC);
  clean_socials_from_memory();
  log("Freeing zones data.", 0, LOG_MISC);
  free_zones_from_memory();
  log("Freeing clan data.", 0, LOG_MISC);
  free_clans_from_memory();
  log("Freeing the world.", 0, LOG_MISC);
  free_world_from_memory();
  log("Freeing mobs from memory.", 0, LOG_MISC);
  free_mobs_from_memory();
  log("Freeing objs from memory.", 0, LOG_MISC);
  free_objs_from_memory();
  log("Freeing messages from memory.", 0, LOG_MISC);
  free_messages_from_memory();
  log("Freeing hash tree from memory.", 0, LOG_MISC);
  free_hsh_tree_from_memory();
  log("Freeing wizlist from memory.", 0, LOG_MISC);
  free_wizlist_from_memory();
  log("Freeing help index.", 0, LOG_MISC);
  free_help_from_memory();
  log("Freeing shops from memory.", 0, LOG_MISC);
  free_shops_from_memory();
  log("Freeing emoting objects from memory.", 0, LOG_MISC);
  free_emoting_objects_from_memory();
  log("Freeing game portals from memory.", 0, LOG_MISC);
  free_game_portals_from_memory();
  log("Freeing boards from memory.", 0, LOG_MISC);
  free_boards_from_memory();
  log("Freeing command radix from memory.", 0, LOG_MISC);
  free_command_radix_nodes(cmd_radix);
  log("Freeing ban list from memory.", 0, LOG_MISC);
  free_ban_list_from_memory();
  log("Freeing the bufpool.", 0, LOG_MISC);
  free_buff_pool_from_memory();
#endif

  log("Goodbye.", 0, LOG_MISC);

#ifdef LEAK_CHECK
  dmalloc_shutdown();
#endif

  log("Normal termination of game.", 0, LOG_MISC);
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(int port)
{
  int s, opt;
  struct sockaddr_in sa;

#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);

    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
      perror("SYSERR: WinSock not available!");
      exit(1);
    }

   if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      exit(1);
   }
#else

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error creating socket");
    exit(1);
  }
#endif
  opt = LARGE_BUFSIZE + GARBAGE_SPACE;
  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt SNDBUF");
    exit(1);
  }

  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt REUSEADDR");
    exit(1);
  }
  struct linger ld;
  ld.l_onoff = 0;
  ld.l_linger = 0;
  if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0) {
     perror("setsockopt LINGER");
     exit(1);
  }

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("bind");
    CLOSE_SOCKET(s);
    exit(1);
  }
#ifndef WIN32
  if(fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
    perror("init_socket : fcntl : nonblock");
    exit(1);
  }
#else
  	unsigned long int nb = 1;
	if(ioctlsocket(s, FIONBIO, &nb) < 0)
	{
		perror("init_socket : ioctl : nonblock");
		exit(1);
	}
#endif
  if(listen(s, 5) < 0) {
    perror("init_socket : listen");
    exit(1);
  }
  return s;
}


// This is set as a global...it is the increment variable for the descriptor list
// used in game_loop.  It has to be global so that "close_socket" can increment
// it if we are closing the socket that is next to be processed.
struct descriptor_data * next_d;

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" function
 */
void game_loop(unsigned mother_desc, unsigned other_desc, unsigned third_desc, unsigned fourth_desc)
{
 
  fd_set input_set, output_set, exc_set, null_set;
  struct timeval last_time, delay_time, now_time;
  long secDelta, usecDelta;

//struct timeval debugtimer1, debugtimer2;

  // comm must be much longer than MAX_INPUT_LENGTH since we allow aliases in-game
  // otherwise an alias'd command could easily overrun the buffer
  char comm[MAX_STRING_LENGTH];
  char buf[128];
  struct descriptor_data *d;
  unsigned maxdesc;
  int aliased;

  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  FD_ZERO(&null_set);
  init_heartbeat();

  gettimeofday(&last_time, NULL);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!_shutdown) {

//logf(110, LOG_BUG, "Startloop");
//gettimeofday(&debugtimer1, NULL);
	extern bool selfpurge;
	selfpurge = FALSE;
    // Set up the input, output, and exception sets for select().
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);
    FD_SET(other_desc, &input_set);
    FD_SET(third_desc, &input_set);
    FD_SET(fourth_desc, &input_set);

    maxdesc = ((mother_desc > other_desc) ? mother_desc : other_desc);
    maxdesc = ((maxdesc > third_desc) ? maxdesc : third_desc);
    maxdesc = ((maxdesc > fourth_desc) ? maxdesc : fourth_desc);
    //maxdesc = mother_desc;   
 
    for (d = descriptor_list; d; d = d->next) {
      if(d->descriptor > maxdesc) 
          maxdesc = d->descriptor;
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    // poll (without blocking) for new input, output, and exceptions
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("game_loop : select : poll");
      return;
    }

    // If new connection waiting, accept it
    if (FD_ISSET(mother_desc, &input_set))
      new_descriptor(mother_desc);
    if (FD_ISSET(other_desc, &input_set))
      new_descriptor(other_desc);
    if (FD_ISSET(third_desc, &input_set))
      new_descriptor(third_desc);
    if (FD_ISSET(fourth_desc, &input_set))
      new_descriptor(fourth_desc);

    // close the weird descriptors in the exception set 
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) {
	FD_CLR(d->descriptor, &input_set);
	FD_CLR(d->descriptor, &output_set);
	close_socket(d);
      }
    }

    /* process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &input_set))
	if (process_input(d) < 0) {
    sprintf(buf, "Connection attempt bailed from %s", d->host);
    printf(buf);
	   log(buf, OVERSEER, LOG_SOCKET);
	  close_socket(d);
	}
    }

    /* process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      d->wait = MAX(d->wait, 1);
	if (d->connected == CON_CLOSE)
	{  close_socket(d); // So they don't have to type a command.
		continue; }
	//debugpoint();
      if ((--(d->wait) <= 0) && get_from_q(&d->input, comm, &aliased)) {
	  /* reset the idle timer & pull char back from void if necessary */
	d->wait = 1;
	d->prompt_mode = 1;

	if (d->showstr_count)	/* reading something w/ pager     */
	  show_string(d, comm);
	else if (d->str)		/* writing boards, mail, etc.     */
	  string_add(d, comm);
        else if(d->hashstr)
          string_hash_add(d, comm);
        else if(d->strnew)
          new_string_add(d, comm);
	else if (d->connected != CON_PLAYING)	/* in menus, etc. */
	  nanny(d, comm);
	else {			/* else: we're playing normally */
	  if (aliased)		/* to prevent recursive aliases */
	    d->prompt_mode = 0;
	  else {
	    if (perform_alias(d, comm))
	      get_from_q(&d->input, comm, &aliased);
	  }
          // Azrack's a chode.  Don't forget to check
          // ->snooping before you check snooping->char:P
          if(*comm == '%' && d->snooping && d->snooping->character) {
             command_interpreter(d->snooping->character, comm + 1);
          } else { 
	     command_interpreter(d->character, comm);	/* send it to interpreter */
          }
	} // else if input
      } // if input
      // the below two if-statements are used to allow the mud to detect and respond
      // to web-browsers attempting to connect to the game on port 80
      // this line processes a "get" or "post" if available.  Otherwise it prints the
      // entrance screen.  If a player has already entered their name, it processes
      // that too.
      else if (d->connected == CON_DISPLAY_ENTRANCE)
        nanny(d, "");
      // this line allows the mud to skip this descriptor until next pulse
      else if (d->connected == CON_PRE_DISPLAY_ENTRANCE)
        d->connected = CON_DISPLAY_ENTRANCE;
      else d->idle_time++;
    } // for

//gettimeofday(&debugtimer2, NULL);
//logf(110, LOG_BUG, "Done socketprocessing.  Time %dsec %dusec.", 
//       (((int) debugtimer2.tv_sec) - ((int) debugtimer1.tv_sec)),
//       (((int) debugtimer2.tv_usec) - ((int) debugtimer1.tv_usec)));
//gettimeofday(&debugtimer1, NULL);

    // do what needs to be done.  violence, repoping, regen, etc.
    heartbeat();

//gettimeofday(&debugtimer2, NULL);
//logf(110, LOG_BUG, "Done heartbeat.  Time %dsec %dusec.", 
//       (((int) debugtimer2.tv_sec) - ((int) debugtimer1.tv_sec)),
//       (((int) debugtimer2.tv_usec) - ((int) debugtimer1.tv_usec)));
//gettimeofday(&debugtimer1, NULL);

    /* send queued output out to the operating system (ultimately to user) */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if ((FD_ISSET(d->descriptor, &output_set) && *(d->output)) || d->prompt_mode)
	if (process_output(d) < 0)
	  close_socket(d);
	// else
	  // d->prompt_mode = 1;
    }

//gettimeofday(&debugtimer2, NULL);
//logf(110, LOG_BUG, "Done output.  Time %dsec %dusec.", 
//       (((int) debugtimer2.tv_sec) - ((int) debugtimer1.tv_sec)),
//       (((int) debugtimer2.tv_usec) - ((int) debugtimer1.tv_usec)));
//gettimeofday(&debugtimer1, NULL);
//    debugpoint();
    // we're done with this pulse.  Now calculate the time until the next pulse and sleep until then
    // we want to pulse PASSES_PER_SEC times a second (duh).  This is currently 4.

    gettimeofday( &now_time, NULL );
    usecDelta   = ((int) last_time.tv_usec) - ((int) now_time.tv_usec);
    secDelta    = ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
//    usecDelta = ((int) now_time.tv_usec) - ((int) last_time.tv_usec);
//    secDelta = ((int) now_time.tv_sec) - ((int) last_time.tv_sec);
//logf(110, LOG_BUG, "Time since last pulse: %dsec %dusec.", 
//       (((int) now_time.tv_sec) - ((int) now_time.tv_sec)),
//       (((int) last_time.tv_usec) - ((int) last_time.tv_usec)));
//logf(110, LOG_BUG, "secD : %d  usecD: %d", secDelta, usecDelta);

    usecDelta   += (1000000 / PASSES_PER_SEC);
    while ( usecDelta < 0 )
    {
       usecDelta += 1000000;
       secDelta  -= 1;
    }
    while ( usecDelta >= 1000000 )
    {
       usecDelta -= 1000000;
       secDelta  += 1;
    }
//logf(110, LOG_BUG, "secD : %d  usecD: %d", secDelta, usecDelta);
    
    if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
    {
       delay_time.tv_usec = usecDelta;
       delay_time.tv_sec  = secDelta;
//logf(110, LOG_BUG, "Pausing for  %dsec %dusec.", secDelta, usecDelta);
#ifndef WIN32
       if ( select( 0, NULL, NULL, NULL, &delay_time ) < 0 && errno != EINTR)
       {
          perror( "game_loop: select: delay" );
          exit( 1 );
       }
#else
	   Sleep(delay_time.tv_sec * 1000 + delay_time.tv_usec / 1000);
#endif
    }
    // temp removing this since it's spamming the crap out of us
    //else logf(110, LOG_BUG, "0 delay on pulse");
    gettimeofday(&last_time, NULL);
  }
}

void init_heartbeat()
{
  pulse_mobile    = PULSE_MOBILE;
  pulse_timer	  = PULSE_TIMER;
  pulse_bard      = PULSE_BARD;   
  pulse_violence  = PULSE_VIOLENCE;
  pulse_weather   = PULSE_WEATHER;
  pulse_regen     = PULSE_REGEN;
  pulse_time      = PULSE_TIME;
  pulse_short     = PULSE_SHORT;
}

void heartbeat()
{
  if (--pulse_mobile < 1) 
  {
    pulse_mobile = PULSE_MOBILE;
    mobile_activity();
    object_activity();
  }
  if (--pulse_timer < 1)
  {
     pulse_timer = PULSE_TIMER;
     check_timer();
     affect_update(PULSE_TIMER);
  }
  if (--pulse_short < 1)
  {
     pulse_short = PULSE_SHORT;
     short_activity();
  }
  // TODO - need to eventually modify this so it works for casters too so I can delay certain
  if (--pulse_bard < 1) 
  {
    pulse_bard = PULSE_BARD;
    update_bard_singing();
    update_mprog_throws();  // convienant place to put it
  }

  if (--pulse_violence < 1) 
  {
    pulse_violence = PULSE_VIOLENCE;
    perform_violence();
    update_command_lag_and_poison();
    affect_update(PULSE_VIOLENCE);
  }

  if(--pulse_weather < 1)
  {
    pulse_weather = PULSE_WEATHER;
    weather_update();
  }

  if (--pulse_regen < 1)
  {
    // random pulse timer for regen to make tick sleeping impossible
    pulse_regen = number(PULSE_REGEN-8*PASSES_PER_SEC, PULSE_REGEN+5*PASSES_PER_SEC);
    point_update();
    pulse_takeover();
    affect_update(PULSE_REGEN);
    if(!number(0,2)) send_hint();
  }

  if(--pulse_time < 1) {
    pulse_time = PULSE_TIME;
    zone_update();
    time_update();
    food_update();
    affect_update(PULSE_TIME);
    update_corpses_and_portals();
    check_idle_passwords();
    check_leaderboard(); //good place to put this
    quest_update();
    check_champion();
    save_slot_machines();
  }
}


/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) TELOPT_NAOFFD,
    (char) TELOPT_NAOCRD,
    (char) 0,
  };

  SEND_TO_Q(on_string, d);
}

int do_prompt(CHAR_DATA *ch, char *arg, int cmd) 
{
  while(*arg == ' ')
    arg++;

  if(IS_MOB(ch)) {
    send_to_char("You're a mob!  You can't set your prompt.\r\n", ch);
    return eFAILURE;
  }

  if(!*arg) {
    send_to_char("Set your prompt to what? Try 'help prompt'.\n\r", ch);
    if(GET_PROMPT(ch)) {
       send_to_char("Current prompt:  ", ch);
       send_to_char(GET_PROMPT(ch), ch);
       send_to_char("\n\r", ch);
    }
    return eSUCCESS;
  }

  // we only have 80 chars of storage in the pfile!
  if(strlen(arg) > 78) {
    send_to_char("Prompts have a maximum of 78 characters.\n\r", ch);
    return eFAILURE;
  }

  if(GET_PROMPT(ch))
    dc_free(GET_PROMPT(ch));
  GET_PROMPT(ch) = str_dup(arg);
  send_to_char("Ok.\n\r", ch);
  return eSUCCESS;
}

char * calc_color_align(int align)
{
  if(align <= -351)
    return BOLD RED;
  if (align <= -300) 
     return BOLD YELLOW;
  if (align <= 299)
     return BOLD GREY;
  if (align <= 349)
     return BOLD YELLOW;
  return BOLD GREEN;
}

char * calc_color(int hit, int max_hit)
{
/* damn whiney players
  int percentage = hit * 100 / max_hit;

  if(percentage >= 100)
    return BOLD GREEN;
  else if(percentage >= 90)
    return GREEN;
  else if(percentage >= 75)
    return BOLD YELLOW;
  else if(percentage >= 50)
    return YELLOW;
  else if(percentage >= 30)
    return RED;
  else if(percentage >= 15)
    return BOLD RED;
  else return BOLD GREY;
*/

  if(hit <= (max_hit / 3))
    return BOLD RED;

  if(hit <= (max_hit / 3) * 2)
    return BOLD YELLOW; 

  return GREEN;

}

char * cond_txtz[] = {
  "excellent condition",
  "a few scratches",
  "slightly hurt",
  "fairly fucked up",
  "bleeding freely",
  "covered in blood",
  "near death",
  "dead as a doornail"
};

char * cond_txtc[] = {
  BOLD GREEN  "excellent condition"  NTEXT,
  GREEN "a few scratches" NTEXT,
  BOLD YELLOW "slightly hurt" NTEXT,
  YELLOW "fairly fucked up" NTEXT,
  RED "bleeding freely" NTEXT,
  BOLD RED "covered in blood" NTEXT,
  BOLD GREY "near death" NTEXT,
  "dead as a doornail"
};


char * calc_condition(CHAR_DATA *ch, bool colour = FALSE)
{
  int percent;
  char *cond_txt[8];// = cond_txtz;

 // if (colour)
 //  cond_txt = cond_txtc;
 // else
 //  cond_txt = cond_txtz;
  if (colour)
   memcpy(cond_txt, cond_txtc, sizeof(cond_txtc));
  else
   memcpy(cond_txt, cond_txtz, sizeof(cond_txtz));

  if(GET_HIT(ch) == 0 || GET_MAX_HIT(ch) == 0)
    percent = 0;
  else
    percent = GET_HIT(ch) * 100 / GET_MAX_HIT(ch);

  if(percent >= 100)
      return cond_txt[0];
  else if(percent >= 90)
    return cond_txt[1];
  else if(percent >= 75)
    return cond_txt[2];
  else if(percent >= 50)
    return cond_txt[3];
  else if(percent >= 30)
    return cond_txt[4];
  else if(percent >= 15)
    return cond_txt[5];
  else if(percent >= 0) 
    return cond_txt[6];
  else
    return cond_txt[7];
}


void make_prompt(struct descriptor_data *d, char *prompt)
{
     char buf[MAX_STRING_LENGTH];
     if(!d->character) {
         return;
     }
     if (d->showstr_count) {
     sprintf(buf,
             "\r\n[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d %d) ]",
              d->showstr_page, d->showstr_count);
     strcat(prompt, buf);
     } else if(d->strnew) {
         strcat(prompt, "*] ");
     } else if(d->str || d->hashstr) {
         strcat(prompt, "] ");
     } else if(STATE(d) != CON_PLAYING) {
         return;
     } else if(IS_MOB(d->character)) {
        generate_prompt(d->character, prompt);
     } else if(GET_LEVEL(d->character) < IMMORTAL) {
         if (!IS_SET(GET_TOGGLES(d->character), PLR_COMPACT))
            strcat(prompt, "\n\r");
         if(!GET_PROMPT(d->character))
            strcat(prompt, "type 'help prompt'> ");
         else
             generate_prompt(d->character, prompt);
     } else {
        if (!IS_SET(GET_TOGGLES(d->character), PLR_COMPACT))
            strcat(prompt, "\n\r");

        struct room_data *rm = &world[d->character->in_room];
        sprintf(buf,
                IS_SET(GET_TOGGLES(d->character), PLR_ANSI) ?
                "Z:"RED"%d "NTEXT"R:"GREEN"%d "NTEXT"I:"YELLOW"%ld"NTEXT"> " :
                "Z:%d R:%d I:%ld> ",
                rm->zone, rm->number, d->character->pcdata->wizinvis);
        strcat(prompt, buf);
    }
}

void generate_prompt(CHAR_DATA *ch, char *prompt)
{
  char gprompt[MAX_STRING_LENGTH];
  char *source;
  char *pro;
  pro = gprompt;
  char * mobprompt = "HP: %i/%H %f >";

  if(IS_NPC(ch))
    source = mobprompt;
  else source = GET_PROMPT(ch);

  for(; *source != '\0';) {
     if(*source != '%') {
       *pro = *source;
       ++pro; ++source;
       *pro = '\0';
       continue;
     }
     ++source;
     if(*source == '\0') {
       strcpy(prompt, "There is a fucked up code in your prompt> ");
       return;
     }

     switch(*source) {
       default:
         strcat(prompt, "There is a fucked up code in your prompt> ");
         return;
       case 'p':
         sprintf(pro, "%d", GET_PLATINUM(ch));
         break;
       case 'g':
         sprintf(pro, "%d", GET_GOLD(ch));
         break;
       case 'G':
         sprintf(pro, "%d", (int32) (GET_GOLD(ch)/20000));
         break;
       case 'h':
         sprintf(pro, "%d", GET_HIT(ch));
         break;
       case 'H':
         sprintf(pro, "%d", GET_MAX_HIT(ch));
         break;
       case 'm':
         sprintf(pro, "%d", GET_MANA(ch));
         break;
       case 'M':
         sprintf(pro, "%d", GET_MAX_MANA(ch));
         break;
       case 'v':
         sprintf(pro, "%d", GET_MOVE(ch));
         break;
       case 'V':
         sprintf(pro, "%d", GET_MAX_MOVE(ch));
         break;
       case 'k':
         sprintf(pro, "%d", GET_KI(ch));
         break;
       case 'K':
         sprintf(pro, "%d", GET_MAX_KI(ch));
         break;
       case 'l':
         sprintf(pro, "%s%d%s", calc_color(GET_KI(ch), GET_MAX_KI(ch)),
                 GET_KI(ch), NTEXT);
         break;
       case 'i':
         sprintf(pro, "%s%d%s", calc_color(GET_HIT(ch), GET_MAX_HIT(ch)),
                 GET_HIT(ch), NTEXT);
         break;
       case 'n':
         sprintf(pro, "%s%d%s", calc_color(GET_MANA(ch), GET_MAX_MANA(ch)),
                 GET_MANA(ch), NTEXT);
         break;
       case 'w':
         sprintf(pro, "%s%d%s", calc_color(GET_MOVE(ch), GET_MAX_MOVE(ch)),
                 GET_MOVE(ch), NTEXT);
         break;
       case 'I':
         sprintf(pro, "%d", (int) (100*((float)GET_HIT(ch)/(float)GET_MAX_HIT(ch))));
         break;
       case 'N':
         sprintf(pro, "%d", (int) (100*((float)GET_MANA(ch)/(float)GET_MAX_MANA(ch))));
         break;
       case 'W':
         sprintf(pro, "%d", (int) (100*((float)GET_MOVE(ch)/(float)GET_MAX_MOVE(ch))));
         break;
       case 'L':
         sprintf(pro, "%d", (int) (100*((float)GET_KI(ch)/(float)GET_MAX_KI(ch))));
         break;
       case 'x':
         sprintf(pro, "%lld", GET_EXP(ch));
         break;
       case 'X':
         sprintf(pro, "%lld", (int64)(exp_table[(int)GET_LEVEL(ch) + 1] -
                                  (int64)GET_EXP(ch)));
         break;
       case '%':
         sprintf(pro, "%%");
         break;
       case 'a':
         sprintf(pro, "%hd", GET_ALIGNMENT(ch));
         break;
       case 'A':
         sprintf(pro, "%s%hd%s", calc_color_align(GET_ALIGNMENT(ch)), GET_ALIGNMENT(ch), NTEXT);
         break;
       case 'c':
         if(ch->fighting)
           sprintf(pro, "<%s>", calc_condition(ch));
/* added by pir to stop "prompt %c" crash bug */
         else sprintf(pro, " ");
         break;
	case 'C':
         if(ch->fighting)
           sprintf(pro, "<%s>", calc_condition(ch,TRUE));
/* added by pir to stop "prompt %c" crash bug */
         else sprintf(pro, " ");
         break;

       case 'f':
         if(ch->fighting)
           sprintf(pro, "(%s)", calc_condition(ch->fighting));
/* added by pir to stop "prompt %c" crash bug */
         else sprintf(pro, " ");
         break;
	case 'F':
         if(ch->fighting)
           sprintf(pro, "(%s)", calc_condition(ch->fighting,TRUE));
/* added by pir to stop "prompt %c" crash bug */
         else sprintf(pro, " ");
         break;

       case 't':
         if(ch->fighting && ch->fighting->fighting)
           sprintf(pro, "[%s]",
           calc_condition(ch->fighting->fighting));
/* added by pir to stop "prompt %c" crash bug */
         else sprintf(pro, " ");
         break;
        case 'T':
         if(ch->fighting && ch->fighting->fighting)
           sprintf(pro, "[%s]", calc_condition(ch->fighting->fighting,TRUE));
/* added by pir to stop "prompt %c" crash bug */
         else sprintf(pro, " ");
         break;

       case 's':
         if(world_array[ch->in_room])
           sprintf(pro, "%s", sector_types[world[ch->in_room].sector_type]);
         else sprintf(pro, " ");
         break;
       case '0':
         sprintf(pro, "%s", NTEXT);
         break;
       case '1':
         sprintf(pro, "%s", RED);
         break;
       case '2':
         sprintf(pro, "%s", GREEN);
         break;
       case '3':
         sprintf(pro, "%s", YELLOW);
         break;
       case '4':
         sprintf(pro, "%s", BLUE);
         break;
       case '5':
         sprintf(pro, "%s", PURPLE);
         break;
       case '6':
         sprintf(pro, "%s", CYAN);
         break;
       case '7':
         sprintf(pro, "%s", GREY);
         break;
       case '8':
         sprintf(pro, "%s", BOLD);
         break;
       case '9':
         sprintf(pro, "%s", FLASH);
         break;
       case 'r':
         sprintf(pro, "%c%c", '\n', '\r');
         break;
     }
     ++source;
     while(*pro != '\0')
       pro++;
  }
  *pro = ' ';
  *(pro+1) = '\0';
  strcat(prompt, gprompt);
}


void write_to_q(char *txt, struct txt_q *queue, int aliased)
{
  struct txt_block *new_block;

#ifdef LEAK_CHECK  
  new_block = (struct txt_block *)calloc(1, sizeof(struct txt_block));
  new_block->text = (char *)calloc(1, strlen(txt) + 1);
#else
  new_block = (struct txt_block *)dc_alloc(1, sizeof(struct txt_block));
  new_block->text = (char *)dc_alloc(1, strlen(txt) + 1);
#endif

  strcpy(new_block->text, txt);
  new_block->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    new_block->next = NULL;
    queue->head = queue->tail = new_block;
  } else {
    queue->tail->next = new_block;
    queue->tail = new_block;
    new_block->next = NULL;
  }
}



int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return 0;

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  dc_free(tmp->text);
  dc_free(tmp);
  return 1;
}



/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  int dummy;
  char buf2[8096];

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy));
  if(d->output) 
         write_to_descriptor(d->descriptor, d->output);
}

void free_buff_pool_from_memory()
{
  struct txt_block * curr = NULL;

  while(bufpool)
  {
    curr = bufpool->next;
    dc_free(bufpool->text);
    dc_free(bufpool);
    bufpool = curr;
  }
}

void scramble_text( char * txt )
{
  char * curr = txt;

  for( ; *curr; curr++)
    // only scramble letters, but not 'm' cause 'm' is used in ansi codes
    if(number(1, 5) == 5 && ((*curr >= 'a' && *curr <= 'z') || (*curr >= 'A' && *curr <= 'Z'))
       && *curr != 'm')
    {
      *curr = number(0, 1) ? (char) number('a', 'z') : (char) number('A', 'Z');
    }
}

/* Add a new string to a player's output queue */
void write_to_output(char *txt, struct descriptor_data *t)
{
  int size;
  char * temp = NULL;
  char * handle_ansi(char * s, char_data * ch);

  /* if there's no descriptor, don't worry about output */
  if (t->descriptor == 0)
    return;

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

  if (t->connected != CON_EDITING && t->connected != CON_WRITE_BOARD && t->connected != CON_EDIT_MPROG) {
    temp = handle_ansi(txt, t->character);
    txt = temp;
  }

  size = strlen(txt);

  if(t->character && IS_AFFECTED(t->character, AFF_INSANE) && t->connected == CON_PLAYING)
  {
//    temp = str_dup(txt);
//    scramble_text(temp);
//    txt = temp;

    scramble_text(txt);
  }

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size) {
    strcpy(t->output + t->bufptr, txt);
    t->bufspace -= size;
    t->bufptr += size;
    if(temp)
      dc_free(temp);
    return;
  }
  /*
   * If we're already using the large buffer, or if even the large buffer
   * is too small to handle this new text, chuck the text and switch to the
   * overflow state.
   */
  if (t->large_outbuf || ((size + strlen(t->output)) > LARGE_BUFSIZE)) {
    t->bufptr = -1;
    buf_overflows++;
    if(temp)
      dc_free(temp);
    return;
  }
  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL) {
    t->large_outbuf = bufpool;
    bufpool = bufpool->next;
  } else {			/* else create a new one */
#ifdef LEAK_CHECK
    t->large_outbuf = (struct txt_block *)calloc(1, sizeof(struct txt_block));
    t->large_outbuf->text = (char *)calloc(1, LARGE_BUFSIZE);
#else
    t->large_outbuf = (struct txt_block *)dc_alloc(1, sizeof(struct txt_block));
    t->large_outbuf->text = (char *)dc_alloc(1, LARGE_BUFSIZE);
#endif
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
  t->output = t->large_outbuf->text;	/* make big buffer primary */
  strcat(t->output, txt);	/* now add new text */

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - strlen(t->output);

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);

  if(temp)
    dc_free(temp);
}



/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


int new_descriptor(int s)
{
  socket_t desc;
#ifndef WIN32
  unsigned int i;
#else
  int i;
#endif
  static int last_desc = 0;	/* last descriptor number */
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  char buf[MAX_STRING_LENGTH];
  
/* accept the new connection */
  i = sizeof(peer);
  getsockname(s, (struct sockaddr *) &peer, &i);
  if ((desc = accept(s, (struct sockaddr *) &peer, &i)) < 0) {
    perror("accept");
    return -1;
  }

  // keep it from blocking 
#ifndef WIN32
  if(fcntl(desc, F_SETFL, O_NONBLOCK) < 0) {
    perror("init_socket : fcntl : nonblock");
    exit(1);
  }
#else
  unsigned long int nb = 1;
  if(ioctlsocket(desc, FIONBIO, &nb) < 0)
  {
	perror("init_socket : ioctl : nonblock");
	exit(1);
  }
#endif

  /* create a new descriptor */
#ifdef LEAK_CHECK
  newd = (struct descriptor_data *)calloc(1, sizeof(struct descriptor_data));
#else
  newd = (struct descriptor_data *)dc_alloc(1, sizeof(struct descriptor_data));
#endif
  memset((char *) newd, 0, sizeof(struct descriptor_data));
  strcpy(newd->host, inet_ntoa(peer.sin_addr));

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    write_to_descriptor(desc, 
              "Your site has been banned from Dark Castle. If you have any\n\r"
              "Questions, please email Apocalypse at:\n\r"
              "dc_apoc@hotmail.com\n\r");
                                                  
    CLOSE_SOCKET(desc);
    sprintf(buf, "Connection attempt denied from [%s]", newd->host);
    log(buf, OVERSEER, LOG_SOCKET);
   // dc_free(newd->host);
    dc_free(newd);
    return 0;
  }

  /* initialize descriptor data */
  newd->descriptor = desc;
  newd->idle_tics = 0;
  newd->idle_time = 0;
  newd->wait = 1;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->next = descriptor_list;
  newd->login_time = time(0);
  newd->astr = 0;
  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  /* prepend to list */
  descriptor_list = newd;

  newd->connected = CON_PRE_DISPLAY_ENTRANCE;
  return 0;
}



int process_output(struct descriptor_data *t)
{
  static char i[LARGE_BUFSIZE + GARBAGE_SPACE + MAX_STRING_LENGTH];
  static int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n");

  /* now, append the 'real' output */
  strcpy(i + 2, t->output);
  
  extern void blackjack_prompt(CHAR_DATA *ch, char *prompt, bool ascii);
  if (t->character && t->connected == CON_PLAYING)
  blackjack_prompt(t->character,i,t->character->pcdata&&!IS_SET(t->character->pcdata->toggles, PLR_ASCII));
  make_prompt(t, i);

  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
    strcat(i, "**OVERFLOW**");

  if (t->character && t->character->name && !str_cmp(t->character->name, "Apocalypse"))
    write_to_descriptor(t->descriptor, FLASH);
  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (!t->prompt_mode) {		/* && !t->connected) */
    result = write_to_descriptor(t->descriptor, i);
    t->prompt_mode = 0;
  } else {
    result = write_to_descriptor(t->descriptor, i + 2);
    t->prompt_mode = 0;
  }
  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by) {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';

  return result;
}



int write_to_descriptor(socket_t desc, char *txt)
{
  int total, bytes_written;

  total = strlen(txt);

  do {
#ifndef WIN32
    if ((bytes_written = write(desc, txt, total)) < 0) {
#else
	if ((bytes_written = send(desc, txt, total, 0)) < 0) {
#endif
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
	errno = EAGAIN;
#endif /* EWOULDBLOCK */
      if (errno == EAGAIN) {
	// log("process_output: socket write would block",
        //     0, LOG_MISC);
      }
      else {
	perror("Write to socket");
        return(-1);
      }
      return(0);
    } else {
      txt += bytes_written;
      total -= bytes_written;
    }
  } while (total > 0);

  return 0;
}


/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(struct descriptor_data *t)
{
  int buf_length, bytes_read, space_left, failed_subst, doublesign;
  char *ptr = NULL;
  char *read_point = NULL;
  char *write_point = NULL;
  char *nl_pos = NULL;
  char *tmp_ptr = NULL;
  char tmp[MAX_INPUT_LENGTH + 8];

  /* initialize doublesign */
  doublesign = 0;
  t->idle_time = 0;
  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do {
    if (space_left <= 0) {
      log("process_input: about to close connection: input overflow", ANGEL,
           LOG_SOCKET);
      return -1;
    }
#ifndef WIN32
    if ((bytes_read = read(t->descriptor, read_point, space_left)) < 0) {
#else
	if((bytes_read = recv(t->descriptor, read_point, space_left, 0)) < 0) {
		if(WSAGetLastError() == WSAEWOULDBLOCK) {
			return(0);
		}
#endif


#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
	errno = EAGAIN;
#endif /* EWOULDBLOCK */
      if (errno != EAGAIN) {
	perror("process_input: about to lose connection");
	return -1;		/* some error condition was encountered on
				 * read */
      } else
	return 0;		/* the read would have blocked: just means no
				 * data there but everything's okay */
    } else if (bytes_read == 0) {
      log("EOF on socket read (connection broken by peer)", ANGEL, LOG_SOCKET);
      return -1;
    }
    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0';	/* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr) || (t->connected != CON_WRITE_BOARD && t->connected != CON_EDITING && t->connected != CON_EDIT_MPROG && *ptr == '|'))
	nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
  } while (nl_pos == NULL);
  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

    for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b') {	// handle backspacing 
	if (write_point > tmp) {
	  if ((*(--write_point) == '$') && (write_point > tmp) &&    // if backup to $ AND room left AND
              (!t->character || GET_LEVEL(t->character) < IMMORTAL)) //    (no char OR mortal)
          {
            // need to backspace twice if it's a $ to keep morts from the $codes
            // need the write_point > tmp check to make sure we don't backspace past beginning
	    write_point--;
	    space_left += 2;
	  } else
	    space_left++;
	}
// BEGIN NEW HERE - replacing how $'s are handled to make it more intelligent
// and to stop it from overwriting our buffer
      } else if((*ptr == '$')) {
        if(!t->character || (GET_LEVEL(t->character) < IMMORTAL))
        { 
          // if it's a $, and I'm a mortal, or have no character yet, handle it.
          // if there is a $, double it if there is room, and keep going
          if(space_left > 2) {
            *write_point++ = *ptr;
            *write_point++ = '$';
            space_left -= 2;
          }
          else space_left = 0; // so it truncates properly
          // do nothing, which junks the $
        }
        else {
          // gods can use $codes but only ones for color UNLESS inside a MOBProg editor
          // I have to let them use $codes inside the editor or they can't write MOBProgs
          // tmp_ptr is just so I don't have to put ptr+1 7 times....
          tmp_ptr = (ptr+1);
          if(isdigit(*tmp_ptr) || *tmp_ptr == 'I' || *tmp_ptr == 'L' ||
                                  *tmp_ptr == '*' || *tmp_ptr == 'R' ||
                                  *tmp_ptr == 'B' || t->connected == CON_EDIT_MPROG ||
				  t->connected == CON_EDITING
            )
          { // write it like normal
            *write_point++ = *ptr;
            space_left--;
          } else if(space_left > 2) { // any other code, double up the $
            *write_point++ = *ptr;
            *write_point++ = '$';
            space_left -= 2;
          }
          else space_left = 0; // if no space left, so it truncates properly
                               // do nothing, which junks the $
        }
      } else if (isascii(*ptr) && isprint(*ptr)) {
	*write_point++ = *ptr;
        space_left--;
      }
// END NEW HERE
      
/* BEGIN OLD HERE
      } else if (isascii(*ptr) && isprint(*ptr)) {
	if (((*(write_point++) = *ptr) == '$') && 
             ((*(ptr+1) != '$') || doublesign))
        {
          if(doublesign) // last one was a '$'
          {
            doublesign = 0;
            space_left--;
          }
          else
          {
	    *(write_point++) = '$';	// double the $ 
	    space_left -= 2;
          }
	} 
        else
        {
          if(*(ptr+1) == '$' && *ptr == '$')
            doublesign = 1;
	  space_left--;
        }
      }
END OLD HERE */
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
	return -1;
    }
    if (t->snoop_by) {
      SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(tmp, t->snoop_by);
      SEND_TO_Q("\r\n", t->snoop_by);
    }
    failed_subst = 0;

    if (*tmp == '!')
      strcpy(tmp, t->last_input);
    else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	strcpy(t->last_input, tmp);
    } else
      strcpy(t->last_input, tmp);

    if (!failed_subst) 
      write_to_q(tmp, &t->input, 0);
    
    /* find the end of this line */
    //while (ISNEWL(*nl_pos))
    while (ISNEWL(*nl_pos) || (t->connected != CON_WRITE_BOARD && t->connected != CON_EDITING && t->connected != CON_EDIT_MPROG && *nl_pos == '|'))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr) || (t->connected != CON_WRITE_BOARD && t->connected != CON_EDITING && t->connected != CON_EDIT_MPROG && *ptr == '|'))
	nl_pos = ptr;
      //if (ISNEWL(*ptr))
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return 1;
}



/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char new_subst[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(new_subst, orig, (strpos - orig));
  new_subst[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(new_subst, second, (MAX_INPUT_LENGTH - strlen(new_subst) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(new_subst, strpos + strlen(first), 
           (MAX_INPUT_LENGTH - strlen(new_subst) - 1));

  /* terminate the string in case of an overflow from strncat */
  new_subst[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, new_subst);

  return 0;
}

// return 1 on success
// return 0 if we quit everyone out at the bottom
int close_socket(struct descriptor_data *d)
{
  char buf[128], idiotbuf[128];
  struct descriptor_data *temp;
  // long target_idnum = -1;
  if (!d) return 0;
  flush_queues(d);
  CLOSE_SOCKET(d->descriptor);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
    d->snoop_by->snooping = NULL;
  }
  if(d->str) {
    strcpy(idiotbuf, "\n\r~\n\r");
    strcat(idiotbuf, "\0");
    string_add(d, idiotbuf);
  }
  if(d->hashstr) {
    strcpy(idiotbuf, "\n\r~\n\r");
    strcat(idiotbuf, "\0");
    string_hash_add(d, idiotbuf);
  }
  if(d->strnew) {
    strcpy(idiotbuf, "/s\n\r");
    strcat(idiotbuf, "\0");
    new_string_add(d, idiotbuf);
  }
  if (d->character) {
    // target_idnum = GET_IDNUM(d->character);
    if (d->connected == CON_PLAYING || d->connected == CON_WRITE_BOARD ||
	d->connected == CON_EDITING || d->connected == CON_EDIT_MPROG) {
      save_char_obj(d->character);
	// clan area stuff
      extern void check_quitter(CHAR_DATA *ch);
      check_quitter(d->character);
	
      // end any performances
      if(IS_SINGING(d->character))
         do_sing(d->character, "stop", 9);

      act("$n has lost $s link.", d->character, 0, 0, TO_ROOM, 0);
      sprintf(buf, "Closing link to: %s at %d.", GET_NAME(d->character),
              world[d->character->in_room].number);
      if(IS_AFFECTED(d->character, AFF_CANTQUIT))
              sprintf(buf, "%s with CQ.", buf);
      log(buf, GET_LEVEL(d->character) > SERAPH ? GET_LEVEL(d->character) : SERAPH, LOG_SOCKET);
      d->character->desc = NULL;
    } else {
      sprintf(buf, "Losing player: %s.",
	      GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
      log(buf, ANGEL, LOG_SOCKET);
      if (d->connected == CON_WRITE_BOARD || d->connected ==
		CON_EDITING || d->connected == CON_EDIT_MPROG)
	{
//		sprintf(buf, "Suspicious: %s.", 
//			GET_NAME(d->character));
//		log(buf, 110, LOG_HMM);
	}
      free_char(d->character);
    }
  }
//   Removed this log caues it's so fricken annoying
//   else
//    log("Losing descriptor without char.", ANGEL, LOG_SOCKET);

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

  // if we're closing the socket that is next to be processed, we want to
  // go ahead and move on to the next one
  if(d == next_d)
    next_d = d->next;

  REMOVE_FROM_LIST(d, descriptor_list, next);

  if (d->showstr_head)
    dc_free(d->showstr_head);
  if (d->showstr_count)
    dc_free(d->showstr_vector);

  dc_free(d);
  d = NULL;

/*  if(descriptor_list == NULL) 
  {
    // if there is NOONE on (everyone got disconnected) loop through and
    // boot all of the linkdeads.  That way if the mud's link is cut, the
    // first person back on can't RK everyone
    char_data * next_i;
    for(char_data * i = character_list; i; i = next_i) {
       next_i = i->next;
       if(IS_NPC(i))
         continue;
       do_quit(i, "", 666);
    }
    return 0;
  }*/
  return 1;
}



void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_GET_OLD_PASSWORD && STATE(d) != CON_GET_NAME)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      continue;
    } else {
      echo_on(d);
      SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
      STATE(d) = CON_CLOSE;
    }
  }
}

/* ******************************************************************
*  signal-handling functions (formerly signals.c)                   *
****************************************************************** */


void checkpointing(int sig)
{
  sig = sig;
  if (!tics) {
    log("SYSERR: CHECKPOINT shutdown: tics not updated", ANGEL, LOG_BUG);
    abort();
  } else
    tics = 0;
}

void report_debug_logging()
{
  extern char last_processed_cmd[MAX_INPUT_LENGTH];
  extern char last_char_name[MAX_INPUT_LENGTH];
  extern int  last_char_room;

  log("Last cmd:", ANGEL, LOG_BUG);
  log(last_processed_cmd, ANGEL, LOG_BUG);
  log("Owner's Name:", ANGEL, LOG_BUG);
  log(last_char_name, ANGEL, LOG_BUG);
  logf(ANGEL, LOG_BUG, "Last room: %d", last_char_room);
}

void crash_hotboot()
{
  struct descriptor_data * d = NULL;
  extern int try_to_hotboot_on_crash;
  extern int died_from_sigsegv;

  // This can be dangerous, because if we had a SIGSEGV due to a descriptor being
  // invalid, we're going to do it again.  That's why we put in extern int died_from_sigsegv
  // sigsegv = # of times we've crashed from SIGSEGV

  for (d=descriptor_list;d && died_from_sigsegv < 2;d=d->next) {
    write_to_descriptor(d->descriptor,"Mud crash detected.\n\r");
  }

  // attempt to hotboot
  if(try_to_hotboot_on_crash) {
    for (d=descriptor_list;d && died_from_sigsegv < 2;d=d->next) {
      write_to_descriptor(d->descriptor,"Attempting to recover with a hotboot.\n\r");
    }
    log("Attempting to hotboot from the crash.", ANGEL, LOG_BUG);
    write_hotboot_file();
    // we shouldn't return from there unless we failed
    log("Hotboot crash recovery failed.  Exiting.", ANGEL, LOG_BUG);
    for (d=descriptor_list;d && died_from_sigsegv < 2;d=d->next) {
      write_to_descriptor(d->descriptor,"Hotboot failed giving up.\n\r");
    }
  }

  for (d=descriptor_list;d && died_from_sigsegv < 2;d=d->next) {
    write_to_descriptor(d->descriptor,"Giving up, goodbye.\n\r");
  }
}

void crashill(int sig)
{
  sig = sig;
  report_debug_logging();
  log("Recieved SIGFPE (Illegal Instruction)", ANGEL, LOG_BUG);
  crash_hotboot();
  log("Mud exiting from SIGFPE.", ANGEL, LOG_BUG);
  exit(0);
}

void crashfpe(int sig)
{
  sig = sig;
  report_debug_logging();
  log("Recieved SIGFPE (Arithmetic Error)", ANGEL, LOG_BUG);
  crash_hotboot();
  log("Mud exiting from SIGFPE.", ANGEL, LOG_BUG);
  exit(0);
}

void crashsig(int sig)
{
  extern int died_from_sigsegv;
  sig = sig;
  died_from_sigsegv++;
  if(died_from_sigsegv > 3) { // panic! error is in log...lovely  just give up
    exit(0);
  }
  if(died_from_sigsegv > 2) { // panic! try to log and get out
    log("Hit 'died_from_sigsegv > 2'", ANGEL, LOG_BUG);
    exit(0);
  }
  report_debug_logging();
  log("Recieved SIGSEGV (Segmentation fault)", ANGEL, LOG_BUG);
  crash_hotboot();
  log("Mud exiting from SIGSEGV.", ANGEL, LOG_BUG);
  exit(0);
}

void unrestrict_game(int sig)
{
  sig = sig;
  extern struct ban_list_element *ban_list;
  extern int num_invalid;

  log("Received SIGUSR2 - completely unrestricting game (emergent)",
       ANGEL, LOG_GOD);
  ban_list = NULL;
  restrict = 0;
  num_invalid = 0;
}


void hupsig(int sig)
{
  sig = sig;
  log("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...", 0, LOG_MISC);
  abort();			/* perhaps something more elegant should
				 * substituted */
}

#ifndef WIN32
void sigchld(int sig)
{
  struct rusage ru;
  wait3(NULL, WNOHANG, &ru);
}
#endif
/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#define my_signal(signo, func) signal(signo, func)


void signal_setup(void)
{
#ifndef WIN32
  // struct timeval interval;


  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.  Doesn't work with
   * OS/2.
   */
  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
  signal(SIGCHLD, sigchld); // hopefully kill zombies

//  my_signal(SIGSEGV, crashsig);  // catch null->blah
  my_signal(SIGFPE,  crashfpe);  // catch x / 0
  my_signal(SIGILL,  crashill);  // catch illegal instruction
#endif
}


/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */

void send_to_char_regardless(char *messg, CHAR_DATA *ch) {
  if(ch->desc && messg) {
    SEND_TO_Q(messg, ch->desc);
  }
}

void send_to_char_nosp(char *messg, struct char_data *ch)
{
  char *tmp = str_nospace(messg);
  send_to_char(tmp, ch);
  dc_free(tmp);
}

void record_msg(char *messg, struct char_data *ch)
{
  if (!messg || IS_NPC(ch) || GET_LEVEL(ch) < IMMORTAL)
    return;

  if (ch->pcdata->away_msgs == 0) {
    ch->pcdata->away_msgs = new std::queue<char *>();
  }

  if (ch->pcdata->away_msgs->size() < 1000) {
    char *our_copy = str_dup(messg);
    ch->pcdata->away_msgs->push(our_copy);
  }
}

int do_awaymsgs(CHAR_DATA *ch, char *argument, int cmd)
{
  int lines = 0;
  char *tmp;
  
  if (IS_NPC(ch))
    return eFAILURE;
  
  if ((ch->pcdata->away_msgs == 0) || ch->pcdata->away_msgs->empty()) {
    SEND_TO_Q("No messages have been recorded.\n\r", ch->desc);
    return eSUCCESS;
  }

  // Show 23 lines of text, then stop
  while (! ch->pcdata->away_msgs->empty()) {
    tmp = ch->pcdata->away_msgs->front();
    SEND_TO_Q(tmp, ch->desc);
    dc_free(tmp);
    ch->pcdata->away_msgs->pop();
    
    if (++lines == 23) {
      SEND_TO_Q("\n\rMore msgs available. Type awaymsgs to see them\n\r",
		ch->desc);
      break;
    }
  }

  return eSUCCESS;
}

void check_for_awaymsgs(struct char_data *ch)
{
  if (!ch)
    return;

  if (IS_NPC(ch))
    return;

  if ((ch->pcdata->away_msgs == 0) || ch->pcdata->away_msgs->empty()) {
    return;
  }

  send_to_char("You have unviewed away messages. ", ch);
  send_to_char("Type awaymsgs to view them.\n\r", ch);
}

void send_to_char(char *messg, struct char_data *ch)
{
  extern bool selfpurge;
  if (IS_NPC(ch) && !ch->desc && MOBtrigger && messg)
    mprog_act_trigger( messg, ch, 0, 0, 0 );
  if (IS_NPC(ch) && !ch->desc && !selfpurge && MOBtrigger && messg)
    oprog_act_trigger( messg, ch);

  if (!selfpurge && (ch->desc && messg) && (!is_busy(ch))) {
    SEND_TO_Q(messg, ch->desc);
  }
}

void send_to_all(char *messg)
{
  struct descriptor_data *i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected)
	SEND_TO_Q(messg, i);
}

void ansi_color( char *txt, CHAR_DATA *ch )
{
    // mobs don't have toggles, so they automatically get ansi on
    if ( txt != NULL && ch->desc != NULL ) {
       if ( !IS_MOB(ch) && 
            !IS_SET(GET_TOGGLES(ch), PLR_ANSI) &&
            !IS_SET(GET_TOGGLES(ch), PLR_VT100) )
          return;
       else if (!IS_MOB(ch) && 
                 IS_SET(GET_TOGGLES(ch), PLR_VT100) &&
                !IS_SET(GET_TOGGLES(ch), PLR_ANSI ) )
      {
         if ( ( txt == GREEN )
           || ( txt == RED )
           || ( txt == BLUE )
           || ( txt == BLACK )
           || ( txt == CYAN )
           || ( txt == GREY )
           || ( txt == EEEE )
           || ( txt == YELLOW )
           || ( txt == PURPLE ) ) return;
      }
     send_to_char( txt, ch );
     return;
    }
}

void send_info(char *messg)
{
    struct descriptor_data *i;

    if(messg)
      for(i = descriptor_list; i; i = i->next) {
         if(!(i->character) ||
            !IS_SET(i->character->misc, CHANNEL_INFO) )
           continue;
         if((!i->connected) && !is_busy(i->character))
           SEND_TO_Q(messg, i);
      }
}


void send_to_outdoor(char *messg) {
    struct descriptor_data *i;

    if (messg)
        for (i = descriptor_list; i; i = i->next)
            if (!i->connected)
                if (OUTSIDE(i->character) && !is_busy(i->character))
                    SEND_TO_Q(messg, i);
}

void send_to_zone(char *messg, int zone)
{
   struct descriptor_data *i = NULL; 
   if(messg) 
   {
      for(i = descriptor_list; i; i = i->next) 
      {
         if(!i->connected && !is_busy(i->character) && i->character->in_room != NOWHERE
                          && world[i->character->in_room].zone == zone) 
         {
            SEND_TO_Q(messg, i);
         }
      }
   }
}

void send_to_room(char *messg, int room, bool awakeonly, CHAR_DATA *nta)
{
    CHAR_DATA *i = NULL;

    //If a megaphone goes off when in someone's inventory this happens
    if (room == -1)
      return;

    if(!world_array[room] || !world[room].people) {
        return;
    }
    if (messg)
        for (i = world[room].people; i; i = i->next_in_room)
            if (i->desc && !is_busy(i) && nta != i)
               if(!awakeonly || GET_POS(i) > POSITION_SLEEPING)
                  SEND_TO_Q(messg, i->desc);
}

int is_busy(CHAR_DATA *ch)
{

  if(ch->desc &&
     ((ch->desc->connected == CON_WRITE_BOARD) ||
      (ch->desc->connected == CON_SEND_MAIL) ||
      (ch->desc->connected == CON_EDITING) ||
      (ch->desc->connected == CON_EDIT_MPROG)
     )
    )
    return 1;

  return(0);
}

int perform_alias(struct descriptor_data *d, char *orig) {
     char first_arg[MAX_INPUT_LENGTH], new_buf[MAX_INPUT_LENGTH], *ptr;
     ptr = any_one_arg(orig, first_arg);
     struct char_player_alias * x;
     int lengthpre;
     int lengthpost;
     
     if(!*first_arg) {
         return(0);
     }
     if(IS_MOB(d->character) || !d->character->pcdata->alias)
         return(0);

     for (x=d->character->pcdata->alias; x ; x = x->next) {
         if (x->keyword)
            if (!strcmp(x->keyword, first_arg)) {
               strcpy(new_buf, x->command);

               // make sure the new command still fits in our buffers
               lengthpre = strlen(new_buf);
               lengthpost = strlen(ptr);
               if(lengthpre + lengthpost > MAX_INPUT_LENGTH - 1)
               {
                  ptr[MAX_INPUT_LENGTH - lengthpre - 1] = '\0'; // truncate it to fit
                  strcat(new_buf, ptr);
                  strcat(new_buf, "\0");
                  strcpy(orig, new_buf);
                  SEND_TO_Q("Line too long.  Truncated to:\r\n", d);
                  SEND_TO_Q(orig, d);
                  SEND_TO_Q("\r\n", d);
               }
               else 
               {
                  strcat(new_buf, ptr);
                  strcat(new_buf, "\0");
                  strcpy(orig, new_buf);
               }
            }
        }
    return(0);
}

void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';
  return argument;
}

void warn_if_duplicate_ip(char_data * ch)
{
   char buf[256];
   int highlev = 51;
   for(descriptor_data * d = descriptor_list; d; d = d->next) 
   {
      if( d->character && 
          strcmp(GET_NAME(ch), GET_NAME(d->character)) &&
          !strcmp(d->host, ch->desc->host)
        )
      {
	highlev = MAX(GET_LEVEL(d->character), GET_LEVEL(ch));
	highlev = MAX(highlev, OVERSEER);
       // sprintf(buf, "MultipleIP: %s -> %s (%d)/ %s (%d)", d->host, GET_NAME(ch), 
      //                  (world[ch->in_room].number ? world[ch->in_room].number : -1), GET_NAME(d->character),
      //                  (world[d->character->in_room].number ? world[d->character->in_room].number : -1));

        sprintf(buf, "MultipleIP: %s -> %s / %s ", d->host, GET_NAME(ch), GET_NAME(d->character));
        log(buf, highlev, LOG_WARNINGS );
      }
   }

}
