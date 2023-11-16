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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>

#include <sstream>
#include <iostream>
#include <list>
#include <algorithm>
#include <string>
#include <queue>
#include <cstdint>

#include <fmt/format.h>
#include <QTimer>
#include <QHttpServer>
#include <QtConcurrent>
#include <QMap>

#include "terminal.h"
#include "fileinfo.h"
#include "act.h"
#include "player.h"
#include "levels.h"
#include "room.h"
#include "structs.h"
#include "utility.h"
#include "connect.h"
#include "interp.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "returnvals.h"
#include "quest.h"
#include "shop.h"
#include "Leaderboard.h"
#include "Timer.h"
#ifdef USE_SQL
#include "Backend/Database.h"
#endif
#include "Timer.h"
#include "DC.h"
#include "CommandStack.h"
#include "SSH.h"

using namespace std;

struct multiplayer
{
  QHostAddress host;
  char *name1;
  char *name2;
};

using namespace std;

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

extern bool MOBtrigger;

// This is turned on right before we call game_loop
int do_not_save_corpses = 1;
int try_to_hotboot_on_crash = 0;
int was_hotboot = 0;
int died_from_sigsegv = 0;

/* these are here for the eventual addition of ban */
int num_invalid = 0;
int restrict = 0;

/* externs */
extern int restrict;
// extern int mini_mud;
// extern int no_rent_check;

/* In db.c */
extern const char *sector_types[];
extern char *time_look[];
extern char *sky_look[];

extern string last_char_name;
extern string last_processed_cmd;
extern struct index_data *obj_index;

void check_champion_and_website_who_list(void);
void save_slot_machines(void);
void check_silence_beacons(void);

/* local globals */
struct txt_block *bufpool = 0; /* pool of large output buffers */
int buf_largecount = 0;        /* # of large buffers which exist */
int buf_overflows = 0;         /* # of overflows of output */
int buf_switches = 0;          /* # of switches from small to large buf */
int _shutdown = 0;             /* clean shutdown */
int tics = 0;                  /* for extern checkpointing */
// int nameserver_is_slow = 0;	/* see config.c */
// extern int auto_save;		/* see config.c */
// extern int autosave_time;	/* see config.c */
struct timeval null_time; /* zero-valued time structure */
time_t start_time;

// heartbeat globals
int pulse_timer;
int pulse_mobile;
int pulse_bard;
int pulse_violence;
int pulse_tensec;
int pulse_weather;
int pulse_regen;
int pulse_time;
int pulse_short; // short timer, for archery

#ifdef USE_SQL
Database db;
#endif

/* functions in this file */
void update_mprog_throws(void);
void update_bard_singing(void);
void update_characters(void);
void short_activity();
void skip_spaces(char **string);
char *any_one_arg(char *argument, char *first_arg);
char *calc_color(int hit, int max_hit);
string generate_prompt(Character *ch);
// string generate_prompt(Character *ch);
string get_from_q(queue<string> &input_queue);
void signal_setup(void);
int new_descriptor(int s);
int process_output(class Connection *t);
int process_input(class Connection *t);
void flush_queues(class Connection *d);
int perform_subst(class Connection *t, char *orig, char *subst);
string perform_alias(class Connection *d, string orig);
void check_idle_passwords(void);
void init_heartbeat();
void heartbeat();
void report_debug_logging();

/* extern fcnts */
void pulse_takeover(void);
void zone_update(void);
void affect_update(int32_t duration_type); /* In spells.c */
void point_update(void);                   /* In limits.c */
void food_update(void);                    /* In limits.c */
void mobile_activity(void);
void object_activity(uint64_t pulse_type);
void update_corpses_and_portals(void);
void string_hash_add(class Connection *d, char *str);
void perform_violence(void);
void time_update();
void weather_update();
void send_hint();
extern void pulse_command_lag();
void checkConsecrate(int);
void update_max_who(void);

// extern char greetings1[MAX_STRING_LENGTH];
// extern char greetings2[MAX_STRING_LENGTH];
// extern char greetings3[MAX_STRING_LENGTH];
// extern char greetings4[MAX_STRING_LENGTH];

#ifdef WIN32
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
  DWORD millisec = GetTickCount();
  t->tv_sec = (int)(millisec / 1000);
  t->tv_usec = (millisec % 1000) * 1000;
}
#endif

// writes all the descriptors to file so we can open them back up after
// a reboot
int write_hotboot_file(char **new_argv)
{
  FILE *fp;
  class Connection *d;
  class Connection *sd;
  /* Azrack -- do these need to be here?
  extern int mother_desc;
  extern int other_desc;
  extern int third_desc;
  */
  //  extern char ** ext_argv;

  if ((fp = fopen("hotboot", "w")) == nullptr)
  {
    logentry("Hotboot failed, unable to open hotboot file.", 0, LogChannels::LOG_MISC);
    return 0;
  }

  DC *dc = dynamic_cast<DC *>(DC::instance());

  // for_each(dc.server_descriptor_list.begin(), dc.server_descriptor_list.end(), [fp](server_descriptor_list_i i)
  for_each(dc->server_descriptor_list.begin(), dc->server_descriptor_list.end(), [&fp](const int &fd)
           { fprintf(fp, "%d\n", fd); });

  for (d = DC::getInstance()->descriptor_list; d; d = sd)
  {
    sd = d->next;
    if (STATE(d) != Connection::states::PLAYING || !d->character || GET_LEVEL(d->character) < 1)
    {
      // Kick out anyone not currently playing in the game.
      write_to_descriptor(d->descriptor, "We are rebooting, come back in a minute.");
      close_socket(d);
    }
    else
    {
      STATE(d) = Connection::states::PLAYING; // if editors.
      if (d->original)
      {
        fprintf(fp, "%d\n%s\n%s\n", d->descriptor, GET_NAME(d->original), d->getPeerOriginalAddress().toString().toStdString().c_str());
        if (d->original->player)
        {
          if (d->original->player->last_site)
            dc_free(d->original->player->last_site);
#ifdef LEAK_CHECK
          d->original->player->last_site = (char *)calloc(strlen(d->getPeerOriginalAddress().toString().toStdString().c_str()) + 1, sizeof(char));
#else
          d->original->player->last_site = (char *)dc_alloc(strlen(d->getPeerOriginalAddress().toString().toStdString().c_str()) + 1, sizeof(char));
#endif
          strcpy(d->original->player->last_site, d->original->desc->getPeerOriginalAddress().toString().toStdString().c_str());
          d->original->player->time.logon = time(0);
        }
        save_char_obj(d->original);
      }
      else
      {
        fprintf(fp, "%d\n%s\n%s\n", d->descriptor, GET_NAME(d->character), d->getPeerOriginalAddress().toString().toStdString().c_str());
        if (d->character->player)
        {
          if (d->character->player->last_site)
            dc_free(d->character->player->last_site);
#ifdef LEAK_CHECK
          d->character->player->last_site = (char *)calloc(strlen(d->getPeerOriginalAddress().toString().toStdString().c_str()) + 1, sizeof(char));
#else
          d->character->player->last_site = (char *)dc_alloc(strlen(d->getPeerOriginalAddress().toString().toStdString().c_str()) + 1, sizeof(char));
#endif
          strcpy(d->character->player->last_site, d->character->desc->getPeerOriginalAddress().toString().toStdString().c_str());
          d->character->player->time.logon = time(0);
        }
        save_char_obj(d->character);
      }
      write_to_descriptor(d->descriptor, "Attempting to maintain your link during reboot.\r\nPlease wait..");
    }
  }
  fclose(fp);
  logentry("Hotboot descriptor file successfully written.", 0, LogChannels::LOG_MISC);

  chdir("../bin/");

  char *cwd = get_current_dir_name();
  if (cwd != nullptr)
  {
    logentry(QString("Hotbooting %1 at [%2]").arg(DC::getInstance()->applicationFilePath()).arg(cwd), 108, LogChannels::LOG_GOD);
    free(cwd);
  }

  QStringList arguments = DC::getInstance()->arguments();
  char **argv = new char *[arguments.size() + 1];
  for (auto i = 0; i < arguments.size(); ++i)
  {
    auto size = arguments.at(i).length();
    argv[i] = new char[size + 1];
    strncpy(argv[i], arguments.at(i).toStdString().c_str(), size);
    argv[i][size] = '\0';
  }
  argv[arguments.size()] = nullptr;

  DC::getInstance()->ssh.close();
  char *const *argv2 = argv;
  if (execv(DC::getInstance()->applicationFilePath().toStdString().c_str(), argv2) == -1)
  {
    char execv_strerror[1024] = {};
    strerror_r(errno, execv_strerror, sizeof(execv_strerror));

    logentry(QString("Hotboot execv(%1, argv) failed with error: %2").arg(DC::getInstance()->applicationFilePath()).arg(execv_strerror), 0, LogChannels::LOG_MISC);

    // wipe the file since we can't use it anyway
    if (unlink("hotboot") == -1)
    {
      char unlink_strerror[1024] = {};
      strerror_r(errno, unlink_strerror, sizeof(unlink_strerror));

      logentry(QString("Hotboot unlink(\"hotboot\") failed with error: %1").arg(unlink_strerror), 0, LogChannels::LOG_MISC);
    }

    for (auto i = 0; i < arguments.size(); ++i)
    {
      delete argv[i];
    }
    delete argv;

    chdir(DFLT_DIR);
    return 0;
  }

  return 1;
}

// attempts to read in the descs written to file, and reconnect their
// links to the mud.
int load_hotboot_descs()
{
  string chr = {};
  char host[MAX_INPUT_LENGTH] = {}, buf[MAX_STRING_LENGTH] = {};
  int desc = {};
  class Connection *d = nullptr;
  DC *dc = dynamic_cast<DC *>(DC::instance());
  ifstream ifs;

  ifs.exceptions(ifstream::eofbit | ifstream::failbit | ifstream::badbit);

  try
  {
    ifs.open("hotboot");
    unlink("hotboot");
    logentry("Hotboot, reloading characters.", 0, LogChannels::LOG_MISC);

    for_each(dc->cf.ports.begin(), dc->cf.ports.end(), [&dc, &ifs](in_port_t &port)
             {
             int fd;
            ifs >> fd;
            dc->server_descriptor_list.insert(fd); });

    while (ifs.good())
    {
      desc = 0;
      chr.clear();
      *host = '\0';
      try
      {
        ifs >> desc;
        ifs >> chr;
        ifs >> host;
      }
      catch (ifstream::failure::runtime_error)
      {
        break;
      }

      // fscanf(fp, "%d\n%s\n%s\n", &desc, chr.data(), host);
      d = new Connection;

      d->idle_time = 0;
      d->idle_tics = 0;
      d->wait = 1;
      d->prompt_mode = 1;
      d->output = {};
      //    *d->output                 = '\0';
      d->input = queue<string>();
      d->output = chr; // store it for later
      d->login_time = time(0);

      if (write_to_descriptor(desc, "Recovering...\r\n") == -1)
      {
        sprintf(buf, "Host %s Char %s Desc %d FAILED to recover from hotboot.", host, chr, desc);
        logentry(buf, 0, LogChannels::LOG_MISC);
        CLOSE_SOCKET(desc);
        delete d;
        d = nullptr;
        continue;
      }

      d->setPeerAddress(QHostAddress(host));
      d->descriptor = desc;

      // we need a second to be sure
      if (-1 == write_to_descriptor(d->descriptor, "Link recovery successful.\n\rPlease wait while mud finishes rebooting...\r\n"))
      {
        sprintf(buf, "Host %s Char %s Desc %d failed to recover from hotboot.", host, chr, desc);
        logentry(buf, 0, LogChannels::LOG_MISC);
        CLOSE_SOCKET(desc);
        dc_free(d);
        d = nullptr;
        continue;
      }

      d->next = DC::getInstance()->descriptor_list;
      DC::getInstance()->descriptor_list = d;
    }
    ifs.close();
  }
  catch (...)
  {
    logentry("Hotboot file missing/unopenable.", 0, LogChannels::LOG_MISC);
    return false;
  }

  unlink("hotboot"); // if the above unlink failed somehow(?),
                     // remove the hotboot file so that it dosen't think
                     // next reboot is another hotboot
  logentry("Successful hotboot file read.", 0, LogChannels::LOG_MISC);
  return 1;
}

