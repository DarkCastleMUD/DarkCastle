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
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <ctime>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <cctype>
#include <sys/time.h>
#include <tracy/Tracy.hpp>

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
#include <QtNetwork>

#include "DC/terminal.h"
#include "DC/fileinfo.h"
#include "DC/act.h"
#include "DC/player.h"
#include "DC/room.h"
#include "DC/structs.h"
#include "DC/utility.h"
#include "DC/connect.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/comm.h"
#include "DC/returnvals.h"
#include "DC/quest.h"
#include "DC/shop.h"
#include "DC/Leaderboard.h"
#include "DC/Timer.h"
#ifdef USE_SQL
#include "Backend/Database.h"
#endif
#include "DC/Timer.h"
#include "DC/obj.h"
#include "DC/DC.h"
#include "DC/CommandStack.h"
#include "DC/SSH.h"
#include "DC/character.h"

struct multiplayer
{
  QHostAddress host;
  QString name1;
  QString name2;
};

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
extern char *sky_look[];

void check_champion_and_website_who_list(void);
void save_slot_machines(void);
void check_silence_beacons(void);

/* local globals */
struct txt_block *bufpool = 0; /* pool of large output buffers */
int buf_largecount = 0;        /* # of large buffers which exist */
int buf_overflows = 0;         /* # of overflows of output */
int buf_switches = 0;          /* # of switches from small to large buf */
int _shutdown = 0;             /* clean shutdown */
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
void update_characters(void);
void short_activity();
void skip_spaces(char **string);
char *any_one_arg(char *argument, char *first_arg);
char *calc_color(int hit, int max_hit);
std::string get_from_q(std::queue<std::string> &input_queue);
void signal_setup(void);
int new_descriptor(int s);
int process_input(class Connection *t);
void flush_queues(class Connection *d);
int perform_subst(class Connection *t, char *orig, char *subst);

void check_idle_passwords(void);
void init_heartbeat();
void heartbeat();
void report_debug_logging();

/* extern fcnts */
void pulse_takeover(void);
void zone_update(void);
void point_update(void); /* In limits.c */
void food_update(void);  /* In limits.c */
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
int DC::write_hotboot_file(void)
{
  FILE *fp;
  class Connection *sd;
  if ((fp = fopen("hotboot", "w")) == nullptr)
  {
    DC::getInstance()->logmisc(QStringLiteral("Hotboot failed, unable to open hotboot file."));
    return 0;
  }
  // for_each(dc.server_descriptor_list.begin(), dc.server_descriptor_list.end(), [fp](server_descriptor_list_i i)
  for_each(server_descriptor_list.begin(), server_descriptor_list.end(), [&fp](const int &fd)
           { fprintf(fp, "%d\n", fd); });

  for (Connection *d = descriptor_list; d; d = sd)
  {
    sd = d->next;
    if (STATE(d) != Connection::states::PLAYING || !d->character || d->character->getLevel() < 1)
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
        fprintf(fp, "%d\n%s\n%s\n", d->descriptor, GET_NAME(d->original), qPrintable(d->getPeerOriginalAddress().toString()));
        if (d->original->player)
        {
          d->original->player->last_site = d->original->desc->getPeerOriginalAddress().toString();
          d->original->player->time.logon = time(0);
        }
        d->original->save_char_obj();
      }
      else
      {
        fprintf(fp, "%d\n%s\n%s\n", d->descriptor, GET_NAME(d->character), qPrintable(d->getPeerOriginalAddress().toString()));
        if (d->character->player)
        {
          d->character->player->last_site = d->character->desc->getPeerOriginalAddress().toString();
          d->character->player->time.logon = time(0);
        }
        d->character->save_char_obj();
      }
      write_to_descriptor(d->descriptor, "Attempting to maintain your link during reboot.\r\nPlease wait..");
    }
  }
  fclose(fp);
  DC::getInstance()->logmisc(QStringLiteral("Hotboot descriptor file successfully written."));

  chdir("../bin/");

  if (char *cwd = get_current_dir_name(); cwd)
  {
    loggod(QStringLiteral("Hotbooting %1 at [%2]").arg(applicationFilePath()).arg(cwd));
    free(cwd);
  }

  ssh.close();
  if (execv(qPrintable(applicationFilePath()), cf.argv_) == -1)
  {
    char execv_strerror[1024] = {};
    strerror_r(errno, execv_strerror, sizeof(execv_strerror));

    DC::getInstance()->logmisc(QStringLiteral("Hotboot execv(%1, argv) failed with error: %2").arg(applicationFilePath()).arg(execv_strerror));

    // wipe the file since we can't use it anyway
    if (unlink("hotboot") == -1)
    {
      char unlink_strerror[1024] = {};
      strerror_r(errno, unlink_strerror, sizeof(unlink_strerror));

      DC::getInstance()->logmisc(QStringLiteral("Hotboot unlink(\"hotboot\") failed with error: %1").arg(unlink_strerror));
    }

    chdir(qPrintable(cf.library_directory));
    return 0;
  }

  return 1;
}

// attempts to read in the descs written to file, and reconnect their
// links to the mud.
int DC::load_hotboot_descs(void)
{
  QFile hotboot_file{QStringLiteral("hotboot")};
  if (!hotboot_file.exists())
  {
    return {};
  }

  if (hotboot_file.open(QFile::ReadOnly))
  {
    unlink("hotboot");
    DC::getInstance()->logmisc(QStringLiteral("Hotboot, reloading characters."));

    QTextStream out(&hotboot_file);
    for_each(cf.ports.begin(), cf.ports.end(), [this, &out](in_port_t &port)
             {
             int fd;
            out >> fd;
            //qDebug("Read %d for port %d", fd, port);
            server_descriptor_list.insert(fd); });

    while (!out.atEnd())
    {
      int descriptor;
      out >> descriptor;
      if (out.atEnd())
        break;
      QString character_name;
      out >> character_name;
      if (out.atEnd())
        break;
      QString address;
      out >> address;
      if (out.atEnd())
        break;
      // qDebug("Read %d as descriptor for character '%s' from %s", descriptor, qPrintable(character_name), qPrintable(address));

      auto d = new Connection;
      d->idle_time = 0;
      d->idle_tics = 0;
      d->wait = 1;
      d->prompt_mode = 1;
      d->output = {};
      d->input = std::queue<std::string>();
      d->output = qPrintable(character_name); // store it for later
      d->login_time = time(0);
      d->setPeerAddress(QHostAddress(address));
      d->descriptor = descriptor;

      for (const auto &str : {"Recovering...\r\n", "Link recovery successful.\r\nPlease wait while mud finishes rebooting...\r\n"})
      {
        if (write_to_descriptor(descriptor, str) == -1)
        {
          DC::getInstance()->logentry(QStringLiteral("Address: %1 Character: %2 Descriptor: %3 failed to recover from hotboot.").arg(address).arg(character_name).arg(descriptor));
          CLOSE_SOCKET(descriptor);
          delete d;
          d = {};
          break;
        }
      }
      if (!d)
        continue;

      d->next = DC::getInstance()->descriptor_list;
      DC::getInstance()->descriptor_list = d;
    }
    hotboot_file.close();
  }

  unlink("hotboot"); // if the above unlink failed somehow(?),
                     // remove the hotboot file so that it dosen't think
                     // next reboot is another hotboot
  DC::getInstance()->logmisc(QStringLiteral("Successful hotboot file read."));
  return 1;
}

