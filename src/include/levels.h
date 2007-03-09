/* $Id: levels.h,v 1.8 2007/03/09 06:08:04 jhhudso Exp $ */
/* This is purely to define god levels as #defines. */
#ifndef LEVELS_H_
#define LEVELS_H_

#include "structs.h"

#define MORTAL 60
#define MAX_MORTAL 60
/* #define GLADIATOR */

//#define IMMORTAL 101
//#define ANGEL 102
//#define ARCHANGEL 103
//#define SERAPH 104
//#define DEITY 105
//#define PATRON 106
//#define POWER 107
//#define G_POWER 108
//#define OVERSEER 109
//#define IMP 110
//#define MAX_MORTAL 50
//#define MIN_GOD IMMORTAL

#define GIFTED_COMMAND 101 // noone should ever "be" this level
#define IMMORTAL 102
#define ANGEL 103
#define DEITY 104
#define OVERSEER 105
#define IMP 110

#define ARCHITECT ANGEL
#define ARCHANGEL ANGEL
#define SERAPH ANGEL
#define PATRON DEITY
#define POWER DEITY
#define G_POWER DEITY

#define MIN_GOD IMMORTAL

#define PIRAHNA_FAKE_LVL 102

struct bestowable_god_commands_type
{
  char * name;  // name of command
  int16 num;    // ID # of command
  bool testcmd; // true = test command, false = normal command
};

#endif