void finish_hotboot()
{
  class Connection *d;
  char buf[MAX_STRING_LENGTH];

  void do_on_login_stuff(Character * ch);

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    write_to_descriptor(d->descriptor, "Reconnecting your link to your character...\r\n");

    if (!load_char_obj(d, d->output.c_str()))
    {
      sprintf(buf, "Could not load char '%s' in hotboot.", d->output);
      logentry(buf, 0, LogChannels::LOG_MISC);
      write_to_descriptor(d->descriptor, "Link Failed!  Tell an Immortal when you can.\r\n");
      close_socket(d);
      continue;
    }

    write_to_descriptor(d->descriptor, "Success...May your visit continue to suck...\r\n");

    d->output.clear();

    auto &character_list = DC::getInstance()->character_list;
    character_list.insert(d->character);

    do_on_login_stuff(d->character);

    STATE(d) = Connection::states::PLAYING;

    update_max_who();
  }

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    do_look(d->character, "", 8);
    d->character->save(666);
  }
}

/* Init sockets, run game, and cleanup sockets */
void DC::init_game(void)
{

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
  void free_command_radix_nodes(struct cmd_hash_info * curr);
  void free_ban_list_from_memory();
  void free_buff_pool_from_memory();
  extern cmd_hash_info *cmd_radix;
#endif

  FILE *fp;
  // create boot'ing lockfile
  if ((fp = fopen("died_in_bootup", "w")))
  {
    fclose(fp);
  }

  logentry("Attempting to load hotboot file.", 0, LogChannels::LOG_MISC);

  if (load_hotboot_descs())
  {
    logentry("Hotboot Loading complete.", 0, LogChannels::LOG_MISC);
    was_hotboot = 1;
  }
  else
  {
    logentry("Hotboot failed.  Starting regular sockets.", 0, LogChannels::LOG_MISC);
    logentry("Opening mother connections.", 0, LogChannels::LOG_MISC);

    for_each(cf.ports.begin(), cf.ports.end(), [this](in_port_t &port)
             {
               logf(0, LogChannels::LOG_MISC, "Opening port %d.", port);
               int listen_fd = init_socket(port);
               if (listen_fd >= 0)
               {
                 server_descriptor_list.insert(listen_fd);
               }
               else
               {
                 logf(0, LogChannels::LOG_MISC, "Error opening port %d.", port);
               } });
  }

  start_time = time(0);
  boot_db();

  if (was_hotboot)
  {
    logentry("Connecting hotboot characters to their descriptiors", 0, LogChannels::LOG_MISC);
    finish_hotboot();
  }

  logentry("Signal trapping.", 0, LogChannels::LOG_MISC);
  signal_setup();

  // we got all the way through, let's turn auto-hotboot back on
  try_to_hotboot_on_crash = 1;

  logentry("Entering game loop.", 0, LogChannels::LOG_MISC);

  unlink("died_in_bootup");

  if (DC::getInstance()->cf.bport == false)
  {
    do_not_save_corpses = 0;
  }
  game_loop_init();
  do_not_save_corpses = 1;

  logentry("Closing all sockets.", 0, LogChannels::LOG_MISC);
  while (DC::getInstance()->descriptor_list)
  {
    close_socket(DC::getInstance()->descriptor_list);
  }

  for_each(server_descriptor_list.begin(), server_descriptor_list.end(), [](const int &fd)
           {
             logf(0, LogChannels::LOG_MISC, "Closing fd %d.", fd);
             CLOSE_SOCKET(fd); });
#ifdef LEAK_CHECK

  logentry("Freeing all mobs in world.", 0, LogChannels::LOG_MISC);
  remove_all_mobs_from_world();
  logentry("Freeing all objs in world.", 0, LogChannels::LOG_MISC);
  remove_all_objs_from_world();
  logentry("Freeing socials from memory.", 0, LogChannels::LOG_MISC);
  clean_socials_from_memory();
  logentry("Freeing zones data.", 0, LogChannels::LOG_MISC);
  free_zones_from_memory();
  logentry("Freeing clan data.", 0, LogChannels::LOG_MISC);
  free_clans_from_memory();
  logentry("Freeing the world.", 0, LogChannels::LOG_MISC);
  free_world_from_memory();
  logentry("Freeing mobs from memory.", 0, LogChannels::LOG_MISC);
  free_mobs_from_memory();
  logentry("Freeing objs from memory.", 0, LogChannels::LOG_MISC);
  free_objs_from_memory();
  logentry("Freeing messages from memory.", 0, LogChannels::LOG_MISC);
  free_messages_from_memory();
  logentry("Freeing hash tree from memory.", 0, LogChannels::LOG_MISC);
  free_hsh_tree_from_memory();
  logentry("Freeing wizlist from memory.", 0, LogChannels::LOG_MISC);
  free_wizlist_from_memory();
  logentry("Freeing help index.", 0, LogChannels::LOG_MISC);
  free_help_from_memory();
  logentry("Freeing shops from memory.", 0, LogChannels::LOG_MISC);
  free_shops_from_memory();
  logentry("Freeing emoting objects from memory.", 0, LogChannels::LOG_MISC);
  free_emoting_objects_from_memory();
  logentry("Freeing game portals from memory.", 0, LogChannels::LOG_MISC);
  free_game_portals_from_memory();
  logentry("Freeing command radix from memory.", 0, LogChannels::LOG_MISC);
  free_command_radix_nodes(cmd_radix);
  logentry("Freeing ban list from memory.", 0, LogChannels::LOG_MISC);
  free_ban_list_from_memory();
  logentry("Freeing the bufpool.", 0, LogChannels::LOG_MISC);
  free_buff_pool_from_memory();
  DC::getInstance()->removeDead();
#endif

  logentry("Goodbye.", 0, LogChannels::LOG_MISC);
  logentry("Normal termination of game.", 0, LogChannels::LOG_MISC);
}

/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int DC::init_socket(in_port_t port)
{
  int fd, opt;
  struct sockaddr_in sa;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Error creating socket");
    exit(1);
  }

  opt = LARGE_BUFSIZE + GARBAGE_SPACE;
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt)) < 0)
  {
    perror("setsockopt SNDBUF");
    exit(1);
  }

  opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
  {
    perror("setsockopt REUSEADDR");
    exit(1);
  }

  struct linger ld;
  ld.l_onoff = 0;
  ld.l_linger = 0;
  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld)) < 0)
  {
    perror("setsockopt LINGER");
    exit(1);
  }

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
  {
    perror("bind");
    CLOSE_SOCKET(fd);
    exit(1);
  }

  if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
  {
    perror("init_socket : fcntl : nonblock");
    exit(1);
  }

  if (listen(fd, 5) < 0)
  {
    perror("init_socket : listen");
    exit(1);
  }
  return fd;
}

// This is set as a global...it is the increment variable for the descriptor list
// used in game_loop.  It has to be global so that "close_socket" can increment
// it if we are closing the socket that is next to be processed.
class Connection *next_d;
stringstream timingDebugStr;
uint64_t pulseavg = 0;

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" function
 */

void DC::game_loop(void)
{
  static QElapsedTimer last_execution;
  if (last_execution.isValid())
  {
    quint64 nsecs_expired = last_execution.nsecsElapsed();
    double msecs_expired = 0;
    if (nsecs_expired >= 1000000)
    {
      msecs_expired = nsecs_expired / 1000000.0;
      // qDebug() << QString("%1 msec.").arg(msecs_expired);
    }
    else
    {
      // qDebug() << QString("%1 nsec.").arg(nsecs_expired);
    }

    last_execution.restart();
  }
  else
  {
    last_execution.start();
  }

  // comm must be much longer than MAX_INPUT_LENGTH since we allow aliases in-game
  // otherwise an alias'd command could easily overrun the buffer
  string comm = {};
  char buf[128] = {};
  class Connection *d = {};
  int maxdesc = {};
  int aliased = false;

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  PerfTimers["gameloop"].start();
  // Set CommandStack to track a command stack with max queue depth of 20
  CommandStack cstack(0, 5);

  selfpurge = false;
  // Set up the input, output, and exception sets for select().
  FD_ZERO(&input_set);
  FD_ZERO(&output_set);
  FD_ZERO(&exc_set);

  maxdesc = 0;
  fd_set &input_set = this->input_set;
  for_each(server_descriptor_list.begin(), server_descriptor_list.end(), [&input_set, &maxdesc](const int &fd)
           {
               FD_SET(fd, &input_set);
               if (fd > maxdesc)
               {
                 maxdesc = fd;
               } });

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    if (d->descriptor > maxdesc)
      maxdesc = d->descriptor;
    FD_SET(d->descriptor, &input_set);
    FD_SET(d->descriptor, &output_set);
    FD_SET(d->descriptor, &exc_set);
  }

  // poll (without blocking) for new input, output, and exceptions
  if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0)
  {
    int save_errno = errno;
    perror("game_loop : select : poll");
    if (save_errno != EINTR)
    {
      return;
    }
  }

  // If new connection waiting, accept it
  for_each(server_descriptor_list.begin(), server_descriptor_list.end(), [this, &input_set](const int &fd)
           {
               if (FD_ISSET(fd, &input_set))
               {
                 new_descriptor(fd);
               } });

  // close the weird descriptors in the exception set
  for (d = DC::getInstance()->descriptor_list; d; d = next_d)
  {
    next_d = d->next;
    if (FD_ISSET(d->descriptor, &exc_set))
    {
      FD_CLR(d->descriptor, &input_set);
      FD_CLR(d->descriptor, &output_set);
      close_socket(d);
    }
  }

  /* process descriptors with input pending */
  for (d = DC::getInstance()->descriptor_list; d; d = next_d)
  {
    next_d = d->next;
    if (FD_ISSET(d->descriptor, &input_set))
    {
      if (process_input(d) < 0)
      {
        if (d->getPeerOriginalAddress() != QHostAddress("127.0.0.1"))
        {
          sprintf(buf, "Connection attempt bailed from %s", d->getPeerOriginalAddress().toString().toStdString().c_str());
          printf(buf);
          logentry(buf, 111, LogChannels::LOG_SOCKET);
        }
        close_socket(d);
      }
    }
  }

  /* process commands we just read from process_input */
  for (d = DC::getInstance()->descriptor_list; d; d = next_d)
  {
    if (d->character != nullptr)
    {
      QString locale = d->character->getSetting("locale", "en_US");
      QLocale::setDefault(QLocale(locale));
    }

    next_d = d->next;
    d->wait = MAX(d->wait, 1);
    if (d->connected == Connection::states::CLOSE)
    {
      close_socket(d); // So they don't have to type a command.
      continue;
    }
    --(d->wait);
    if (STATE(d) == Connection::states::QUESTION_ANSI ||
        STATE(d) == Connection::states::QUESTION_SEX ||
        STATE(d) == Connection::states::QUESTION_STAT_METHOD ||
        STATE(d) == Connection::states::NEW_STAT_METHOD ||
        STATE(d) == Connection::states::OLD_STAT_METHOD ||
        STATE(d) == Connection::states::QUESTION_RACE ||
        STATE(d) == Connection::states::QUESTION_CLASS ||
        STATE(d) == Connection::states::QUESTION_STATS ||
        STATE(d) == Connection::states::NEW_PLAYER)
    {
      nanny(d);
    }
    else if ((d->wait <= 0) && !d->input.empty())
    {

      comm = get_from_q(d->input);
#ifdef DEBUG_INPUT
      // cerr << "Got command [" << comm << "] from the d->input queue" << endl;
#endif
      /* reset the idle timer & pull char back from void if necessary */
      d->wait = 1;
      d->prompt_mode = 1;

      if (d->showstr_count) /* reading something w/ pager     */
        show_string(d, comm.data());
      //	else if (d->str)		/* writing boards, mail, etc.     */
      //	  string_add(d, comm);
      else if (d->strnew && STATE(d) == Connection::states::EXDSCR)
        new_string_add(d, comm.data());
      else if (d->hashstr)
        string_hash_add(d, comm.data());
      else if (d->strnew && (IS_MOB(d->character) || !DC::isSet(d->character->player->toggles, Player::PLR_EDITOR_WEB)))
        new_string_add(d, comm.data());
      else if (d->connected != Connection::states::PLAYING) /* in menus, etc. */
        nanny(d, comm);
      else
      {              /* else: we're playing normally */
        if (aliased) /* to prevent recursive aliases */
          d->prompt_mode = 0;
        else
        {
          comm = perform_alias(d, comm);
        }
        PerfTimers["command"].start();
        // Azrack's a chode.  Don't forget to check
        // ->snooping before you check snooping->char:P
        if (!comm.empty() && comm[0] == '%' && d->snooping && d->snooping->character)
        {
          command_interpreter(d->snooping->character, comm.substr(1));
        }
        else
        {
          command_interpreter(d->character, comm); /* send it to interpreter */
        }
        PerfTimers["command"].stop();

      } // else if input
    }   // if input
    // the below two if-statements are used to allow the mud to detect and respond
    // to web-browsers attempting to connect to the game on port 80
    // this line processes a "get" or "post" if available.  Otherwise it prints the
    // entrance screen.  If a player has already entered their name, it processes
    // that too.
    else if (d->connected == Connection::states::DISPLAY_ENTRANCE)
      nanny(d, "");
    // this line allows the mud to skip this descriptor until next pulse
    else if (d->connected == Connection::states::PRE_DISPLAY_ENTRANCE)
      d->connected = Connection::states::DISPLAY_ENTRANCE;
    else
      d->idle_time++;
  } // for

  // do what needs to be done.  violence, repoping, regen, etc.
  PerfTimers["heartbeat"].start();
  heartbeat();
  PerfTimers["heartbeat"].stop();