vnum_t DC::getObjectVNUM(Object *obj, bool *ok)
{
  if (obj && obj_index)
  {
    if (ok)
    {
      *ok = true;
    }
    return obj_index[obj->item_number].virt;
  }

  if (ok)
  {
    *ok = false;
  }
  return INVALID_VNUM;
}

vnum_t DC::getObjectVNUM(legacy_rnum_t nr, bool *ok)
{
  if (nr >= 0 && nr <= top_of_objt && obj_index)
  {
    if (ok)
    {
      *ok = true;
    }
    return obj_index[nr].virt;
  }

  if (ok)
  {
    *ok = false;
  }
  return INVALID_VNUM;
}

vnum_t DC::getObjectVNUM(rnum_t nr, bool *ok)
{
  if (nr != DC::INVALID_RNUM && nr <= top_of_objt && obj_index)
  {
    if (ok)
    {
      *ok = true;
    }
    return obj_index[nr].virt;
  }

  if (ok)
  {
    *ok = false;
  }
  return INVALID_VNUM;
}

void DC::finish_hotboot(void)
{
  class Connection *d;
  char buf[MAX_STRING_LENGTH];

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    write_to_descriptor(d->descriptor, "Reconnecting your link to your character...\r\n");

    if (!load_char_obj(d, d->output))
    {
      DC::getInstance()->logmisc(QStringLiteral("Could not load char '%1' in hotboot.").arg(d->output));
      write_to_descriptor(d->descriptor, "Link Failed!  Tell an Immortal when you can.\r\n");
      close_socket(d);
      continue;
    }

    write_to_descriptor(d->descriptor, "Success...May your visit continue to suck...\r\n");

    d->output.clear();

    auto &character_list = DC::getInstance()->character_list;
    character_list.insert(d->character);

    d->character->do_on_login_stuff();

    STATE(d) = Connection::states::PLAYING;

    update_max_who();
  }

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    do_look(d->character, "");
    d->character->save(cmd_t::SAVE_SILENTLY);
  }
}

