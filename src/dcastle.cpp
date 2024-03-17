#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <unistd.h>
#include <cstring>
#include <QCoreApplication>
#include "DC.h"
#include "handler.h"
#include <iostream>

uint16_t DFLT_PORT = 6667, DFLT_PORT2 = 6666, DFLT_PORT3 = 4000, DFLT_PORT4 = 6669;

void init_game(void);
void backup_executable(char *const argv[]);
DC::config parse_arguments(int argc, char **argv);

/**********************************************************************
 *  main game loop and related stuff                                  *
 **********************************************************************/

int main(int argc, char **argv)
{
  DC dcastle(parse_arguments(argc, argv));
  QThread::currentThread()->setObjectName("Main Thread");

  logentry(QString("Executable: %1 Version: %2 Build date: %3").arg(argv[0]).arg(DC::getBuildVersion()).arg(DC::getBuildTime()));
  backup_executable(argv);

  // If no ports specified then set default ports
  if (dcastle.cf.ports.size() == 0)
  {
    dcastle.cf.ports.push_back(DFLT_PORT);
    dcastle.cf.ports.push_back(DFLT_PORT2);
    dcastle.cf.allow_multi = true;
    dcastle.cf.ports.push_back(DFLT_PORT3);
    dcastle.cf.ports.push_back(DFLT_PORT4);
  }

  logentry(QString("Using %1 as data directory.").arg(dcastle.cf.library_directory));

  if (!QFile(dcastle.cf.library_directory).exists())
  {
    logentry(QString("Data directory %1 is missing.").arg(dcastle.cf.library_directory));
    exit(EXIT_FAILURE);
  }

  if (chdir(dcastle.cf.library_directory.toStdString().c_str()) < 0)
  {
    char strerror_buffer[512];
    const char *strerror_result = strerror_r(errno, strerror_buffer, sizeof(strerror_buffer));
    logentry(QString("Error changing current directory to %1: %2").arg(dcastle.cf.library_directory).arg(strerror_result));
    exit(EXIT_FAILURE);
  }

  if (dcastle.cf.check_syntax)
  {
    dcastle.boot_zones();
    dcastle.boot_world();
    logentry(QStringLiteral("Done."), 0, LogChannels::LOG_MISC);
    exit(EXIT_SUCCESS);
  }
  else
  {
    dcastle.init_game();
  }

  return 0;
}

void backup_executable(char *const argv[])
{
  // Make a copy of our executable so that in the event of a crash we have a
  // known good copy to debug with.
  QString backup_filename = QString("%1.%2.%3").arg(argv[0]).arg(DC::getBuildVersion()).arg(getpid());

  // If backup file does not exist already then link to it
  if (!QFile(backup_filename).exists())
  {
    if (link(argv[0], backup_filename.toStdString().c_str()) == -1)
    {
      char buffer[512];
      const char *strerror_result = strerror_r(errno, buffer, sizeof(buffer));
      logentry(QString("Error linking %1 to %2: %3").arg(argv[0]).arg(backup_filename).arg(strerror_result));
    }
  }
  return;
}

DC::config parse_arguments(int argc, char **argv)
{
  char opt;
  uint32_t port;
  DC::config cf(argc, argv);

  while ((opt = getopt(argc, argv, "tvp:Pmbd:shwcn?i:")) != -1)
  {
    switch (opt)
    {
    case 't':
      cf.testing = true;
    case 'i':
      cf.implementer = optarg;
      break;
    case 'v':
      cf.verbose_mode = true;
      break;
    case 'p':
      port = atoi(optarg);
      cf.ports.push_back(port);
      if (port == 6666)
      {
        cf.allow_multi = true;
      }
      break;
    case 'P':
      cf.allow_imp_password = true;
      break;
    case 'm':
      cf.test_mobs = 1;
      cf.test_objs = 1;
      logentry(QStringLiteral("Mud in testing mode. TinyTinyworld being used. (MOB,OBJ)"), 0, LogChannels::LOG_MISC);
      break;
    case 'n': // inhibits printing timeout on LogChannels::LOG_MISC messages normally sent to STDERR
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
      cf.library_directory = optarg;
      break;
    case 's':
      cf.check_syntax = true;
      break;
    case 'w':
      cf.test_world = 1;
      logentry(QStringLiteral("Mud in world checking mode. TinyTinyworld being used. (WLD)"), 0, LogChannels::LOG_MISC);
      logentry(QStringLiteral("Do NOT have mortals login when in world checking mode."), 0,
               LogChannels::LOG_MISC);
      break;
    case 'c':
      cf.test_objs = 1;
      logentry(QStringLiteral("Mud in testing mode. TinyTinyworld being used. (OBJ)"), 0, LogChannels::LOG_MISC);
      break;
    default:
    case 'h':
    case '?':
      fprintf(stderr, "Usage: %s [-t] [-v] [-h] [-w] [-c] [-m] [-d directory] [-p port#] [-P] [-i playername]\n"
                      "-t\t\tTesting mode\n"
                      "-v\t\tVerbose mode\n"
                      "-h\t\tUsage information\n"
                      "-w\t\tWorld testing mode\n"
                      "-c\t\tObj testing mode\n"
                      "-m\t\tObj & Mob testing mode\n"
                      "-n\t\tinhibits printing timestamps on STDOUT/STDERR\n"
                      "-b\t\tBuilders' port (7000-7003)\n"
                      "-d directory\tData directory\n"
                      "-p [port#]\tCan be repeated to listen on multiple ports\n"
                      "-P\t\tAllow imps to use their password\n"
                      "-s\t\tSyntax checking mode\n"
                      "-i playername\tset playername as level 110\n",
              argv[0]);

      exit(EXIT_FAILURE);
      break;
    }
  }

  return cf;
}