#ifdef USE_SQL
  PerfTimers["db"].start();
  db.processqueue();
  PerfTimers["db"].stop();
#endif
  PerfTimers["output"].start();

  /* send queued output out to the operating system (ultimately to user) */
  for (d = DC::getInstance()->descriptor_list; d; d = next_d)
  {
    next_d = d->next;
    if ((FD_ISSET(d->descriptor, &output_set) && !d->output.empty()) || d->prompt_mode)
      if (process_output(d) < 0)
        close_socket(d);
    // else
    // d->prompt_mode = 1;
  }
  PerfTimers["output"].stop();
  // we're done with this pulse.  Now calculate the time until the next pulse and sleep until then
  // we want to pulse DC::PASSES_PER_SEC times a second (duh).  This is currently 4.

  gettimeofday(&now_time, nullptr);

  // temp removing this since it's spamming the crap out of us
  // else logf(110, LogChannels::LOG_BUG, "0 delay on pulse");
  gettimeofday(&last_time, nullptr);
  PerfTimers["gameloop"].stop();
}

void DC::game_loop_init(void)
{
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  FD_ZERO(&null_set);
  init_heartbeat();

  ssh.setup();

  gettimeofday(&last_time, nullptr);

  QTimer *gameLoopTimer = new QTimer(this);
  connect(gameLoopTimer, &QTimer::timeout, this, &DC::game_loop);
  gameLoopTimer->start(1000 / DC::PASSES_PER_SEC);

  // QTimer *sshLoopTimer = new QTimer(this);
  // connect(sshLoopTimer, &QTimer::timeout, &ssh, &SSH::SSH::poll);
  // sshLoopTimer->start();

  QHttpServer server(this);
  QStringList myData;
  auto dc = this;

  server.route("/myApi/<arg>", QHttpServerRequest::Method::Get,
               [&dc](int id, const QHttpServerRequest &request)
               {
                 return QHttpServerResponse(QString("int:%1\r\n").arg(id));
               });
  server.route("/myApi/<arg>/<arg>", QHttpServerRequest::Method::Get,
               [&dc](QString str, QString str2, const QHttpServerRequest &request)
               {
                 return QHttpServerResponse(QString("str:%1 str2:%2\r\n").arg(str).arg(str2));
               });

  server.route("/test", QHttpServerRequest::Method::Get, [&dc](const QHttpServerRequest &request)
               {
    if (!dc->authenticate(request))
    {
      return QHttpServerResponse(QString("Failed to authenticate.\r\n"));
    }

    auto future = QtConcurrent::run([]() {});

    if (future.isValid())
    {
      // QJsonArray array = QJsonArray::fromStringList(myData);

      return QHttpServerResponse(QString("Success.\r\n"));
    }
    else
    {
      return QHttpServerResponse("Failed.\r\n");
    } });

  server.route("/shutdown", QHttpServerRequest::Method::Get, [&dc](const QHttpServerRequest &request)
               {
                 if (!dc->authenticate(request, 110))
                 {
                   return QHttpServerResponse(QString("Failed to authenticate.\r\n"));
                 }

                 auto future = QtConcurrent::run([]() {});

                 int do_not_save_corpses = 1;

                 QString buf = QString("Hot reboot by %1.\r\n").arg("HTTP /shutdown/");
                 send_to_all(buf);
                 logentry(buf, ANGEL, LogChannels::LOG_GOD);
                 logentry("Writing sockets to file for hotboot recovery.", 0, LogChannels::LOG_MISC);

                 for (const auto &ch : dc->character_list)
                 {
                   if (ch->player && IS_PC(ch))
                   {
                     ch->save();
                   }
                 }

                 char **argv = nullptr;
                 if (!write_hotboot_file(argv))
                 {
                   logentry("Hotboot failed.  Closing all sockets.", 0, LogChannels::LOG_MISC);
                   return QHttpServerResponse("Failed.\r\n");
                 }

                 return QHttpServerResponse("Rebooting.\r\n"); });

  server.listen(QHostAddress::LocalHost, 6980);

  exec();

  ssh.close();
}

extern void pulse_hunts();
extern void auction_expire();
void init_heartbeat()
{
  pulse_mobile = DC::PULSE_MOBILE;
  pulse_timer = DC::PULSE_TIMER;
  pulse_bard = DC::PULSE_BARD;
  pulse_violence = DC::PULSE_VIOLENCE;
  pulse_tensec = DC::PULSE_TENSEC;
  pulse_weather = DC::PULSE_WEATHER;
  pulse_regen = DC::PULSE_REGEN;
  pulse_time = DC::PULSE_TIME;
  pulse_short = DC::PULSE_SHORT;
}

void heartbeat()
{
  DC *dc = DC::getInstance();
  if (--pulse_mobile < 1)
  {
    pulse_mobile = DC::PULSE_MOBILE;

    PerfTimers["mobile"].start();
    mobile_activity();
    PerfTimers["mobile"].stop();

    PerfTimers["object"].start();
    object_activity(DC::PULSE_MOBILE);
    PerfTimers["object"].stop();
  }
  if (--pulse_timer < 1)
  {
    pulse_timer = DC::PULSE_TIMER;
    check_timer();

    PerfTimers["affect"].start();
    affect_update(DC::PULSE_TIMER);
    PerfTimers["affect"].stop();
  }
  if (--pulse_short < 1)
  {
    pulse_short = DC::PULSE_SHORT;
    PerfTimers["short"].start();
    short_activity();
    PerfTimers["short"].stop();
    pulse_command_lag();
  }
  // TODO - need to eventually modify this so it works for casters too so I can delay certain
  if (--pulse_bard < 1)
  {
    pulse_bard = DC::PULSE_BARD;

    PerfTimers["bard"].start();
    update_bard_singing();
    PerfTimers["bard"].stop();

    PerfTimers["mprogthrows"].start();
    update_mprog_throws(); // convienant place to put it
    PerfTimers["mprogthrows"].stop();

    PerfTimers["camp"].start();
    update_make_camp_and_leadership(); // and this, too
    PerfTimers["camp"].stop();
  }

  if (--pulse_violence < 1)
  {
    pulse_violence = DC::PULSE_VIOLENCE;

    PerfTimers["violence"].start();
    perform_violence();
    update_characters();
    affect_update(DC::PULSE_VIOLENCE);
    check_silence_beacons();
    PerfTimers["violence"].stop();
  }

  if (--pulse_tensec < 1)
  {
    pulse_tensec = DC::PULSE_TENSEC;
    PerfTimers["consecrate"].start();
    checkConsecrate(DC::PULSE_TENSEC);
    PerfTimers["consecrate"].stop();
  }

  if (--pulse_weather < 1)
  {
    pulse_weather = DC::PULSE_WEATHER;
    PerfTimers["weather"].start();
    weather_update();
    PerfTimers["weather"].stop();

    PerfTimers["auctionexp"].start();
    auction_expire();
    PerfTimers["auctionexp"].stop();
  }

  if (--pulse_regen < 1)
  {
    PerfTimers["pulse_regen"].start();
    // random pulse timer for regen to make tick sleeping impossible
    pulse_regen = number((quint64)DC::PULSE_REGEN - 8 * DC::PASSES_PER_SEC, (quint64)DC::PULSE_REGEN + 5 * DC::PASSES_PER_SEC);
    point_update();
    pulse_takeover();
    affect_update(DC::PULSE_REGEN);
    checkConsecrate(DC::PULSE_REGEN);
    if (!number(0, 2))
    {
      dc->send_hint();
    }
    PerfTimers["pulse_regen"].stop();
  }

  if (--pulse_time < 1)
  {
    pulse_time = DC::PULSE_TIME;
    PerfTimers["pulse_time"].start();

    PerfTimers["zone_update"].start();
    zone_update();
    PerfTimers["zone_update"].stop();

    PerfTimers["time_update"].start();
    time_update();
    PerfTimers["time_update"].stop();

    PerfTimers["food_update"].start();
    food_update();
    PerfTimers["food_update"].stop();

    PerfTimers["affect_update"].start();
    affect_update(DC::PULSE_TIME);
    PerfTimers["affect_update"].stop();

    PerfTimers["update_corpses"].start();
    update_corpses_and_portals();
    PerfTimers["update_corpses"].stop();

    PerfTimers["check_idle"].start();
    check_idle_passwords();
    PerfTimers["check_idle"].stop();

    PerfTimers["quest_update"].start();
    quest_update();
    PerfTimers["quest_update"].stop();

    PerfTimers["leaderboard"].start();
    leaderboard.check(); // good place to put this
    PerfTimers["leaderboard"].stop();

    if (DC::getInstance()->cf.bport == false)
    {
      PerfTimers["check_champ"].start();
      check_champion_and_website_who_list();
      PerfTimers["check_champ"].stop();
    }

    PerfTimers["save_slot"].start();
    save_slot_machines();
    PerfTimers["save_slot"].stop();

    PerfTimers["pulse_hunts"].start();
    pulse_hunts();
    PerfTimers["pulse_hunts"].stop();

    PerfTimers["redo_shop"].start();
    if (!number(0, 47))
    {
      redo_shop_profit();
    }
    PerfTimers["redo_shop"].stop();

    PerfTimers["pulse_time"].stop();
  }
}

/* ******************************************************************
 *  general utility stuff (for local use)                            *
 ****************************************************************** */

/*
 * Turn off echoing (specific to telnet client)
 */
void telnet_echo_off(class Connection *d)
{
  char off_string[] =
      {
          (char)IAC,
          (char)WILL,
          (char)TELOPT_ECHO,
          (char)0,
      };

  SEND_TO_Q(off_string, d);
}

/*
 * Turn on echoing (specific to telnet client)
 */
void telnet_echo_on(class Connection *d)
{
  char on_string[] =
      {
          (char)IAC,
          (char)WONT,
          (char)TELOPT_ECHO,
          (char)TELOPT_NAOFFD,
          (char)TELOPT_NAOCRD,
          (char)0,
      };

  SEND_TO_Q(on_string, d);
}