/* Init sockets, run game, and cleanup sockets */
void DC::init_game(void)
{
  FILE *fp;
  // create boot'ing lockfile
  if ((fp = fopen("died_in_bootup", "w")))
  {
    fclose(fp);
  }

  logverbose(QStringLiteral("Attempting to load hotboot file."));

  if (load_hotboot_descs())
  {
    logverbose(QStringLiteral("Hotboot Loading complete."));
    was_hotboot = 1;
  }
  else
  {
    logverbose(QStringLiteral("Hotboot failed.  Starting regular sockets."));
    logverbose(QStringLiteral("Opening mother connections."));

    for_each(cf.ports.begin(), cf.ports.end(), [this](in_port_t &port)
             {
               DC::getInstance()->logf(0, DC::LogChannel::LOG_MISC, "Opening port %d.", port);
               int listen_fd = init_socket(port);
               if (listen_fd >= 0)
               {
                 server_descriptor_list.insert(listen_fd);
               }
               else
               {
                 DC::getInstance()->logf(0, DC::LogChannel::LOG_MISC, "Error opening port %d.", port);
               } });
  }

  start_time = time(0);
  boot_db();

  if (was_hotboot)
  {
    DC::getInstance()->logmisc(QStringLiteral("Connecting hotboot characters to their descriptiors"));
    finish_hotboot();
  }

  logverbose(QStringLiteral("Signal trapping."));
  signal_setup();

  // we got all the way through, let's turn auto-hotboot back on
  try_to_hotboot_on_crash = 1;

  DC::getInstance()->logmisc(QStringLiteral("Entering game loop."));

  unlink("died_in_bootup");

  if (DC::getInstance()->cf.bport == false)
  {
    do_not_save_corpses = 0;
  }

  if (cf.testing)
  {
    game_test_init();
  }
  else
  {
    game_loop_init();
  }

  do_not_save_corpses = 1;

  DC::getInstance()->logmisc(QStringLiteral("Closing all sockets."));
  while (DC::getInstance()->descriptor_list)
  {
    close_socket(DC::getInstance()->descriptor_list);
  }

  for_each(server_descriptor_list.begin(), server_descriptor_list.end(), [](const int &fd)
           {
             DC::getInstance()->logf(0, DC::LogChannel::LOG_MISC, "Closing fd %d.", fd);
             CLOSE_SOCKET(fd); });

  DC::getInstance()->logmisc(QStringLiteral("Goodbye."));
  DC::getInstance()->logmisc(QStringLiteral("Normal termination of game."));
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
std::stringstream timingDebugStr;
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
      // qDebug() << QStringLiteral("%1 msec.").arg(msecs_expired);
    }
    else
    {
      // qDebug() << QStringLiteral("%1 nsec.").arg(nsecs_expired);
    }

    last_execution.restart();
  }
  else
  {
    last_execution.start();
  }

  // comm must be much longer than MAX_INPUT_LENGTH since we allow aliases in-game
  // otherwise an alias'd command could easily overrun the buffer
  std::string comm = {};
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
  fd_set &input_set = input_set;
  for_each(server_descriptor_list.begin(), server_descriptor_list.end(), [&input_set, &maxdesc](const int &fd)
           {
               FD_SET(fd, &input_set);
               if (fd > maxdesc)
               {
                 maxdesc = fd;
               } });

  for (d = descriptor_list; d; d = d->next)
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
  for (d = descriptor_list; d; d = next_d)
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
  for (d = descriptor_list; d; d = next_d)
  {
    next_d = d->next;

    const auto idle_seconds = d->idle_time / DC::PASSES_PER_SEC;
    if (FD_ISSET(d->descriptor, &input_set))
    {
      if (process_input(d) < 0)
      {
        close_socket(d);
      }
    }
    else
    {
      switch (d->connected)
      {
      case Connection::states::GET_NAME:
      case Connection::states::GET_OLD_PASSWORD:
        if (idle_seconds > 30)
        {
          SEND_TO_Q(QStringLiteral("Disconnected for being idle for over 30 seconds.\r\n"), d);
          close_socket(d);
        }
        break;
      }
    }
  }

  /* process commands we just read from process_input */
  for (d = descriptor_list; d; d = next_d)
  {
    if (d->character != nullptr)
    {
      QLocale::setDefault(QLocale(d->character->getSetting("locale", "en_US")));
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
      // std::cerr << "Got command [" << comm << "] from the d->input queue" << std::endl;
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
      else if (d->strnew && (IS_NPC(d->character) || !isSet(d->character->player->toggles, Player::PLR_EDITOR_WEB)))
        new_string_add(d, comm.data());
      else if (d->connected != Connection::states::PLAYING) /* in menus, etc. */
        nanny(d, comm);
      else
      {              /* else: we're playing normally */
        if (aliased) /* to prevent recursive aliases */
          d->prompt_mode = 0;
        else if (d && d->character && d->character->player)
        {
          comm = d->character->player->perform_alias(comm.c_str()).toStdString();
        }
        PerfTimers["command"].start();
        // Azrack's a chode.  Don't forget to check
        // ->snooping before you check snooping->char:P
        if (!comm.empty() && comm[0] == '%' && d->snooping && d->snooping->character)
        {
          d->snooping->character->command_interpreter(comm.substr(1).c_str());
        }
        else
        {
          d->character->command_interpreter(comm.c_str()); /* send it to interpreter */
        }
        PerfTimers["command"].stop();

      } // else if input
    } // if input
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
    if ((FD_ISSET(d->descriptor, &output_set) && !d->output.isEmpty()) || d->prompt_mode)
      if (d->process_output() < 0)
        close_socket(d);
    // else
    // d->prompt_mode = 1;
  }
  PerfTimers["output"].stop();
  // we're done with this pulse.  Now calculate the time until the next pulse and sleep until then
  // we want to pulse DC::PASSES_PER_SEC times a second (duh).  This is currently 4.

  gettimeofday(&now_time_, nullptr);

  // temp removing this since it's spamming the crap out of us
  // else DC::getInstance()->logf(110, DC::LogChannel::LOG_BUG, "0 delay on pulse");
  gettimeofday(&last_time_, nullptr);
  PerfTimers["gameloop"].stop();
  FrameMark;
}

void DC::game_loop_init(void)
{
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  FD_ZERO(&null_set);
  init_heartbeat();

  gettimeofday(&last_time_, nullptr);

  QTimer *gameLoopTimer = new QTimer(this);
  connect(gameLoopTimer, &QTimer::timeout, this, &DC::game_loop);
  gameLoopTimer->start(1000 / DC::PASSES_PER_SEC);

  QHttpServer server(this);
  QStringList myData;
  auto dc = this;

  server.route("/myApi/<arg>", QHttpServerRequest::Method::Get,
               [&dc](int id, const QHttpServerRequest &request)
               {
                 return QHttpServerResponse(QStringLiteral("int:%1\r\n").arg(id));
               });
  server.route("/myApi/<arg>/<arg>", QHttpServerRequest::Method::Get,
               [&dc](QString str, QString str2, const QHttpServerRequest &request)
               {
                 return QHttpServerResponse(QStringLiteral("str:%1 str2:%2\r\n").arg(str).arg(str2));
               });

  server.route("/test", [&dc](const QHttpServerRequest &request)
               {
    if (!dc->authenticate(request))
    {
      return QHttpServerResponse(QStringLiteral("Failed to authenticate.\r\n"));
    }

    auto future = QtConcurrent::run([]() {});

    if (future.isValid())
    {
      // QJsonArray array = QJsonArray::fromStringList(myData);

      return QHttpServerResponse(QStringLiteral("Success.\r\n"));
    }
    else
    {
      return QHttpServerResponse("Failed.\r\n");
    } });

  server.route("/", [&dc](const QHttpServerRequest &request)
               {
    if (!dc->authenticate(request))
    {
      return QHttpServerResponse(QStringLiteral("Failed to authenticate.\r\n"));
    }

    auto future = QtConcurrent::run([]() {});

    if (future.isValid())
    {
      return QHttpServerResponse(QStringLiteral("Success.\r\n"));
    }
    else
    {
      return QHttpServerResponse("Failed.\r\n");
    } });

  server.route("/shutdown", QHttpServerRequest::Method::Get, [this, &dc](const QHttpServerRequest &request)
               {
                 if (!dc->authenticate(request, 110))
                 {
                   return QHttpServerResponse(QStringLiteral("Failed to authenticate.\r\n"));
                 }

                 auto future = QtConcurrent::run([]() {});

                 int do_not_save_corpses = 1;

                 QString buf = QStringLiteral("Hot reboot by %1.\r\n").arg("HTTP /shutdown/");
                 send_to_all(buf);
                 DC::getInstance()->logentry(buf, ANGEL, DC::LogChannel::LOG_GOD);
                 DC::getInstance()->logmisc(QStringLiteral("Writing sockets to file for hotboot recovery."));

                 for (const auto &ch : dc->character_list)
                 {
                   if (ch->player && IS_PC(ch))
                   {
                     ch->save();
                   }
                 }

                 if (!write_hotboot_file())
                 {
                   DC::getInstance()->logmisc(QStringLiteral("Hotboot failed.  Closing all sockets."));
                   return QHttpServerResponse("Failed.\r\n");
                 }

                 return QHttpServerResponse("Rebooting.\r\n"); });

  QLoggingCategory::setFilterRules("qt.httpserver=true");

  auto tcpserver = new QTcpServer();
  if (!tcpserver->listen(QHostAddress::LocalHost, 6980))
  {
    DC::getInstance()->logmisc(QStringLiteral("Unable to listen to port 6980."));
  }

  if (!server.bind(tcpserver))
  {
    DC::getInstance()->logmisc(QStringLiteral("Unable to bind HTTP server."));
  }

  exec();

  ssh.close();
}

