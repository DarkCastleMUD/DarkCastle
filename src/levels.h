/* $Id: levels.h,v 1.9 2011/08/28 03:43:42 jhhudso Exp $ */
/* This is purely to define god levels as #defines. */
#ifndef LEVELS_H_
#define LEVELS_H_

#include "structs.h"

#define MORTAL 60
#define MAX_MORTAL 60
/* #define GLADIATOR */

// #define IMMORTAL 101
// #define ARCHITECT 102
// #define ARCHITECT 103
// #define ARCHITECT 104
// #define DEITY 105
// #define DEITY 106
// #define DEITY 107
// #define DEITY 108
// #define OVERSEER 109
// #define IMPLEMENTER 110
// #define MAX_MORTAL 50
// #define IMMORTAL IMMORTAL

constexpr level_t GIFTED_COMMAND = 101; // noone should ever "be" this level
constexpr level_t IMMORTAL = 102;
constexpr level_t ARCHITECT = 103;
constexpr level_t DEITY = 104;
constexpr level_t OVERSEER = 105;
constexpr level_t COORDINATOR = 108;
constexpr level_t IMPLEMENTER = 110;

struct bestowable_god_commands_type
{
  char *name;   // name of command
  int16_t num;  // ID # of command
  bool testcmd; // true = test command, false = normal command
};

#endif