void telnet_sga(Connection *d)
{
  const char suppress_go_ahead[] = {(char)IAC, (char)WILL, (char)TELOPT_SGA, (char)0};
  SEND_TO_Q(QByteArray(suppress_go_ahead), d);
}

void telnet_ga(Connection *d)
{
  const char go_ahead[] = {(char)IAC, (char)GA, (char)0};
  SEND_TO_Q(QByteArray(go_ahead), d);
}

int do_lastprompt(Character *ch, char *arg, int cmd)
{
  if (GET_LAST_PROMPT(ch))
    csendf(ch, "Last prompt: %s\n\r", GET_LAST_PROMPT(ch));
  else
    send_to_char("Last prompt: unset\n\r", ch);

  return eSUCCESS;
}

int do_prompt(Character *ch, char *arg, int cmd)
{
  while (*arg == ' ')
    arg++;

  if (IS_MOB(ch))
  {
    send_to_char("You're a mob!  You can't set your prompt.\r\n", ch);
    return eFAILURE;
  }

  if (!*arg)
  {
    send_to_char("Set your prompt to what? Try 'help prompt'.\r\n", ch);
    if (GET_PROMPT(ch))
    {
      send_to_char("Current prompt:  ", ch);
      send_to_char(GET_PROMPT(ch), ch);
      send_to_char("\n\r", ch);
      send_to_char("Last prompt: ", ch);
      if (GET_LAST_PROMPT(ch))
      {
        send_to_char(GET_LAST_PROMPT(ch), ch);
      }
      else
      {
        send_to_char("unset", ch);
      }
      send_to_char("\n\r", ch);
    }
    return eSUCCESS;
  }

  if (GET_LAST_PROMPT(ch))
    dc_free(GET_LAST_PROMPT(ch));

  if (GET_PROMPT(ch))
  {
    GET_LAST_PROMPT(ch) = str_dup(GET_PROMPT(ch));
    dc_free(GET_PROMPT(ch));
  }

  GET_PROMPT(ch) = str_dup(arg);
  send_to_char("Ok.\r\n", ch);
  return eSUCCESS;
}

char *calc_color_align(int align)
{
  if (align <= -351)
    return BOLD RED;
  if (align <= -300)
    return BOLD YELLOW;
  if (align <= 299)
    return BOLD GREY;
  if (align <= 349)
    return BOLD YELLOW;
  return BOLD GREEN;
}

char *calc_color(int hit, int max_hit)
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

  if (hit <= (max_hit / 3))
    return BOLD RED;

  if (hit <= (max_hit / 3) * 2)
    return BOLD YELLOW;

  return GREEN;
}

char *cond_txtz[] = {
    "excellent condition",
    "a few scratches",
    "slightly hurt",
    "fairly fucked up",
    "bleeding freely",
    "covered in blood",
    "near death",
    "dead as a doornail"};

char *cond_txtc[] = {
    BOLD GREEN "excellent condition" NTEXT,
    GREEN "a few scratches" NTEXT,
    BOLD YELLOW "slightly hurt" NTEXT,
    YELLOW "fairly fucked up" NTEXT,
    RED "bleeding freely" NTEXT,
    BOLD RED "covered in blood" NTEXT,
    BOLD GREY "near death" NTEXT,
    "dead as a doornail"};

char *cond_colorcodes[] = {
    BOLD GREEN,
    GREEN,
    BOLD YELLOW,
    YELLOW,
    RED,
    BOLD RED,
    BOLD GREY,
};

string calc_name(Character *ch, bool colour = false)
{
  int percent;
  string name;

  if (ch->getHP() == 0 || GET_MAX_HIT(ch) == 0)
    percent = 0;
  else
    percent = ch->getHP() * 100 / GET_MAX_HIT(ch);

  if (colour == true)
  {
    if (percent >= 100)
      name = cond_colorcodes[0];
    else if (percent >= 90)
      name = cond_colorcodes[1];
    else if (percent >= 75)
      name = cond_colorcodes[2];
    else if (percent >= 50)
      name = cond_colorcodes[3];
    else if (percent >= 30)
      name = cond_colorcodes[4];
    else if (percent >= 15)
      name = cond_colorcodes[5];
    else if (percent >= 0)
      name = cond_colorcodes[6];
  }

  if (IS_PC(ch))
    name += ch->name;
  else
    name += ch->short_desc;

  name += NTEXT;

  return name;
}

char *calc_condition(Character *ch, bool colour = false)
{
  int percent;
  char *cond_txt[8]; // = cond_txtz;

  if (colour)
    memcpy(cond_txt, cond_txtc, sizeof(cond_txtc));
  else
    memcpy(cond_txt, cond_txtz, sizeof(cond_txtz));

  if (ch->getHP() == 0 || GET_MAX_HIT(ch) == 0)
    percent = 0;
  else
    percent = ch->getHP() * 100 / GET_MAX_HIT(ch);

  if (percent >= 100)
    return cond_txt[0];
  else if (percent >= 90)
    return cond_txt[1];
  else if (percent >= 75)
    return cond_txt[2];
  else if (percent >= 50)
    return cond_txt[3];
  else if (percent >= 30)
    return cond_txt[4];
  else if (percent >= 15)
    return cond_txt[5];
  else if (percent >= 0)
    return cond_txt[6];
  else
    return cond_txt[7];
}

void make_prompt(class Connection *d, string &prompt)
{
  string buf = {};
  if (!d->character)
  {
    return;
  }
  if (d->showstr_count)
  {
    buf = fmt::format("\r\n[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number ({} {}) ]",
                      d->showstr_page, d->showstr_count);
    prompt += buf;
  }
  else if (d->strnew)
  {
    if (IS_PC(d->character) && DC::isSet(d->character->player->toggles, Player::PLR_EDITOR_WEB))
    {
      prompt += "Web Editor] ";
    }
    else
    {
      prompt += "*] ";
    }
  }
  else if (d->hashstr)
  {
    prompt += "] ";
  }
  else if (STATE(d) != Connection::states::PLAYING)
  {
    return;
  }
  else if (IS_MOB(d->character))
  {
    prompt += generate_prompt(d->character);
  }
  else
  {
    if (!DC::isSet(GET_TOGGLES(d->character), Player::PLR_COMPACT))
      prompt += "\n\r";
    if (!GET_PROMPT(d->character))
      prompt += "type 'help prompt'> ";
    else
      prompt += generate_prompt(d->character);
  }

  // As long as we're not in telnet character mode then send IAC GA
  if (d->character->getSetting("mode").startsWith("char") == false)
  {
    char go_ahead[] = {(char)IAC, (char)GA, (char)0};
    prompt += go_ahead;
  }
}

Character *get_charmie(Character *ch)
{
  if (!ch)
    return 0;

  struct follow_type *k;
  if (ch->followers)
    for (k = ch->followers; k && k != (follow_type *)0x95959595; k = k->next)
      if (IS_AFFECTED(k->follower, AFF_CHARM))
        return k->follower;

  return nullptr;
}