void DC::game_test_init(void)
{
  assert(remove_all_codes(QStringLiteral("$B")) == "$$B");
  assert(remove_non_color_codes(QStringLiteral("$B")) == "$B");
  assert(nocolor_strlen(QStringLiteral("$B")) == 0);
  assert(remove_all_codes(QStringLiteral("$B123$R")) == "$$B123$$R");
  assert(remove_non_color_codes(QStringLiteral("$B123$R")) == "$B123$R");
  assert(nocolor_strlen(QStringLiteral("$B123$R")) == 3);

  char arg[] = " start 1";
  char name[MAX_STRING_LENGTH] = {};
  half_chop(arg, arg, name);
  assert(!strcmp(arg, "start"));
  assert(!strcmp(name, "1"));

  auto d = new Connection;
  Character *ch = new Character(this);
  ch->setName("Debugimp");
  ch->player = new Player;
  ch->setType(Character::Type::Player);

  ch->desc = d;
  ch->setLevel(110);
  d->descriptor = 1;
  d->character = ch;
  d->output = {};

  auto &character_list = DC::getInstance()->character_list;
  character_list.insert(d->character);

  d->character->do_on_login_stuff();

  STATE(d) = Connection::states::PLAYING;

  for (uint32_t i = 0; i < UINT32_MAX; ++i)
  {
    QString aff_name = get_skill_name(i);
  }

  update_max_who();

  do_restore(ch, "debugimp");

  do_stand(ch, "");
  d->process_output();

  char_to_room(ch, 3001);
  d->process_output();
  ch->do_toggle({"pager"}, cmd_t::DEFAULT);
  ch->do_toggle({"ansi"}, cmd_t::DEFAULT);
  ch->do_toggle({}, cmd_t::DEFAULT);
  ch->do_goto({"23"});
  do_score(ch, "");
  d->process_output();

  do_load(ch, "m 23");
  d->process_output();

  do_look(ch, "debugimp");
  d->process_output();

  ch->do_bestow({"debugimp", "load"});
  d->process_output();

  do_load(ch, "m 23");
  d->process_output();

  ch->do_test({"all"});
  d->process_output();

  std::cout << std::endl;
  std::cerr << std::endl;
  exit(EXIT_SUCCESS);
}

extern void pulse_hunts();
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

