#include "levels.h"

const bestowable_god_commands_type bestowable_god_commands[] =
{
{ "impchan",	COMMAND_IMP_CHAN, false },
{ "snoop",	COMMAND_SNOOP, false },
{ "restore",	COMMAND_RESTORE, false },
{ "purloin",	COMMAND_PURLOIN, false },
{ "possess",	COMMAND_POSSESS, false},
{ "arena",	COMMAND_ARENA, false },
{ "set",	COMMAND_SET, false },
{ "load",	COMMAND_LOAD, false },
{ "shutdown",   COMMAND_SHUTDOWN, false },
{ "procedit",	COMMAND_MP_EDIT, false },
{ "range",      COMMAND_RANGE, false },
{ "procstat",	COMMAND_MPSTAT, false },
{ "sedit",      COMMAND_SEDIT, false },
{ "punish",     COMMAND_PUNISH, false },
{ "sqedit",     COMMAND_SQEDIT, false },
{ "hedit",      COMMAND_HEDIT, false },
{ "opstat",	COMMAND_OPSTAT, false },
{ "opedit",	COMMAND_OPEDIT, false },
{ "force",	COMMAND_FORCE, false },
{ "string",	COMMAND_STRING, false },
{ "stat",	COMMAND_STAT, false },
{ "sqsave",	COMMAND_SQSAVE, false },
{ "find",	COMMAND_FIND, false },
{ "log",	COMMAND_LOG, false },
{ "addnews",	COMMAND_ADDNEWS, false },
{ "prize",	COMMAND_PRIZE, false },
{ "sockets",	COMMAND_SOCKETS, false },
{ "qedit",	COMMAND_QEDIT, false },
{ "rename",	COMMAND_RENAME, false },
{ "findpath",   COMMAND_FINDPATH, true },
{ "findpath2",  COMMAND_FINDPATH2, true },
{ "addroom",    COMMAND_ADDROOM, true },
{ "newpath",    COMMAND_NEWPATH, true },
{ "listpathsbyzone", COMMAND_LISTPATHSBYZONE, true },
{ "listallpaths",    COMMAND_LISTALLPATHS, true },
{ "testhand",   COMMAND_TESTHAND, true },
{ "dopathpath", COMMAND_DOPATHPATH, true },
{ "testport", COMMAND_TESTPORT, false },
{ "testuser", COMMAND_TESTUSER, false },
{ "remort", COMMAND_REMORT, true },
{ "testhit", COMMAND_TESTHIT, true },
{ "\n",		-1 }
};

// WEAR, ITEM_WEAR correspondances
const int wear_corr[] =
{
  ITEM_LIGHT_SOURCE, //0
  ITEM_WEAR_FINGER,
  ITEM_WEAR_FINGER,
  ITEM_WEAR_NECK,
  ITEM_WEAR_NECK,
  ITEM_WEAR_BODY, // 5
  ITEM_WEAR_HEAD,
  ITEM_WEAR_LEGS,
  ITEM_WEAR_FEET,
  ITEM_WEAR_HANDS, 
  ITEM_WEAR_ARMS, // 10
  ITEM_WEAR_SHIELD,
  ITEM_WEAR_ABOUT,
  ITEM_WEAR_WAISTE,
  ITEM_WEAR_WRIST,
  ITEM_WEAR_WRIST, //15
  ITEM_WIELD,
  ITEM_WIELD,
  ITEM_HOLD,
  ITEM_HOLD,
  ITEM_WEAR_FACE,//20
  ITEM_WEAR_EAR, 
  ITEM_WEAR_EAR,
  0
};

