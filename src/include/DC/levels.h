/* $Id: levels.h,v 1.9 2011/08/28 03:43:42 jhhudso Exp $ */
/* This is purely to define god levels as #defines. */
#ifndef LEVELS_H_
#define LEVELS_H_

#include "DC/structs.h"
#include <QString>

#define MORTAL 60
/* #define GLADIATOR */

// #define IMMORTAL 101
// #define ANGEL 102
// #define ARCHANGEL 103
// #define SERAPH 104
// #define DEITY 105
// #define PATRON 106
// #define POWER 107
// #define G_POWER 108
// #define OVERSEER 109
// #define IMPLEMENTER 110
// #define DC::MAX_MORTAL_LEVEL 50
// #define MIN_GOD IMMORTAL

#define GIFTED_COMMAND 101 // noone should ever "be" this level
const uint64_t IMMORTAL = 102;
#define ANGEL 103
#define DEITY 104
#define OVERSEER 105
#define DIVINITY 106
#define COORDINATOR 108
const uint64_t IMPLEMENTER = 110;

#define ARCHITECT ANGEL
#define ARCHANGEL ANGEL
#define SERAPH ANGEL
#define PATRON DEITY
#define POWER DEITY
#define G_POWER DEITY

#define MIN_GOD IMMORTAL

#define PIRAHNA_FAKE_LVL 102

#endif