void DC::heartbeat(void)
{
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
    TheAuctionHouse.CheckExpire();
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
      send_hint();
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

    if (cf.bport == false)
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

int do_lastprompt(Character *ch, char *arg, cmd_t cmd)
{
  if (ch->getLastPrompt().isEmpty())
    ch->sendln("Last prompt: unset");
  else
    ch->sendln(QStringLiteral("Last prompt: %1").arg(ch->getLastPrompt()));

  return eSUCCESS;
}

int do_prompt(Character *ch, char *arg, cmd_t cmd)
{
  while (*arg == ' ')
    arg++;

  if (IS_NPC(ch))
  {
    ch->sendln("You're a mob!  You can't set your prompt.");
    return eFAILURE;
  }

  if (!*arg)
  {
    ch->sendln("Set your prompt to what? Try 'help prompt'.");
    if (!ch->getPrompt().isEmpty())
    {
      ch->send("Current prompt:  ");
      send_to_char(ch->getPrompt(), ch);
      ch->sendln("");
      ch->send("Last prompt: ");
      if (!ch->getLastPrompt().isEmpty())
      {
        send_to_char(ch->getLastPrompt(), ch);
      }
      else
      {
        ch->send("unset");
      }
      ch->sendln("");
    }
    return eSUCCESS;
  }

  ch->setLastPrompt(ch->getPrompt());
  ch->setPrompt(arg);
  ch->sendln("Ok.");
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

char *calc_condition(Character *ch, bool colour)
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

QString Connection::createPrompt(void)
{
  QString prompt, buf;
  if (!character || !character->player)
  {
    return {};
  }
  if (showstr_count)
  {
    return QStringLiteral("\r\n[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%1 %2) ]").arg(showstr_page).arg(showstr_count);
  }
  else if (strnew)
  {
    if (IS_PC(character) && isSet(character->player->toggles, Player::PLR_EDITOR_WEB))
    {
      return "Web Editor] ";
    }
    else
    {
      return "*] ";
    }
  }
  else if (hashstr)
  {
    return "] ";
  }
  else if (STATE(this) != Connection::states::PLAYING)
  {
    return {};
  }
  else if (IS_NPC(character))
  {
    return character->createPrompt();
  }
  else
  {
    if (!isSet(GET_TOGGLES(character), Player::PLR_COMPACT))
      prompt += "\r\n";
    if (character->getPrompt().isEmpty())
      prompt += "type 'help prompt'> ";
    else
      prompt += character->createPrompt();
  }
  return prompt;
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

void write_to_q(const std::string txt, std::queue<std::string> &input_queue)
{
#ifdef DEBUG_INPUT
  // std::cerr << "Writing to queue '" << txt << "'" << std::endl;
#endif
  input_queue.push(txt);
}

std::string get_from_q(std::queue<std::string> &input_queue)
{
  if (input_queue.empty())
  {
    return std::string();
  }

  std::string dest = input_queue.front();
  input_queue.pop();

  return dest;
}

/* Empty the queues before closing connection */
void flush_queues(class Connection *d)
{
  while (!get_from_q(d->input).empty())
    ;
  if (!d->output.isEmpty())
  {
    write_to_descriptor(d->descriptor, d->output);
  }
}

void DC::free_buff_pool_from_memory(void)
{
  struct txt_block *curr = nullptr;

  while (bufpool)
  {
    curr = bufpool->next;
    delete bufpool;
    bufpool = curr;
  }
}

void scramble_text(std::string &txt)
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
  for (auto &c : input)
  {
    // only scramble letters, but not 'm' cause 'm' is used in ansi codes
    if (number(1, 5) == 5 && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) && c != 'm')
    {
      c = number(0, 1) ? (char)number('a', 'z') : (char)number('A', 'Z');
    }
  }

  return input;
}

void write_to_output(const char *txt, class Connection *t)
{
  if (txt)
  {
    write_to_output(QByteArray(txt), t);
  }
}

void write_to_output(std::string txt, class Connection *t)
{
  if (!txt.empty())
  {
    write_to_output(QByteArray(txt.c_str()), t);
  }
}

void write_to_output(QString txt, class Connection *t)
{
  if (!txt.isEmpty())
  {
    write_to_output(QByteArray(qPrintable(txt)), t);
  }
}

void write_to_output(QByteArray txt, class Connection *t)
{
  /* if there's no descriptor, don't worry about output */
  if (t->descriptor == 0)
    return;

  if (t->allowColor && !t->isEditing())
  {
    txt = handle_ansi(txt, t->character);
  }
  if (t->character && IS_AFFECTED(t->character, AFF_INSANE) && t->connected == Connection::states::PLAYING)
  {
    txt = scramble_text(txt.toStdString().c_str()).toStdString().c_str();
  }
  t->output.append(txt);
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
  if (auto return_value = getsockname(s, (struct sockaddr *)&peer, &i) == -1)
  {
    auto saved_errno = errno;
    qDebug("getsockname(%d, &peer, %d) returned %d with errno %d %s", s, i, return_value, saved_errno, strerror(saved_errno));
  }

  if ((desc = accept(s, (struct sockaddr *)&peer, &i)) < 0)
  {
    auto saved_errno = errno;
    qDebug("accept(%d, &peer, %d) returned %d with errno %d %s", s, i, desc, saved_errno, strerror(saved_errno));
    return -1;
  }

  // keep it from blocking
  if (fcntl(desc, F_SETFL, O_NONBLOCK) < 0)
  {
    perror("init_socket : fcntl : nonblock");
    exit(1);
  }

  /* create a new descriptor */
  newd = new Connection;
  newd->setPeerAddress(QHostAddress(inet_ntoa(peer.sin_addr)));

  /* determine if the site is banned */
  if (isbanned(newd->getPeerOriginalAddress()) == BAN_ALL)
  {
    write_to_descriptor(desc, "Your site has been banned from Dark Castle. If you have any\r\n"
                              "Questions, please email us at:\r\n"
                              "imps@dcastle.org\r\n");

    CLOSE_SOCKET(desc);
    DC::getInstance()->logentry(QStringLiteral("Connection attempt denied from [%1]").arg(newd->getPeerOriginalAddress().toString()), OVERSEER, DC::LogChannel::LOG_SOCKET);
    delete newd;
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

QString Connection::createBlackjackPrompt(void)
{
  if (character)
    return character->createBlackjackPrompt();
  return {};
}

void Connection::setOutput(QString output_buffer)
{
  output = qUtf8Printable(output_buffer);
}

void Connection::appendOutput(QString output_buffer)
{
  output += qUtf8Printable(output_buffer);
}

QByteArray Connection::getOutput(void) const
{
  return output;
}

int Connection::process_output(void)
{
  QByteArray i = output;

  /* now, append the 'real' output */
  i += qUtf8Printable(createBlackjackPrompt());
  i += qUtf8Printable(createPrompt());

  // As long as we're not in telnet character mode then send IAC GA
  if (character && character->getSetting("mode").startsWith("char") == false)
  {
    char go_ahead[] = {(char)IAC, (char)GA, (char)0};
    i += go_ahead;
  }

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  int result{};
  if (!prompt_mode)
  { /* && !t->connected) */
    result = write_to_descriptor(descriptor, "\r\n" + i);
    prompt_mode = 0;
  }
  else
  {
    result = write_to_descriptor(descriptor, i);
    prompt_mode = 0;
  }
  /* handle snooping: prepend "% " and send to snooper */
  if (snoop_by)
  {
    SEND_TO_Q("% ", snoop_by);
    SEND_TO_Q(output, snoop_by);
    SEND_TO_Q("%%", snoop_by);
  }

  output = {};

  return result;
}

int write_to_descriptor(int desc, QByteArray txt)
{
  if (txt.isEmpty())
    return 0;

  auto txtPtr = txt.constData();

  auto total = txt.size();
  do
  {
    auto bytes_written = write(desc, txtPtr, total);
    if (bytes_written < 0)
    {
      if (errno != EAGAIN && errno != EWOULDBLOCK)
      {
        DC::getInstance()->logmisc(QStringLiteral("write(%1,-,%2) returned %3 and errno=%4").arg(desc).arg(total).arg(bytes_written).arg(errno));
        if (errno != EPIPE)
          return -1;
      }
      return 0;
    }
    else
    {
      txtPtr += bytes_written;
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
          // std::cerr << "Unrecognized telnet option " << hex << static_cast<int>(c) << std::endl;
          prev = 0;
          break;
        }
      }
      else if (prev == telnet::do_opt)
      {
        if (c == '\x1')
        {
          // std::cerr << "Telnet client requests to turn on server-side echo" << std::endl;
          t->server_size_echo = true;
        }
        else if (c == '\x3')
        {
          // std::cerr << "Telnet client requests server to suppress sending go-ahead" << std::endl;
        }
        else
        {
          // std::cerr << "Unrecognized do option " << hex << static_cast<int>(c) << std::endl;
        }
        prev = 0;
      }
      else
      {
        // std::cerr << "Unrecognized telnet code " << hex << static_cast<int>(c) << std::endl;
        prev = 0;
      }
    }
    t->inbuf.erase(iac_pos, processed);
  }
}

std::string removeUnprintable(std::string input)
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

std::string makePrintable(std::string input)
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
        DC::getInstance()->logentry(QStringLiteral("Connection broken by peer %1 playing %2.").arg(t->getPeerAddress().toString()).arg(GET_NAME(t->character)), IMPLEMENTER + 1, DC::LogChannel::LOG_SOCKET);
      }
      else
      {
        DC::getInstance()->logentry(QStringLiteral("Connection broken by peer %1 not playing a character.").arg(t->getPeerAddress().toString()), IMPLEMENTER + 1, DC::LogChannel::LOG_SOCKET);
      }

      return -1;
    }
    std::string buffer = c_buffer;
    t->inbuf += buffer;

    // Search for telnet control codes
    process_iac(t);
    if (t->server_size_echo)
    {
      std::string new_buffer;
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
            // // std::cerr << "Before: [" << t->inbuf << "]" << t->inbuf.size() << std::endl;
            t->inbuf.erase(t->inbuf.end() - 1, t->inbuf.end());
            // // std::cerr << "After: [" << t->inbuf << "]" << t->inbuf.size() << std::endl;
          }
          if (t->inbuf.size() >= 2)
          {
            // // std::cerr << "Before: [" << t->inbuf << "]" << t->inbuf.size() << std::endl;
            t->inbuf.erase(t->inbuf.end() - 2, t->inbuf.end());
            // // std::cerr << "After: [" << t->inbuf << "]" << t->inbuf.size() << std::endl;
            new_buffer += "\b \b";
          }
        }
        else if (c == '\r')
        {
          new_buffer += "\r\n";
        }
        write_to_descriptor(t->descriptor, new_buffer.c_str());
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
    // std::cerr << "old t->inbuf [" << makePrintable(t->inbuf) << "]"
    << "(" << t->inbuf.length() << ")" << std::endl;