string generate_prompt(Character *ch)
{
  Character *charmie = nullptr;
  char *source = nullptr;
  char *pro = nullptr;
  char *prompt = nullptr;
  pro = prompt = new char[MAX_STRING_LENGTH];
  memset(pro, 0, sizeof(pro));
  char *mobprompt = "HP: %i/%H %f >";
  Room *rm = nullptr;
  try
  {
    rm = &DC::getInstance()->world[ch->in_room];
  }
  catch (...)
  {
    rm = nullptr;
  }

  if (IS_NPC(ch))
    source = mobprompt;
  else
    source = GET_PROMPT(ch);

  for (; *source != '\0';)
  {
    if (*source != '%')
    {
      *pro = *source;
      ++pro;
      ++source;
      *pro = '\0';
      continue;
    }
    ++source;
    if (*source == '\0')
    {
      return "1There is a fucked up code in your prompt> ";
    }

    switch (*source)
    {
    default:
      return "2There is a fucked up code in your prompt> ";
    case 'a':
      sprintf(pro, "%hd", GET_ALIGNMENT(ch));
      break;
    case 'A':
      sprintf(pro, "%s%hd%s", calc_color_align(GET_ALIGNMENT(ch)), GET_ALIGNMENT(ch), NTEXT);
      break;
    // %b
    case 'b':
      try
      {
        Character *peer = ch->getFollowers().at(0);
        if (peer != nullptr)
        {
          if (CAN_SEE(ch, peer))
          {
            sprintf(pro, "%s", GET_NAME(peer));
          }
          else
          {
            sprintf(pro, "someone");
          }
        }
      }
      catch (...)
      {
      }
      break;
    // %B - group member 1 hitpoints
    case 'B':
      try
      {
        Character *peer = ch->getFollowers().at(0);
        if (peer != nullptr)
        {
          sprintf(pro, "%s%d%s", calc_color(peer->getHP(), GET_MAX_HIT(peer)), peer->getHP(), NTEXT);
        }
      }
      catch (...)
      {
      }
      break;
    case 'c':
      if (ch->fighting)
        sprintf(pro, "<%s>", calc_condition(ch));
      /* added by pir to stop "prompt %c" crash bug */
      else
        sprintf(pro, " ");
      break;
    case 'C':
      if (ch->fighting)
        sprintf(pro, "<%s>", calc_condition(ch, true));
      /* added by pir to stop "prompt %c" crash bug */
      else
        sprintf(pro, " ");
      break;
    case 'd':
      sprintf(pro, "%s", time_look[weather_info.sunlight]);
      break;
    // %D - indicate weather conditions
    case 'D':
      if (OUTSIDE(ch))
        sprintf(pro, "%s", sky_look[weather_info.sky]);
      else
        sprintf(pro, "indoors");
      break;
    // %e
    case 'e':
      try
      {
        Character *peer = ch->getFollowers().at(1);
        if (peer != nullptr)
        {
          if (CAN_SEE(ch, peer))
          {
            sprintf(pro, "%s", GET_NAME(peer));
          }
          else
          {
            sprintf(pro, "someone");
          }
        }
      }
      catch (...)
      {
      }
      break;
    // %E
    case 'E':
      try
      {
        Character *peer = ch->getFollowers().at(1);
        if (peer != nullptr)
        {
          sprintf(pro, "%s%d%s", calc_color(peer->getHP(), GET_MAX_HIT(peer)), peer->getHP(), NTEXT);
        }
      }
      catch (...)
      {
      }
      break;
    case 'f':
      if (ch->fighting)
        sprintf(pro, "(%s)", calc_condition(ch->fighting));
      /* added by pir to stop "prompt %c" crash bug */
      else
        sprintf(pro, " ");
      break;
    case 'F':
      if (ch->fighting)
        sprintf(pro, "(%s)", calc_condition(ch->fighting, true));
      /* added by pir to stop "prompt %c" crash bug */
      else
        sprintf(pro, " ");
      break;
    case 'g':
      sprintf(pro, "%llu", ch->getGold());
      break;
    case 'G':
      sprintf(pro, "%u", (int32_t)(ch->getGold() / 20000));
      break;
    case 'h':
      sprintf(pro, "%d", ch->getHP());
      break;
    // %H
    case 'H':
      sprintf(pro, "%d", GET_MAX_HIT(ch));
      break;
    // %i - color max hitpoints
    case 'i':
      sprintf(pro, "%s%d%s", calc_color(ch->getHP(), GET_MAX_HIT(ch)),
              ch->getHP(), NTEXT);
      break;
    case 'I':
      sprintf(pro, "%d", ((ch->getHP() * 100) / GET_MAX_HIT(ch)));
      break;
    // j
    case 'j':
      try
      {
        Character *peer = ch->getFollowers().at(2);
        if (peer != nullptr)
        {
          if (CAN_SEE(ch, peer))
          {
            sprintf(pro, "%s", GET_NAME(peer));
          }
          else
          {
            sprintf(pro, "someone");
          }
        }
      }
      catch (...)
      {
      }
      break;
    case 'J':
      try
      {
        Character *peer = ch->getFollowers().at(2);
        if (peer != nullptr)
        {
          sprintf(pro, "%s%d%s", calc_color(peer->getHP(), GET_MAX_HIT(peer)), peer->getHP(), NTEXT);
        }
      }
      catch (...)
      {
      }
      break;
    // %k - indicate time of day
    case 'k':
      sprintf(pro, "%d", GET_KI(ch));
      break;
    case 'K':
      sprintf(pro, "%d", GET_MAX_KI(ch));
      break;
    // %l - color current ki
    case 'l':
      sprintf(pro, "%s%d%s", calc_color(GET_KI(ch), GET_MAX_KI(ch)),
              GET_KI(ch), NTEXT);
      break;
    case 'L':
      sprintf(pro, "%d", ((GET_KI(ch) * 100) / GET_MAX_KI(ch)));
      break;
    case 'm':
      sprintf(pro, "%d", GET_MANA(ch));
      break;
    case 'M':
      sprintf(pro, "%d", GET_MAX_MANA(ch));
      break;
    case 'n':
      sprintf(pro, "%s%d%s", calc_color(GET_MANA(ch), GET_MAX_MANA(ch)),
              GET_MANA(ch), NTEXT);
      break;
    case 'N':
      sprintf(pro, "%d", ((GET_MANA(ch) * 100) / GET_MAX_MANA(ch)));
      break;
    // o - unused
    // O - Last object edited
    case 'O':
      if (IS_PC(ch))
      {
        if (ch->player->last_obj_vnum > 0)
        {
          sprintf(pro, "%llu", ch->player->last_obj_vnum);
        }
      }
      break;
    case 'p':
      if (ch->fighting && ch->fighting->fighting)
      {
        sprintf(pro, "%s", calc_name(ch->fighting->fighting).c_str());
      }
      else
        sprintf(pro, " ");
      break;
    case 'P':
      if (ch->fighting && ch->fighting->fighting)
      {
        sprintf(pro, "%s", calc_name(ch->fighting->fighting, true).c_str());
      }
      else
        sprintf(pro, " ");
      break;
    case 'q':
      if (ch->fighting)
      {
        sprintf(pro, "%s", calc_name(ch->fighting).c_str());
      }
      else
        sprintf(pro, " ");
      break;
    case 'Q':
      if (ch->fighting)
      {
        sprintf(pro, "%s", calc_name(ch->fighting, true).c_str());
      }
      else
        sprintf(pro, " ");
      break;
    case 'r':
      sprintf(pro, "%c%c", '\n', '\r');
      break;
    case 'R':
      if (rm != nullptr)
      {
        sprintf(pro, "%s%d%s", GREEN, rm->number, NTEXT);
      }
      break;
    case 's':
      if (DC::getInstance()->rooms.contains(ch->in_room))
        sprintf(pro, "%s", sector_types[DC::getInstance()->world[ch->in_room].sector_type]);
      else
        sprintf(pro, " ");
      break;
    // S - Last mob edited
    case 'S':
      if (IS_PC(ch))
      {
        sprintf(pro, "%d", ch->player->last_mob_edit);
      }
      break;
    case 't':
      if (ch->fighting && ch->fighting->fighting)
        sprintf(pro, "[%s]",
                calc_condition(ch->fighting->fighting));
      /* added by pir to stop "prompt %c" crash bug */
      else
        sprintf(pro, " ");
      break;
    case 'T':
      if (ch->fighting && ch->fighting->fighting)
        sprintf(pro, "[%s]", calc_condition(ch->fighting->fighting, true));
      /* added by pir to stop "prompt %c" crash bug */
      else
        sprintf(pro, " ");
      break;
    case 'u':
      try
      {
        Character *peer = ch->getFollowers().at(3);
        if (peer != nullptr)
        {
          if (CAN_SEE(ch, peer))
          {
            sprintf(pro, "%s", GET_NAME(peer));
          }
          else
          {
            sprintf(pro, "someone");
          }
        }
      }
      catch (...)
      {
      }
      break;
    case 'U':
      try
      {
        Character *peer = ch->getFollowers().at(3);
        if (peer != nullptr)
        {
          sprintf(pro, "%s%d%s", calc_color(peer->getHP(), GET_MAX_HIT(peer)), peer->getHP(), NTEXT);
        }
      }
      catch (...)
      {
      }
      break;
    case 'v':
      sprintf(pro, "%d", GET_MOVE(ch));
      break;
    case 'V':
      sprintf(pro, "%d", GET_MAX_MOVE(ch));
      break;
    case 'W':
      sprintf(pro, "%d", ((GET_MOVE(ch) * 100) / GET_MAX_MOVE(ch)));
      break;
    case 'w':
      sprintf(pro, "%s%d%s", calc_color(GET_MOVE(ch), GET_MAX_MOVE(ch)),
              GET_MOVE(ch), NTEXT);
      break;
    case 'x':
      sprintf(pro, "%lld", GET_EXP(ch));
      break;
    case 'X':
      sprintf(pro, "%lld", (int64_t)(exp_table[(int)GET_LEVEL(ch) + 1] - (int64_t)GET_EXP(ch)));
      break;
    case 'y':
      charmie = get_charmie(ch);
      if (charmie != nullptr)
        sprintf(pro, "%d", (charmie->getHP() * 100) / GET_MAX_HIT(charmie));
      else
        sprintf(pro, " ");
      break;
    case 'Y':
      charmie = get_charmie(ch);
      if (charmie != nullptr)
        sprintf(pro, "%s%d%s", calc_color(charmie->getHP(), GET_MAX_HIT(charmie)),
                (charmie->getHP() * 100) / GET_MAX_HIT(charmie), NTEXT);
      else
        sprintf(pro, " ");
      break;
    // z - wizinvis level
    case 'z':
      if (IS_IMMORTAL(ch))
      {
        sprintf(pro, "%s%d%s", YELLOW, ch->player->wizinvis, NTEXT);
      }
      break;
    // Z - zone number
    case 'Z':
      if (rm != nullptr)
      {
        sprintf(pro, "%s%d%s", RED, rm->zone, NTEXT);
      }
      break;
    case '%':
      sprintf(pro, "%%");
      break;
    case '$':
      sprintf(pro, "%d", GET_PLATINUM(ch));
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
    }
    ++source;
    while (*pro != '\0')
      pro++;
  }
  *pro = ' ';
  *(pro + 1) = '\0';

  string buffer = prompt;
  delete[] prompt;
  return buffer;
}

void write_to_q(const string txt, queue<string> &input_queue)
{
#ifdef DEBUG_INPUT
  // cerr << "Writing to queue '" << txt << "'" << endl;
#endif
  input_queue.push(txt);
}

string get_from_q(queue<string> &input_queue)
{
  if (input_queue.empty())
  {
    return string();
  }

  string dest = input_queue.front();
  input_queue.pop();

  return dest;
}

/* Empty the queues before closing connection */
void flush_queues(class Connection *d)
{
  int dummy;
  string buf2 = {};

  while (!get_from_q(d->input).empty())
    ;
  if (!d->output.empty())
  {
    write_to_descriptor(d->descriptor, d->output);
  }
}

void free_buff_pool_from_memory()
{
  struct txt_block *curr = nullptr;

  while (bufpool)
  {
    curr = bufpool->next;
    dc_free(bufpool);
    bufpool = curr;
  }
}

void scramble_text(string &txt)
{
  for (auto &curr : txt)
  {
    // only scramble letters, but not 'm' cause 'm' is used in ansi codes
    if (number(1, 5) == 5 && ((curr >= 'a' && curr <= 'z') || (curr >= 'A' && curr <= 'Z')) && curr != 'm')
    {
      curr = number(0, 1) ? (char)number('a', 'z') : (char)number('A', 'Z');
    }
  }
}

QString scramble_text(QString input)
{
  QString output;

  for (auto &c : input)
  {
    // only scramble letters, but not 'm' cause 'm' is used in ansi codes
    if (number(1, 5) == 5 && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) && c != 'm')
    {
      c = number(0, 1) ? (char)number('a', 'z') : (char)number('A', 'Z');
    }
  }

  return output;
}

void write_to_output(const char *txt, class Connection *t)
{
  if (txt)
  {
    write_to_output(QByteArray(txt), t);
  }
}

void write_to_output(string txt, class Connection *t)
{
  if (!txt.empty())
  {
    write_to_output(QByteArray(txt.c_str()), t);
  }
}

void write_to_output(QByteArray txt, class Connection *t)
{
  /* if there's no descriptor, don't worry about output */
  if (t->descriptor == 0)
    return;

  if (t->allowColor && t->connected != Connection::states::EDITING && t->connected != Connection::states::WRITE_BOARD && t->connected != Connection::states::EDIT_MPROG)
  {
    txt = handle_ansi(txt, t->character);
  }
  if (t->character && IS_AFFECTED(t->character, AFF_INSANE) && t->connected == Connection::states::PLAYING)
  {
    txt = scramble_text(txt.toStdString().c_str()).toStdString().c_str();
  }
  t->output += txt.toStdString();
}

/* ******************************************************************
 *  socket handling                                                  *
 ****************************************************************** */

int new_descriptor(int s)
{
  socket_t desc = {};
  socklen_t i = {};
  static int last_desc = 0; /* last descriptor number */
  class Connection *newd = {};
  struct sockaddr_in peer = {};
  char buf[MAX_STRING_LENGTH] = {};

  /* accept the new connection */
  i = sizeof(peer);
  getsockname(s, (struct sockaddr *)&peer, &i);
  if ((desc = accept(s, (struct sockaddr *)&peer, &i)) < 0)
  {
    perror("accept");
    return -1;
  }

  // keep it from blocking
#ifndef WIN32
  if (fcntl(desc, F_SETFL, O_NONBLOCK) < 0)
  {
    perror("init_socket : fcntl : nonblock");
    exit(1);
  }
#else
  uint32_t nb = 1;
  if (ioctlsocket(desc, FIONBIO, &nb) < 0)
  {
    perror("init_socket : ioctl : nonblock");
    exit(1);
  }
#endif

  /* create a new descriptor */
  newd = new Connection;
  newd->setPeerAddress(QHostAddress(inet_ntoa(peer.sin_addr)));

  /* determine if the site is banned */
  if (isbanned(newd->getPeerOriginalAddress()) == BAN_ALL)
  {
    write_to_descriptor(desc,
                        "Your site has been banned from Dark Castle. If you have any\n\r"
                        "Questions, please email us at:\n\r"
                        "imps@dcastle.org\n\r");

    CLOSE_SOCKET(desc);
    sprintf(buf, "Connection attempt denied from [%s]", newd->getPeerOriginalAddress().toString().toStdString().c_str());
    logentry(buf, OVERSEER, LogChannels::LOG_SOCKET);
    dc_free(newd);
    return 0;
  }

  /* initialize descriptor data */
  newd->descriptor = desc;
  newd->idle_tics = 0;
  newd->idle_time = 0;
  newd->wait = 1;
  newd->output = {};
  newd->next = DC::getInstance()->descriptor_list;
  newd->login_time = time(0);
  newd->astr = 0;
  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  /* prepend to list */
  DC::getInstance()->descriptor_list = newd;

  newd->connected = Connection::states::PRE_DISPLAY_ENTRANCE;
  return 0;
}

int process_output(class Connection *t)
{
  string i = {};
  static int result;

  /* we may need this \r\n for later -- see below */
  i = "\r\n";

  /* now, append the 'real' output */
  i += t->output;

  if (t->character && t->connected == Connection::states::PLAYING)
    blackjack_prompt(t->character, i, t->character->player && !DC::isSet(t->character->player->toggles, Player::PLR_ASCII));
  make_prompt(t, i);

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (!t->prompt_mode)
  { /* && !t->connected) */
    result = write_to_descriptor(t->descriptor, i);
    t->prompt_mode = 0;
  }
  else
  {
    result = write_to_descriptor(t->descriptor, i.substr(2));
    t->prompt_mode = 0;
  }
  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by)
  {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }

  t->output = {};

  return result;
}