#endif
    std::string buffer = t->inbuf.substr(0, eoc_pos);
    t->inbuf.erase(0, eoc_pos + erase);
#ifdef DEBUG_INPUT
    // std::cerr << "new t->inbuf [" << makePrintable(t->inbuf) << "]"
    << "(" << t->inbuf.length() << ")" << std::endl;
    // std::cerr << "buffer [" << makePrintable(buffer) << "]"
    << "(" << buffer.length() << ")" << std::endl;
#endif

    if (t->character == nullptr || t->character->isMortalPlayer())
    {
      buffer = remove_all_codes(buffer);
    }

    // Only search for pipe (|) when not editing
    if (!t->isEditing())
    {
      size_t pipe_pos = 0;
      do
      {
        pipe_pos = buffer.find('|');
        if (pipe_pos != buffer.npos)
        {
          std::string new_buffer = buffer.substr(0, pipe_pos);
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
        std::string buffer;

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


  //  while (ISNEWL(*nl_pos) || (t->connected != Connection::states::WRITE_BOARD && t->connected != Connection::states::EDITING && t->connected != Connection::states::EDIT_MPROG && *nl_pos == '|'))
  //    nl_pos++;

  // see if there's another newline in the input buffer

    read_point = ptr = nl_pos;
    for (nl_pos = nullptr; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr) || (t->connected != Connection::states::WRITE_BOARD && t->connected != Connection::states::EDITING && t->connected != Connection::states::EDIT_MPROG && *ptr == '|'))
        nl_pos = ptr;

  }
  //
  // now move the rest of the buffer up to the beginning for the next pass
  //
      write_point = t->inbuf.data();
    while (*read_point)
      *(write_point++) = *(read_point++);
    *write_point = '\0';
  */

  return 1;
}

/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig std::string (i.e. the one being modified.
 * subst contains the substition std::string, i.e. "^telm^tell"
 */
int perform_subst(class Connection *t, char *orig, char *subst)
{
  char new_subst[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first std::string (the one
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
   * to the beginning of the second std::string */
  *(second++) = '\0';

  /* now, see if the contents of the first std::string appear in the original */
  if (!(strpos = strstr(orig, first)))
  {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  /* now, we construct the new std::string for output. */

  /* first, everything in the original, up to the std::string to be replaced */
  strncpy(new_subst, orig, (strpos - orig));
  new_subst[(strpos - orig)] = '\0';

  /* now, the replacement std::string */
  strncat(new_subst, second, (MAX_INPUT_LENGTH - strlen(new_subst) - 1));

  /* now, if there's anything left in the original after the std::string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(new_subst, strpos + strlen(first),
            (MAX_INPUT_LENGTH - strlen(new_subst) - 1));

  /* terminate the std::string in case of an overflow from strncat */
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
    strcpy(idiotbuf, "\r\n~\r\n");
    strcat(idiotbuf, "\0");
    string_hash_add(d, idiotbuf);
  }
  if (d->strnew && (IS_NPC(d->character) || !isSet(d->character->player->toggles, Player::PLR_EDITOR_WEB)))
  {
    strcpy(idiotbuf, "/s\r\n");
    strcat(idiotbuf, "\0");
    new_string_add(d, idiotbuf);
  }
  if (d->character)
  {
    // target_idnum = GET_IDNUM(d->character);
    if (d->isPlaying() || d->isEditing())
    {
      d->character->save_char_obj();
      // clan area stuff
      extern void check_quitter(Character * ch);
      check_quitter(d->character);

      // end any performances
      if (IS_SINGING(d->character))
        do_sing(d->character, "stop");

      act("$n has lost $s link.", d->character, 0, 0, TO_ROOM, 0);

      if (IS_AFFECTED(d->character, AFF_CANTQUIT))
      {
        DC::getInstance()->logsocket(QStringLiteral("%1@%2 has disconnected from room %3 with CANTQUIT.").arg(d->character->getName()).arg(d->getPeerFullAddressString()).arg(DC::getInstance()->world[d->character->in_room].number));
      }
      else
      {
        DC::getInstance()->logsocket(QStringLiteral("%1@%2 has disconnected from room %3.").arg(d->character->getName()).arg(d->getPeerFullAddressString()).arg(DC::getInstance()->world[d->character->in_room].number));
      }
      d->character->desc = nullptr;
    }
    else
    {
      sprintf(buf, "Losing player: %s.",
              GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
      DC::getInstance()->logentry(buf, 111, DC::LogChannel::LOG_SOCKET);
      if (d->isEditing())
      {
        //		sprintf(buf, "Suspicious: %s.",
        //			GET_NAME(d->character));
        //		DC::getInstance()->logentry(buf, 110, LOG_HMM);
      }
      free_char(d->character, Trace("close_socket"));
    }
  }
  //   Removed this log caues it's so fricken annoying
  //   else
  //    DC::getInstance()->logentry(QStringLiteral("Losing descriptor without char."), ANGEL, DC::LogChannel::LOG_SOCKET);

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = nullptr;

  // if we're closing the socket that is next to be processed, we want to
  // go ahead and move on to the next one
  if (d == next_d)
    next_d = d->next;

  REMOVE_FROM_LIST(d, DC::getInstance()->descriptor_list, next);

  if (d->showstr_head)
    delete[] d->showstr_head;
  if (d->showstr_count)
    delete[] d->showstr_vector;

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
       do_quit(i, "", cmd_t::SAVE_SILENTLY);
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

void report_debug_logging()
{
  DC::getInstance()->logentry(QStringLiteral("Name: [%1] Last cmd: [%2] Last room: [%3]").arg(DC::getInstance()->last_char_name).arg(DC::getInstance()->last_processed_cmd).arg(DC::getInstance()->last_char_room), ANGEL, DC::LogChannel::LOG_BUG);
}

void DC::crash_hotboot(void)
{
  class Connection *d = nullptr;
  extern int try_to_hotboot_on_crash;
  extern int died_from_sigsegv;

  // This can be dangerous, because if we had a SIGSEGV due to a descriptor being
  // invalid, we're going to do it again.  That's why we put in extern int died_from_sigsegv
  // sigsegv = # of times we've crashed from SIGSEGV

  for (d = descriptor_list; d && died_from_sigsegv < 2; d = d->next)
  {
    write_to_descriptor(d->descriptor, "Mud crash detected.\r\n");
  }

  // attempt to hotboot
  if (try_to_hotboot_on_crash)
  {
    for (d = descriptor_list; d && died_from_sigsegv < 2; d = d->next)
    {
      write_to_descriptor(d->descriptor, "Attempting to recover with a hotboot.\r\n");
    }
    DC::getInstance()->logentry(QStringLiteral("Attempting to hotboot from the crash."), ANGEL, DC::LogChannel::LOG_BUG);
    write_hotboot_file();
    // we shouldn't return from there unless we failed
    DC::getInstance()->logentry(QStringLiteral("Hotboot crash recovery failed.  Exiting."), ANGEL, DC::LogChannel::LOG_BUG);
    for (d = descriptor_list; d && died_from_sigsegv < 2; d = d->next)
    {
      write_to_descriptor(d->descriptor, "Hotboot failed giving up.\r\n");
    }
  }

  for (d = descriptor_list; d && died_from_sigsegv < 2; d = d->next)
  {
    write_to_descriptor(d->descriptor, "Giving up, goodbye.\r\n");
  }
}

void crashill(int sig)
{
  report_debug_logging();
  DC::getInstance()->logentry(QStringLiteral("Recieved SIGFPE (Illegal Instruction)"), ANGEL, DC::LogChannel::LOG_BUG);
  DC::getInstance()->crash_hotboot();
  DC::getInstance()->logentry(QStringLiteral("Mud exiting from SIGFPE."), ANGEL, DC::LogChannel::LOG_BUG);
  exit(0);
}

void crashfpe(int sig)
{
  report_debug_logging();
  DC::getInstance()->logentry(QStringLiteral("Recieved SIGFPE (Arithmetic Error)"), ANGEL, DC::LogChannel::LOG_BUG);
  DC::getInstance()->crash_hotboot();
  DC::getInstance()->logentry(QStringLiteral("Mud exiting from SIGFPE."), ANGEL, DC::LogChannel::LOG_BUG);
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
    DC::getInstance()->logentry(QStringLiteral("Hit 'died_from_sigsegv > 2'"), ANGEL, DC::LogChannel::LOG_BUG);
    exit(0);
  }
  report_debug_logging();
  DC::getInstance()->logentry(QStringLiteral("Recieved SIGSEGV (Segmentation fault)"), ANGEL, DC::LogChannel::LOG_BUG);
  DC::getInstance()->crash_hotboot();
  DC::getInstance()->logentry(QStringLiteral("Mud exiting from SIGSEGV."), ANGEL, DC::LogChannel::LOG_BUG);
  exit(0);
}

void unrestrict_game(int sig)
{
  extern struct ban_list_element *ban_list;
  extern int num_invalid;

  DC::getInstance()->logentry(QStringLiteral("Received SIGUSR2 - completely unrestricting game (emergent)"),
                              ANGEL, DC::LogChannel::LOG_GOD);
  ban_list = nullptr;
  restrict = 0;
  num_invalid = 0;
}

void hupsig(int sig)
{
  DC::getInstance()->logmisc(QStringLiteral("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down..."));
  abort(); /* perhaps something more elegant should
            * substituted */
}

void sigusr1(int sig)
{
  do_not_save_corpses = 1;
  DC::getInstance()->logmisc(QStringLiteral("Writing sockets to file for hotboot recovery."));
  if (!DC::getInstance()->write_hotboot_file())
  {
    DC::getInstance()->logmisc(QStringLiteral("Hotboot failed.  Closing all sockets."));
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
  DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "signal_handler: signo=%d errno=%d code=%d "
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
    extern int do_not_save_corpses;
    do_not_save_corpses = 1;
    send_to_all(QStringLiteral("Hot reboot by SIGHUP.\r\n"));
    DC::getInstance()->logentry(QStringLiteral("Hot reboot by SIGHUP.\r\n"), ANGEL, DC::LogChannel::LOG_GOD);
    DC::getInstance()->logmisc(QStringLiteral("Writing sockets to file for hotboot recovery."));
    if (!DC::getInstance()->write_hotboot_file())
    {
      DC::getInstance()->logmisc(QStringLiteral("Hotboot failed.  Closing all sockets."));
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

void send_to_char_regardless(QString messg, Character *ch)
{
  if (ch->desc && !messg.isEmpty())
  {
    SEND_TO_Q(messg, ch->desc);
  }
}

void send_to_char_regardless(std::string messg, Character *ch)
{
  if (ch->desc && !messg.empty())
  {
    SEND_TO_Q(messg, ch->desc);
  }
}

void send_to_char_nosp(const char *messg, Character *ch)
{
  char *tmp = str_nospace(messg);
  ch->send(tmp);
  delete[] tmp;
}

void send_to_char_nosp(QString messg, Character *ch)
{
  send_to_char_nosp(messg.toStdString().c_str(), ch);
}

void record_msg(QString messg, Character *ch)
{
  if (messg.isEmpty() || !ch->isImmortalPlayer())
    return;

  if (ch->player->away_msgs.size() < 1000)
  {
    ch->player->away_msgs.push_back(messg);
  }
}

int do_awaymsgs(Character *ch, char *argument, cmd_t cmd)
{
  int lines = 0;
  QString tmp;

  if (IS_NPC(ch))
    return eFAILURE;

  if (ch->player->away_msgs.isEmpty())
  {
    SEND_TO_Q("No messages have been recorded.\r\n", ch->desc);
    return eSUCCESS;
  }

  // Show 23 lines of text, then stop
  while (!ch->player->away_msgs.isEmpty())
  {
    tmp = ch->player->away_msgs.front();
    SEND_TO_Q(tmp, ch->desc);
    ch->player->away_msgs.pop_back();

    if (++lines == 23)
    {
      SEND_TO_Q("\r\nMore msgs available. Type awaymsgs to see them\r\n",
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

  if (ch->player->away_msgs.isEmpty())
  {
    return;
  }

  ch->send("You have unviewed away messages. ");
  ch->sendln("Type awaymsgs to view them.");
}

void send_to_char(QString messg, Character *ch)
{
  if (IS_NPC(ch) && !ch->desc && MOBtrigger && !messg.isEmpty())
    mprog_act_trigger(messg.toStdString(), ch, 0, 0, 0);
  if (IS_NPC(ch) && !ch->desc && !selfpurge && MOBtrigger && !messg.isEmpty())
    ch->oprog_act_trigger(messg);

  if (!selfpurge && (ch->desc && !messg.isEmpty()) && (!is_busy(ch)))
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

void ansi_color(const char *txt, Character *ch)
{
  // mobs don't have toggles, so they automatically get ansi on
  if (txt != nullptr && ch->desc != nullptr)
  {
    if (ch->isPlayer() &&
        !isSet(GET_TOGGLES(ch), Player::PLR_ANSI) &&
        !isSet(GET_TOGGLES(ch), Player::PLR_VT100))
      return;
    else if (ch->isPlayer() &&
             isSet(GET_TOGGLES(ch), Player::PLR_VT100) &&
             !isSet(GET_TOGGLES(ch), Player::PLR_ANSI))
    {
      if ((!strcmp(txt, GREEN)) || (!strcmp(txt, RED)) || (!strcmp(txt, BLUE)) || (!strcmp(txt, BLACK)) || (!strcmp(txt, CYAN)) || (!strcmp(txt, GREY)) || (!strcmp(txt, EEEE)) || (!strcmp(txt, YELLOW)) || (!strcmp(txt, PURPLE)))
        return;
    }
    ch->send(txt);
    return;
  }
}

void send_info(QString messg)
{
  send_info(messg.toStdString().c_str());
}

void send_info(std::string messg)
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
          !isSet(i->character->misc, DC::LogChannel::CHANNEL_INFO))
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

void send_to_room(QString messg, int room, bool awakeonly, Character *nta)
{
  Character *i = nullptr;

  // If a megaphone goes off when in someone's inventory this happens
  if (room == DC::NOWHERE)
    return;

  if (!DC::getInstance()->rooms.contains(room) || !DC::getInstance()->world[room].people)
  {
    return;
  }
  if (!messg.isEmpty())
    for (i = DC::getInstance()->world[room].people; i; i = i->next_in_room)
      if (i->desc && !is_busy(i) && nta != i)
        if (!awakeonly || GET_POS(i) > position_t::SLEEPING)
          SEND_TO_Q(messg, i->desc);
}

bool is_busy(Character *ch)
{
  if (ch->desc && ch->desc->isEditing())
  {
    return true;
  }

  return false;
}

QString Player::perform_alias(QString orig)
{
  auto arguments = orig.split(' ');
  auto arg1 = arguments.value(0);

  if (arg1.isEmpty() || aliases_.isEmpty() || !aliases_.contains(arg1))
  {
    return orig;
  }

  auto command = aliases_[arg1];
  arguments.pop_front();
  arguments.push_front(command);

  return arguments.join(' ');
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

  std::list<multiplayer> multi_list;

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

      highlev = MAX(d->character->getLevel(), ch->getLevel());
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

  for (std::list<multiplayer>::iterator i = multi_list.begin(); i != multi_list.end(); ++i)
  {
    DC::getInstance()->logf(108, DC::LogChannel::LOG_WARNING, "MultipleIP: %s -> %s / %s ", (*i).host.toString().toStdString().c_str(), (*i).name1.toStdString().c_str(), (*i).name2.toStdString().c_str());
  }
}

int do_editor(Character *ch, char *argument, cmd_t cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  if (argument == 0)
    return eFAILURE;

  if (IS_NPC(ch))
    return eFAILURE;

  csendf(ch, "Current editor: %s\r\n\r\n", isSet(ch->player->toggles, Player::PLR_EDITOR_WEB) ? "web" : "game");

  one_argument(argument, arg1);

  if (*arg1)
  {
    if (!strcmp(arg1, "web"))
    {
      SET_BIT(ch->player->toggles, Player::PLR_EDITOR_WEB);
      ch->sendln("Changing to web editor.");
      ch->sendln("Ok.");
      return eSUCCESS;
    }
    else if (!strcmp(arg1, "game"))
    {
      REMOVE_BIT(ch->player->toggles, Player::PLR_EDITOR_WEB);
      ch->sendln("Changing to in game line editor.");
      ch->sendln("Ok.");
      return eSUCCESS;
    }
  }

  ch->sendln("Usage: editor <type>");
  ch->sendln("Where type can be:");
  ch->sendln("web    - use online web editor");
  ch->sendln("game   - use in game line editor");

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
      DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, QStringLiteral("Unrecognized PROXY inet protocol family in arg2 [%1]").arg(arg2).toStdString().c_str());
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
      DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, QStringLiteral("Invalid source port [%1]").arg(arg5).toStdString().c_str());
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
      DC::getInstance()->logf(IMMORTAL, DC::LogChannel::LOG_BUG, QStringLiteral("Invalid source port [%1]").arg(arg6).toStdString().c_str());
      return;
    }

    active = true;
  }
}

const char *Connection::getPeerOriginalAddressC(void)
{
  return getPeerOriginalAddress().toString().toStdString().c_str();
}