int write_to_descriptor(int desc, string txt)
{
  int total, bytes_written;

  total = txt.size();

  do
  {
#ifndef WIN32
    if ((bytes_written = write(desc, txt.c_str(), total)) < 0)
    {
#else
    if ((bytes_written = send(desc, txt, total, 0)) < 0)
    {
#endif
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
        errno = EAGAIN;
#endif /* EWOULDBLOCK */
      if (errno == EAGAIN)
      {
        // logentry("process_output: socket write would block",
        //     0, LogChannels::LOG_MISC);
      }
      else
      {
        perror("Write to socket");
        return (-1);
      }
      return (0);
    }
    else
    {
      txt += bytes_written;
      total -= bytes_written;
    }
  } while (total > 0);

  return 0;
}

enum telnet
{
  will_opt = '\xFB',
  wont_opt = '\xFC',
  do_opt = '\xFD',
  dont_opt = '\xFE',
  iac = '\xFF'
};

void process_iac(Connection *t)
{
  char prev = '\0';

  size_t iac_pos = t->inbuf.find(telnet::iac);
  if (iac_pos != t->inbuf.npos)
  {
    size_t processed = 0;
    auto iac_str = t->inbuf.substr(iac_pos);
    for (const auto &c : iac_str)
    {
      // count every processed character so we know how many to delete from t->inbuf
      processed++;

      // Find IAC
      if (prev == 0)
      {
        if (c == telnet::iac)
        {
          prev = c;
          continue;
        }
      }
      // Determine telnet option
      else if (prev == telnet::iac)
      {
        prev = c;
        switch (c)
        {
        case telnet::will_opt:
        case telnet::wont_opt:
        case telnet::do_opt:
        case telnet::dont_opt:
          continue;
          break;

        default:
          // cerr << "Unrecognized telnet option " << hex << static_cast<int>(c) << endl;
          prev = 0;
          break;
        }
      }
      else if (prev == telnet::do_opt)
      {
        if (c == '\x1')
        {
          // cerr << "Telnet client requests to turn on server-side echo" << endl;
          t->server_size_echo = true;
        }
        else if (c == '\x3')
        {
          // cerr << "Telnet client requests server to suppress sending go-ahead" << endl;
        }
        else
        {
          // cerr << "Unrecognized do option " << hex << static_cast<int>(c) << endl;
        }
        prev = 0;
      }
      else
      {
        // cerr << "Unrecognized telnet code " << hex << static_cast<int>(c) << endl;
        prev = 0;
      }
    }
    t->inbuf.erase(iac_pos, processed);
  }
}

string removeUnprintable(string input)
{
  size_t found_pos = input.npos;
  do
  {
    found_pos = input.find('\n');
    if (found_pos != input.npos)
    {
      input.erase(found_pos, 1);
    }
  } while (found_pos != input.npos);

  do
  {
    found_pos = input.find('\r');
    if (found_pos != input.npos)
    {
      input.erase(found_pos, 1);
    }
  } while (found_pos != input.npos);

  do
  {
    found_pos = input.find('\b');
    if (found_pos != input.npos)
    {
      input.erase(found_pos, 1);
    }
  } while (found_pos != input.npos);

  return input;
}

string makePrintable(string input)
{
  size_t found_pos = input.npos;
  do
  {
    found_pos = input.find('\n');
    if (found_pos != input.npos)
    {
      input.erase(found_pos, 1);
      input.insert(found_pos, "\\n");
    }
  } while (found_pos != input.npos);

  do
  {
    found_pos = input.find('\r');
    if (found_pos != input.npos)
    {
      input.erase(found_pos, 1);
      input.insert(found_pos, "\\r");
    }
  } while (found_pos != input.npos);

  do
  {
    found_pos = input.find('\b');
    if (found_pos != input.npos)
    {
      input.erase(found_pos, 1);
      input.insert(found_pos, "\\b");
    }
  } while (found_pos != input.npos);

  return input;
}

string remove_all_codes(string input)
{
  size_t pos = 0, found_pos = 0, skip = 0;
  while ((found_pos = input.find("$", pos)) != input.npos)
  {
    skip = 1;

    if (found_pos + 1 <= input.length())
    {
      try
      {
        input.replace(found_pos, 1, "$$");
        skip = 2;
      }
      catch (...)
      {
      }
    }
    pos = found_pos + skip;
  }

  return input;
}

string remove_non_color_codes(string input)
{
  string output = {};
  size_t pos = 0, found_pos = 0;

  try
  {
    while ((found_pos = input.find("$")) != input.npos)
    {
      if (found_pos + 1 == input.length())
      {
        output += input.substr(0, found_pos + 1);
        output += "$";
        input.erase(0, found_pos + 1);
        output += input;
        return output;
      }

      char code = input.at(found_pos + 1);
      switch (code)
      {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'I':
      case 'L':
      case '*':
      case 'R':
      case 'B':
        output += input.substr(0, found_pos + 2);
        input.erase(0, found_pos + 2);
        break;
      default:
        output += input.substr(0, found_pos + 1);
        output += "$";
        input.erase(0, found_pos + 1);
        break;
      }
    }
    output += input;
  }
  catch (...)
  {
  }

  return output;
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(class Connection *t)
{
  size_t eoc_pos = t->inbuf.npos;
  size_t erase = 0;
  ssize_t bytes_read = 0;
  t->idle_time = 0;

  do
  {
    char c_buffer[8193] = {};
    if ((bytes_read = read(t->descriptor, &c_buffer, sizeof(c_buffer) - 1)) < 0)
    {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
        errno = EAGAIN;
#endif /* EWOULDBLOCK */
      if (errno != EAGAIN)
      {
        perror("process_input: about to lose connection");
        return -1; /* some error condition was encountered on
                    * read */
      }
      else
        return 0; /* the read would have blocked: just means no
                   * data there but everything's okay */
    }
    else if (bytes_read == 0)
    {
      if (t->character != nullptr && GET_NAME(t->character) != nullptr)
      {
        logentry(QString("Connection broken by peer %1 playing %2.").arg(t->getPeerAddress().toString()).arg(GET_NAME(t->character)), IMPLEMENTER + 1, LogChannels::LOG_SOCKET);
      }
      else
      {
        logentry(QString("Connection broken by peer %1 not playing a character.").arg(t->getPeerAddress().toString()), IMPLEMENTER + 1, LogChannels::LOG_SOCKET);
      }

      return -1;
    }
    string buffer = c_buffer;
    t->inbuf += buffer;

    // Search for telnet control codes
    process_iac(t);
    if (t->server_size_echo)
    {
      string new_buffer;
      for (const auto &c : buffer)
      {
        if ((c >= ' ' && c <= '~'))
        {
          new_buffer += c;
        }
        else if (c == '\b' || c == '\x7F') // erase last character for backspace or delete
        {
          if (t->inbuf.size() == 1)
          {
            // // cerr << "Before: [" << t->inbuf << "]" << t->inbuf.size() << endl;
            t->inbuf.erase(t->inbuf.end() - 1, t->inbuf.end());
            // // cerr << "After: [" << t->inbuf << "]" << t->inbuf.size() << endl;
          }
          if (t->inbuf.size() >= 2)
          {
            // // cerr << "Before: [" << t->inbuf << "]" << t->inbuf.size() << endl;
            t->inbuf.erase(t->inbuf.end() - 2, t->inbuf.end());
            // // cerr << "After: [" << t->inbuf << "]" << t->inbuf.size() << endl;
            new_buffer += "\b \b";
          }
        }
        else if (c == '\r')
        {
          new_buffer += "\r\n";
        }
        write_to_descriptor(t->descriptor, new_buffer);
      }
    }
    // Keep looping until client sends us a \n or \r
    // if read() above has nothing then we return
  } while (t->inbuf.find('\n') == t->inbuf.npos && t->inbuf.find('\r') == t->inbuf.npos);

  do
  {
    eoc_pos = t->inbuf.npos;
    erase = 0;
    size_t crnl_pos = t->inbuf.find("\r\n");
    if (crnl_pos != t->inbuf.npos)
    {
      eoc_pos = crnl_pos;
      erase = 2;
    }

    // search for carriage return, new line or pipe representing end of command
    size_t cr_pos = t->inbuf.find('\r');
    if (cr_pos != t->inbuf.npos && cr_pos < eoc_pos)
    {
      eoc_pos = cr_pos;
      erase = 1;
    }

    size_t nl_pos = t->inbuf.find('\n');
    if (nl_pos != t->inbuf.npos && nl_pos <= eoc_pos)
    {
      eoc_pos = nl_pos;
      erase = 1;
    }

#ifdef DEBUG_INPUT
    // cerr << "old t->inbuf [" << makePrintable(t->inbuf) << "]"
    << "(" << t->inbuf.length() << ")" << endl;
#endif
    string buffer = t->inbuf.substr(0, eoc_pos);
    t->inbuf.erase(0, eoc_pos + erase);
#ifdef DEBUG_INPUT
    // cerr << "new t->inbuf [" << makePrintable(t->inbuf) << "]"
    << "(" << t->inbuf.length() << ")" << endl;
    // cerr << "buffer [" << makePrintable(buffer) << "]"
    << "(" << buffer.length() << ")" << endl;
#endif

    if (t->character == nullptr || GET_LEVEL(t->character) < IMMORTAL)
    {
      buffer = remove_all_codes(buffer);
    }

    // Only search for pipe (|) when not editing
    if (t->connected != Connection::states::WRITE_BOARD && t->connected != Connection::states::EDITING && t->connected != Connection::states::EDIT_MPROG)
    {
      size_t pipe_pos = 0;
      do
      {
        pipe_pos = buffer.find('|');
        if (pipe_pos != buffer.npos)
        {
          string new_buffer = buffer.substr(0, pipe_pos);
          write_to_q(new_buffer, t->input);

          buffer.erase(0, pipe_pos + 1);
        }
        else
        {
          write_to_q(buffer, t->input);
        }
      } while (pipe_pos != buffer.npos);
    }
    else
    {
      write_to_q(buffer, t->input);
    }
  } while (t->inbuf.find('\n') != t->inbuf.npos || t->inbuf.find('\r') != t->inbuf.npos);

  /*
          else
          {
            // gods can use $codes but only ones for color UNLESS inside a MOBProg editor
            // I have to let them use $codes inside the editor or they can't write MOBProgs
            // tmp_ptr is just so I don't have to put ptr+1 7 times....
            tmp_ptr = (ptr + 1);
            if (isdigit(*tmp_ptr) || *tmp_ptr == 'I' || *tmp_ptr == 'L' ||
                *tmp_ptr == '*' || *tmp_ptr == 'R' ||
                *tmp_ptr == 'B' || t->connected == Connection::states::EDIT_MPROG ||
                t->connected == Connection::states::EDITING)
            { // write it like normal
              *write_point++ = *ptr;
              space_left--;
            }
            else if (space_left > 2)
            { // any other code, double up the $
              *write_point++ = *ptr;
              *write_point++ = '$';
              space_left -= 2;
            }
            else
              space_left = 0; // if no space left, so it truncates properly
                              // do nothing, which junks the $
          }
        }
        else if (isascii(*ptr) && isprint(*ptr))
        {
          *write_point++ = *ptr;
          space_left--;
        }
      }

      *write_point = '\0';

      if ((space_left <= 0) && (ptr < nl_pos))
      {
        string buffer;

        buffer = fmt::format("Line too long.  Truncated to:\r\n{}\r\n", tmp);
        if (write_to_descriptor(t->descriptor, buffer) < 0)
          return -1;
      }
      if (t->snoop_by)
      {
        SEND_TO_Q("% ", t->snoop_by);
        SEND_TO_Q(tmp, t->snoop_by);
        SEND_TO_Q("\r\n", t->snoop_by);
      }
      failed_subst = 0;

      if (!tmp.empty() && tmp[0] == '!')
      {
        tmp = t->last_input;
      }

      else if (!tmp.empty() && tmp[0] == '^')
      {
        if (!(failed_subst = perform_subst(t, t->last_input.data(), tmp.data())))
          t->last_input = tmp;
      }
      else
        t->last_input = tmp;

      if (!failed_subst)
        write_to_q(tmp, t->input, 0);

      /* find the end of this line */
  /*
    while (ISNEWL(*nl_pos) || (t->connected != Connection::states::WRITE_BOARD && t->connected != Connection::states::EDITING && t->connected != Connection::states::EDIT_MPROG && *nl_pos == '|'))
      nl_pos++;
*/
  /* see if there's another newline in the input buffer */
  /*
    read_point = ptr = nl_pos;
    for (nl_pos = nullptr; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr) || (t->connected != Connection::states::WRITE_BOARD && t->connected != Connection::states::EDITING && t->connected != Connection::states::EDIT_MPROG && *ptr == '|'))
        nl_pos = ptr;

  }
*/
  /* now move the rest of the buffer up to the beginning for the next pass */
  /*
      write_point = t->inbuf.data();
    while (*read_point)
      *(write_point++) = *(read_point++);
    *write_point = '\0';
  */

  return 1;
}

/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
int perform_subst(class Connection *t, char *orig, char *subst)
{
  char new_subst[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^')))
  {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first)))
  {
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
int close_socket(class Connection *d)
{
  char buf[128], idiotbuf[128];
  class Connection *temp;
  // int32_t target_idnum = -1;
  if (!d)
    return 0;
  flush_queues(d);
  CLOSE_SOCKET(d->descriptor);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = nullptr;

  if (d->snoop_by)
  {
    SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
    d->snoop_by->snooping = nullptr;
  }
  if (d->hashstr)
  {
    strcpy(idiotbuf, "\n\r~\n\r");
    strcat(idiotbuf, "\0");
    string_hash_add(d, idiotbuf);
  }
  if (d->strnew && (IS_MOB(d->character) || !DC::isSet(d->character->player->toggles, Player::PLR_EDITOR_WEB)))
  {
    strcpy(idiotbuf, "/s\n\r");
    strcat(idiotbuf, "\0");
    new_string_add(d, idiotbuf);
  }
  if (d->character)
  {
    // target_idnum = GET_IDNUM(d->character);
    if (d->connected == Connection::states::PLAYING || d->connected == Connection::states::WRITE_BOARD ||
        d->connected == Connection::states::EDITING || d->connected == Connection::states::EDIT_MPROG)
    {
      save_char_obj(d->character);
      // clan area stuff
      extern void check_quitter(Character * ch);
      check_quitter(d->character);

      // end any performances
      if (IS_SINGING(d->character))
        do_sing(d->character, "stop", CMD_DEFAULT);

      act("$n has lost $s link.", d->character, 0, 0, TO_ROOM, 0);
      sprintf(buf, "Closing link to: %s at %d.", GET_NAME(d->character),
              DC::getInstance()->world[d->character->in_room].number);
      if (IS_AFFECTED(d->character, AFF_CANTQUIT))
        sprintf(buf, "%s with CQ.", buf);
      logentry(buf, GET_LEVEL(d->character) > SERAPH ? GET_LEVEL(d->character) : SERAPH, LogChannels::LOG_SOCKET);
      d->character->desc = nullptr;
    }
    else
    {
      sprintf(buf, "Losing player: %s.",
              GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
      logentry(buf, 111, LogChannels::LOG_SOCKET);
      if (d->connected == Connection::states::WRITE_BOARD || d->connected == Connection::states::EDITING || d->connected == Connection::states::EDIT_MPROG)
      {
        //		sprintf(buf, "Suspicious: %s.",
        //			GET_NAME(d->character));
        //		logentry(buf, 110, LOG_HMM);
      }
      free_char(d->character, Trace("close_socket"));
    }
  }
  //   Removed this log caues it's so fricken annoying
  //   else
  //    logentry("Losing descriptor without char.", ANGEL, LogChannels::LOG_SOCKET);

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = nullptr;

  // if we're closing the socket that is next to be processed, we want to
  // go ahead and move on to the next one
  if (d == next_d)
    next_d = d->next;

  REMOVE_FROM_LIST(d, DC::getInstance()->descriptor_list, next);

  if (d->showstr_head)
    dc_free(d->showstr_head);
  if (d->showstr_count)
    dc_free(d->showstr_vector);

  delete d;
  d = nullptr;

  /*  if(DC::getInstance()->descriptor_list == nullptr)
  {
    // if there is NOONE on (everyone got disconnected) loop through and
    // boot all of the linkdeads.  That way if the mud's link is cut, the
    // first person back on can't RK everyone
    Character * next_i;
    for(Character * i = character_list; i; i = next_i) {
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
  class Connection *d, *next_d;

  for (d = DC::getInstance()->descriptor_list; d; d = next_d)
  {
    next_d = d->next;
    if (STATE(d) != Connection::states::GET_OLD_PASSWORD && STATE(d) != Connection::states::GET_NAME)
      continue;
    if (!d->idle_tics)
    {
      d->idle_tics++;
      continue;
    }
    else if (d->idle_tics > 5)
    {
      telnet_echo_on(d);
      SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
      STATE(d) = Connection::states::CLOSE;
    }
  }
}

/* ******************************************************************
 *  signal-handling functions (formerly signals.c)                   *
 ****************************************************************** */

void checkpointing(int sig)
{
  if (!tics)
  {
    logentry("SYSERR: CHECKPOINT shutdown: tics not updated", ANGEL, LogChannels::LOG_BUG);
    abort();
  }
  else
    tics = 0;
}

void report_debug_logging()
{
  extern int last_char_room;

  logentry("Last cmd:", ANGEL, LogChannels::LOG_BUG);
  logentry(QString::fromStdString(last_processed_cmd), ANGEL, LogChannels::LOG_BUG);
  logentry("Owner's Name:", ANGEL, LogChannels::LOG_BUG);
  logentry(QString::fromStdString(last_char_name), ANGEL, LogChannels::LOG_BUG);
  logf(ANGEL, LogChannels::LOG_BUG, "Last room: %d", last_char_room);
}

void crash_hotboot()
{
  class Connection *d = nullptr;
  extern int try_to_hotboot_on_crash;
  extern int died_from_sigsegv;

  // This can be dangerous, because if we had a SIGSEGV due to a descriptor being
  // invalid, we're going to do it again.  That's why we put in extern int died_from_sigsegv
  // sigsegv = # of times we've crashed from SIGSEGV

  for (d = DC::getInstance()->descriptor_list; d && died_from_sigsegv < 2; d = d->next)
  {
    write_to_descriptor(d->descriptor, "Mud crash detected.\r\n");
  }

  // attempt to hotboot
  if (try_to_hotboot_on_crash)
  {
    for (d = DC::getInstance()->descriptor_list; d && died_from_sigsegv < 2; d = d->next)
    {
      write_to_descriptor(d->descriptor, "Attempting to recover with a hotboot.\r\n");
    }
    logentry("Attempting to hotboot from the crash.", ANGEL, LogChannels::LOG_BUG);
    write_hotboot_file(0);
    // we shouldn't return from there unless we failed
    logentry("Hotboot crash recovery failed.  Exiting.", ANGEL, LogChannels::LOG_BUG);
    for (d = DC::getInstance()->descriptor_list; d && died_from_sigsegv < 2; d = d->next)
    {
      write_to_descriptor(d->descriptor, "Hotboot failed giving up.\r\n");
    }
  }

  for (d = DC::getInstance()->descriptor_list; d && died_from_sigsegv < 2; d = d->next)
  {
    write_to_descriptor(d->descriptor, "Giving up, goodbye.\r\n");
  }
}

void crashill(int sig)
{
  report_debug_logging();
  logentry("Recieved SIGFPE (Illegal Instruction)", ANGEL, LogChannels::LOG_BUG);
  crash_hotboot();
  logentry("Mud exiting from SIGFPE.", ANGEL, LogChannels::LOG_BUG);
  exit(0);
}

void crashfpe(int sig)
{
  report_debug_logging();
  logentry("Recieved SIGFPE (Arithmetic Error)", ANGEL, LogChannels::LOG_BUG);
  crash_hotboot();
  logentry("Mud exiting from SIGFPE.", ANGEL, LogChannels::LOG_BUG);
  exit(0);
}

void crashsig(int sig)
{
  extern int died_from_sigsegv;
  died_from_sigsegv++;
  if (died_from_sigsegv > 3)
  { // panic! error is in log...lovely  just give up
    exit(0);
  }
  if (died_from_sigsegv > 2)
  { // panic! try to log and get out
    logentry("Hit 'died_from_sigsegv > 2'", ANGEL, LogChannels::LOG_BUG);
    exit(0);
  }
  report_debug_logging();
  logentry("Recieved SIGSEGV (Segmentation fault)", ANGEL, LogChannels::LOG_BUG);
  crash_hotboot();
  logentry("Mud exiting from SIGSEGV.", ANGEL, LogChannels::LOG_BUG);
  exit(0);
}

void unrestrict_game(int sig)
{
  extern struct ban_list_element *ban_list;
  extern int num_invalid;

  logentry("Received SIGUSR2 - completely unrestricting game (emergent)",
           ANGEL, LogChannels::LOG_GOD);
  ban_list = nullptr;
  restrict = 0;
  num_invalid = 0;
}

void hupsig(int sig)
{
  logentry("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...", 0, LogChannels::LOG_MISC);
  abort(); /* perhaps something more elegant should
            * substituted */
}

void sigusr1(int sig)
{
  do_not_save_corpses = 1;
  logentry("Writing sockets to file for hotboot recovery.", 0, LogChannels::LOG_MISC);
  if (!write_hotboot_file(nullptr))
  {
    logentry("Hotboot failed.  Closing all sockets.", 0, LogChannels::LOG_MISC);
  }
}

#ifndef WIN32
void sigchld(int sig)
{
  struct rusage ru;
  wait3(nullptr, WNOHANG, &ru);
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

void signal_handler(int signal, siginfo_t *si, void *)
{
  logf(IMMORTAL, LogChannels::LOG_BUG, "signal_handler: signo=%d errno=%d code=%d "
                                       "pid=%d uid=%d status=%d utime=%lu stime=%lu value=%d "
                                       "int=%d ptr=%p overrun=%d timerid=%d addr=%p band=%ld "
                                       "fd=%d",
       si->si_signo, si->si_errno, si->si_code,
       si->si_pid, si->si_uid, si->si_status, si->si_utime, si->si_stime, si->si_value.sival_int,
       si->si_int, si->si_ptr, si->si_overrun, si->si_timerid, si->si_addr, si->si_band,
       si->si_fd);

  if (signal == SIGINT || signal == SIGTERM)
  {
    abort();
  }
  if (signal == SIGHUP)
  {
    char **new_argv = nullptr;
    string buf = "Hot reboot by SIGHUP.\r\n";
    extern int do_not_save_corpses;
    do_not_save_corpses = 1;
    send_to_all(buf.data());
    logentry(buf.c_str(), ANGEL, LogChannels::LOG_GOD);
    logentry("Writing sockets to file for hotboot recovery.", 0, LogChannels::LOG_MISC);
    if (!write_hotboot_file(new_argv))
    {
      logentry("Hotboot failed.  Closing all sockets.", 0, LogChannels::LOG_MISC);
    }
  }
}

void signal_setup(void)
{
  sigset_t set;
  sigfillset(&set);
  sigprocmask(SIG_UNBLOCK, &set, nullptr);

  // Hot reboots the game
  my_signal(SIGUSR1, sigusr1);

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
  struct sigaction sahup;
  memset(&sahup, 0, sizeof(sahup));
  sahup.sa_sigaction = signal_handler;
  sigemptyset(&sahup.sa_mask);
  sahup.sa_flags = SA_SIGINFO;
  sahup.sa_restorer = nullptr;
  sigaction(SIGHUP, &sahup, nullptr);

  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
  signal(SIGCHLD, sigchld); // hopefully kill zombies

  // my_signal(SIGSEGV, crashsig);  // catch null->blah
  my_signal(SIGFPE, crashfpe); // catch x / 0
  my_signal(SIGILL, crashill); // catch illegal instruction
}

/* ****************************************************************
 *       Public routines for system-to-player-communication        *
 **************************************************************** */

void send_to_char_regardless(string messg, Character *ch)
{
  if (ch->desc && !messg.empty())
  {
    SEND_TO_Q(messg, ch->desc);
  }
}

void send_to_char_nosp(const char *messg, Character *ch)
{
  char *tmp = str_nospace(messg);
  send_to_char(tmp, ch);
  dc_free(tmp);
}

void send_to_char_nosp(QString messg, Character *ch)
{
  send_to_char_nosp(messg.toStdString().c_str(), ch);
}

void record_msg(string messg, Character *ch)
{
  if (messg.empty() || IS_NPC(ch) || GET_LEVEL(ch) < IMMORTAL)
    return;

  if (ch->player->away_msgs == 0)
  {
    ch->player->away_msgs = new std::queue<string>();
  }

  if (ch->player->away_msgs->size() < 1000)
  {
    ch->player->away_msgs->push(messg);
  }
}

int do_awaymsgs(Character *ch, char *argument, int cmd)
{
  int lines = 0;
  string tmp;

  if (IS_NPC(ch))
    return eFAILURE;

  if ((ch->player->away_msgs == 0) || ch->player->away_msgs->empty())
  {
    SEND_TO_Q("No messages have been recorded.\r\n", ch->desc);
    return eSUCCESS;
  }

  // Show 23 lines of text, then stop
  while (!ch->player->away_msgs->empty())
  {
    tmp = ch->player->away_msgs->front();
    SEND_TO_Q(tmp, ch->desc);
    ch->player->away_msgs->pop();

    if (++lines == 23)
    {
      SEND_TO_Q("\n\rMore msgs available. Type awaymsgs to see them\n\r",
                ch->desc);
      break;
    }
  }

  return eSUCCESS;
}

void check_for_awaymsgs(Character *ch)
{
  if (!ch)
    return;

  if (IS_NPC(ch))
    return;

  if ((ch->player->away_msgs == 0) || ch->player->away_msgs->empty())
  {
    return;
  }

  send_to_char("You have unviewed away messages. ", ch);
  send_to_char("Type awaymsgs to view them.\r\n", ch);
}

void send_to_char(const char *mesg, Character *ch)
{
  send_to_char(string(mesg), ch);
}

void send_to_char(QString messg, Character *ch)
{
  send_to_char(messg.toStdString(), ch);
}

void send_to_char(string messg, Character *ch)
{
  if (IS_NPC(ch) && !ch->desc && MOBtrigger && !messg.empty())
    mprog_act_trigger(messg, ch, 0, 0, 0);
  if (IS_NPC(ch) && !ch->desc && !selfpurge && MOBtrigger && !messg.empty())
    oprog_act_trigger(messg.c_str(), ch);

  if (!selfpurge && (ch->desc && !messg.empty()) && (!is_busy(ch)))
  {
    SEND_TO_Q(messg, ch->desc);
  }
}

void DC::sendAll(QString message)
{
  if (message.isEmpty())
  {
    return;
  }

  for (auto i = descriptor_list; i; i = i->next)
  {
    if (i->connected == Connection::states::PLAYING)
    {
      i->send(message);
    }
  }
}

void send_to_all(QString message)
{
  class Connection *i;

  if (!message.isEmpty())
  {
    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    {
      if (!i->connected)
      {
        SEND_TO_Q(message.toStdString().c_str(), i);
      }
    }
  }
}

void ansi_color(char *txt, Character *ch)
{
  // mobs don't have toggles, so they automatically get ansi on
  if (txt != nullptr && ch->desc != nullptr)
  {
    if (!IS_MOB(ch) &&
        !DC::isSet(GET_TOGGLES(ch), Player::PLR_ANSI) &&
        !DC::isSet(GET_TOGGLES(ch), Player::PLR_VT100))
      return;
    else if (!IS_MOB(ch) &&
             DC::isSet(GET_TOGGLES(ch), Player::PLR_VT100) &&
             !DC::isSet(GET_TOGGLES(ch), Player::PLR_ANSI))
    {
      if ((!strcmp(txt, GREEN)) || (!strcmp(txt, RED)) || (!strcmp(txt, BLUE)) || (!strcmp(txt, BLACK)) || (!strcmp(txt, CYAN)) || (!strcmp(txt, GREY)) || (!strcmp(txt, EEEE)) || (!strcmp(txt, YELLOW)) || (!strcmp(txt, PURPLE)))
        return;
    }
    send_to_char(txt, ch);
    return;
  }
}

void send_info(QString messg)
{
  send_info(messg.toStdString().c_str());
}

void send_info(string messg)
{
  send_info(messg.c_str());
}

void send_info(const char *messg)
{
  class Connection *i;

  if (messg)
    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    {
      if (!(i->character) ||
          !DC::isSet(i->character->misc, LogChannels::CHANNEL_INFO))
        continue;
      if ((!i->connected) && !is_busy(i->character))
        SEND_TO_Q(messg, i);
    }
}

void send_to_outdoor(char *messg)
{
  class Connection *i;

  if (messg)
    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
      if (!i->connected)
        if (OUTSIDE(i->character) && !is_busy(i->character))
          SEND_TO_Q(messg, i);
}

void send_to_zone(char *messg, int zone)
{
  class Connection *i = nullptr;
  if (messg)
  {
    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    {
      if (!i->connected && !is_busy(i->character) && i->character->in_room != DC::NOWHERE && DC::getInstance()->world[i->character->in_room].zone == zone)
      {
        SEND_TO_Q(messg, i);
      }
    }
  }
}

void send_to_room(string messg, int room, bool awakeonly, Character *nta)
{
  Character *i = nullptr;

  // If a megaphone goes off when in someone's inventory this happens
  if (room == DC::NOWHERE)
    return;

  if (!DC::getInstance()->rooms.contains(room) || !DC::getInstance()->world[room].people)
  {
    return;
  }
  if (!messg.empty())
    for (i = DC::getInstance()->world[room].people; i; i = i->next_in_room)
      if (i->desc && !is_busy(i) && nta != i)
        if (!awakeonly || GET_POS(i) > POSITION_SLEEPING)
          SEND_TO_Q(messg, i->desc);
}

int is_busy(Character *ch)
{
  if (ch->desc &&
      ((ch->desc->connected == Connection::states::WRITE_BOARD) ||
       (ch->desc->connected == Connection::states::SEND_MAIL) ||
       (ch->desc->connected == Connection::states::EDITING) ||
       (ch->desc->connected == Connection::states::EDIT_MPROG)))
    return 1;

  return (0);
}

string perform_alias(class Connection *d, string orig)
{
  string first_arg, remainder, new_buf;
  // ptr = any_one_arg(orig, first_arg);
  tie(first_arg, remainder) = half_chop(orig);
  struct char_player_alias *x = nullptr;
  int lengthpre;
  int lengthpost;

  if (first_arg.empty())
  {
    return orig;
  }
  if (IS_MOB(d->character) || !d->character->player->alias)
    return orig;

  for (x = d->character->player->alias; x; x = x->next)
  {
    if (x->keyword)
    {
      if (x->keyword == first_arg)
      {
        new_buf = string(x->command);
        new_buf += " " + remainder;
        return new_buf;
      }
    }
  }

  return orig;
}

void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++)
    ;
}

char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument))
  {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';
  return argument;
}

bool is_multi(Character *ch)
{
  for (Connection *d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    if (d->character != nullptr && strcmp(GET_NAME(ch), GET_NAME(d->character)) && d->getPeerOriginalAddress() == ch->desc->getPeerOriginalAddress())
    {
      return true;
    }
  }

  return false;
}

void warn_if_duplicate_ip(Character *ch)
{
  char buf[256];
  int highlev = 51;

  list<multiplayer> multi_list;

  for (Connection *d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    if (d->character &&
        strcmp(GET_NAME(ch), GET_NAME(d->character)) &&
        !strcmp(d->getPeerOriginalAddress().toString().toStdString().c_str(), ch->desc->getPeerOriginalAddress().toString().toStdString().c_str()))
    {
      multiplayer m;
      m.host = d->getPeerAddress();
      m.name1 = GET_NAME(ch);
      m.name2 = GET_NAME(d->character);

      multi_list.push_back(m);

      highlev = MAX(GET_LEVEL(d->character), GET_LEVEL(ch));
      highlev = MAX(highlev, OVERSEER);

      // Mark both characters as multi-playing until they log out
      // This will be used elsewhere to enable automatic logging
      if (ch->player)
      {
        ch->player->multi = true;
      }

      if (d->character->player)
      {
        d->character->player->multi = true;
      }
    }
  }

  for (list<multiplayer>::iterator i = multi_list.begin(); i != multi_list.end(); ++i)
  {
    logf(108, LogChannels::LOG_WARNINGS, "MultipleIP: %s -> %s / %s ", (*i).host.toString().toStdString().c_str(), (*i).name1, (*i).name2);
  }
}

int do_editor(Character *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (argument == 0)
    return eFAILURE;

  if (IS_MOB(ch))
    return eFAILURE;

  csendf(ch, "Current editor: %s\n\r\n\r", DC::isSet(ch->player->toggles, Player::PLR_EDITOR_WEB) ? "web" : "game");

  one_argument(argument, arg1);

  if (*arg1)
  {
    if (!strcmp(arg1, "web"))
    {
      SET_BIT(ch->player->toggles, Player::PLR_EDITOR_WEB);
      send_to_char("Changing to web editor.\r\n", ch);
      send_to_char("Ok.\r\n", ch);
      return eSUCCESS;
    }
    else if (!strcmp(arg1, "game"))
    {
      REMOVE_BIT(ch->player->toggles, Player::PLR_EDITOR_WEB);
      send_to_char("Changing to in game line editor.\r\n", ch);
      send_to_char("Ok.\r\n", ch);
      return eSUCCESS;
    }
  }

  send_to_char("Usage: editor <type>\n\r", ch);
  send_to_char("Where type can be:\n\r", ch);
  send_to_char("web    - use online web editor\n\r", ch);
  send_to_char("game   - use in game line editor\n\r", ch);

  return eSUCCESS;
}

Proxy::Proxy(QString h)
    : header(h)
{
  QStringList elements = h.split(' ');

  if (elements.size() >= 2 && elements.at(0).indexOf("PROXY") == 0)
  {
    QString arg2 = elements.at(1);
    if (arg2 == "TCP4")
    {
      inet_protocol_family = inet_protocol_family_t::TCP4;
    }
    else if (arg2 == "TCP6")
    {
      inet_protocol_family = inet_protocol_family_t::TCP6;
    }
    else if (arg2 == "UNKNOWN")
    {
      inet_protocol_family = inet_protocol_family_t::UNKNOWN;
    }
    else
    {
      inet_protocol_family = inet_protocol_family_t::UNRECOGNIZED;
      logf(IMMORTAL, LogChannels::LOG_BUG, QString("Unrecognized PROXY inet protocol family in arg2 [%1]").arg(arg2).toStdString().c_str());
    }
  }

  if (elements.size() >= 3)
  {
    source_address = QHostAddress(elements.at(2));
  }

  if (elements.size() >= 4)
  {
    destination_address = QHostAddress(elements.at(3));
  }

  if (elements.size() >= 5)
  {
    QString arg5 = elements.at(4);

    bool ok = false;
    source_port = arg5.toUInt(&ok);
    if (!ok)
    {
      logf(IMMORTAL, LogChannels::LOG_BUG, QString("Invalid source port [%1]").arg(arg5).toStdString().c_str());
      return;
    }
  }

  if (elements.size() >= 6)
  {
    QString arg6 = elements.at(4);

    bool ok = false;
    destination_port = arg6.toUInt(&ok);
    if (!ok)
    {
      logf(IMMORTAL, LogChannels::LOG_BUG, QString("Invalid source port [%1]").arg(arg6).toStdString().c_str());
      return;
    }

    active = true;
  }
}

const char *Connection::getPeerOriginalAddressC(void)
{
  return getPeerOriginalAddress().toString().toStdString().c_str();
}