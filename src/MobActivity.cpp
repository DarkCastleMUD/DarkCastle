
#include <cctype>
#include <cstring>

#include "DC/structs.h"
#include "DC/room.h"
#include "DC/character.h"
#include "DC/DC.h"
#include "DC/utility.h"
#include "DC/terminal.h"
#include "DC/player.h"
#include "DC/mobile.h"
#include "DC/clan.h"
#include "DC/handler.h"
#include "DC/db.h" // exp_table
#include "DC/interp.h"
#include "DC/connect.h"
#include "DC/spells.h"
#include "DC/race.h"
#include "DC/act.h"
#include "DC/set.h"
#include "DC/returnvals.h"
#include "DC/fileinfo.h"
#include "DC/MobActivity.h"
#include "DC/const.h"
// Externs

// Locals
class Path *mPathList = nullptr;

/* PATHFINDING */

struct path_data *newPath()
{
  struct path_data *p;
#ifdef LEAK_CHECK
  p = (struct path_data *)calloc(1, sizeof(struct path_data));
#else
  p = (struct path_data *)dc_alloc(1, sizeof(struct path_data));
#endif
  return p;
}

bool Path::isRoomConnected(int room)
{
  struct path_data *p;
  int i;

  for (i = 0; i < MAX_DIRS; i++)
    if (DC::getInstance()->world[room].dir_option[i] && DC::getInstance()->world[room].dir_option[i]->to_room >= 0)
      for (p = DC::getInstance()->world[DC::getInstance()->world[room].dir_option[i]->to_room].paths; p; p = p->next)
        if (p->p == this)
          return true;

  return false;
}

bool Path::isRoomPathed(int room)
{
  struct path_data *p;

  for (p = DC::getInstance()->world[room].paths; p; p = p->next)
    if (p->p == this)
      return true;

  return false;
}

char *Path::determineRoute(Character *ch, int from, int to)
{
  int i;

  if (!isRoomPathed(from) || !isRoomPathed(to))
  {
    if (ch)
      ch->sendln("Error::determineBestRoute:: Room 'to' or 'from' is not connected to the path.");
    return nullptr;
  }
  i = 1000;
  resetPath();
  leastSteps(from, to, 1, &i);
  static char buf[MAX_STRING_LENGTH];
  buf[0] = 0;
  if (ch && ch->getLevel() >= 105)
    ch->send(QStringLiteral("# of steps: %1\r\n").arg(i));
  resetPath();
  findRoom(from, to, 1, i, &buf[0]);
  if (ch && ch->getLevel() >= 105)
    ch->send(QStringLiteral("Best route: %1\r\n").arg(buf));

  return &buf[0];
}

void Path::resetPath()
{
  for (std::map<int, int>::iterator iter = begin(); iter != end(); iter++)
    (*iter).second = 1000;
}

bool Path::findRoom(int from, int to, int steps, int leastSteps, char *buf)
{
  if (steps > leastSteps)
    return false; // Longer than the shortest path known. fuck it.
  if ((*this)[from] <= steps)
    return false; // already checked this room, circly paths

  (*this)[from] = steps;

  for (int i = 0; i < MAX_DIRS; i++)
  {
    if (!DC::getInstance()->world[from].dir_option[i])
      continue;
    if (DC::getInstance()->world[from].dir_option[i]->to_room == DC::NOWHERE)
      continue;
    if (!isRoomPathed(DC::getInstance()->world[from].dir_option[i]->to_room))
      continue;

    if (DC::getInstance()->world[from].dir_option[i]->to_room == to)
    {
      *buf = dirs[i][0];
      *(buf + 1) = '\0';
      return true;
    }

    if (findRoom(DC::getInstance()->world[from].dir_option[i]->to_room, to, steps + 1, leastSteps, buf + 1))
    {
      *buf = dirs[i][0];
      return true;
    }
  }
  return false;
}

int Path::leastSteps(int from, int to, int val, int *bestval)
{
  if (val > (*this)[from])
    return *bestval; // Already been here.
  (*this)[from] = val;

  for (int i = 0; i < MAX_DIRS; i++)
  {
    if (!DC::getInstance()->world[from].dir_option[i])
      continue;
    if (DC::getInstance()->world[from].dir_option[i]->to_room == DC::NOWHERE)
      continue;
    if (!isRoomPathed(DC::getInstance()->world[from].dir_option[i]->to_room))
      continue;

    if (DC::getInstance()->world[from].dir_option[i]->to_room == to)
    {
      if (val < *bestval)
        *bestval = val;
      return val;
    }
    else
      leastSteps(DC::getInstance()->world[from].dir_option[i]->to_room, to, val + 1, bestval);
  }
  return *bestval;
}

bool Path::isPathConnected(class Path *pa)
{
  struct path_data *t;
  for (t = p; t; t = t->next)
    if (t->p == pa)
      return true;

  return false;
}

void Path::addRoom(Character *ch, int room, bool IgnoreConnectingIssues)
{

  // Used at times with ch == nullptr

  if (!IgnoreConnectingIssues)
  {
    if (!isRoomConnected(room))
    {
      if (ch)
        ch->sendln("This room is does not connect to that path.");
      return;
    }
  }
  if (isRoomPathed(room))
  {
    if (ch)
      ch->sendln("This room is already connected to that path.");
    return;
  }
  struct path_data *pa;

  if (DC::getInstance()->world[room].paths)
  {
    struct path_data *t;
    for (pa = DC::getInstance()->world[room].paths; pa; pa = pa->next)
    {
      if (isPathConnected(pa->p))
      {
        struct path_data *t;
        for (t = this->p; t; t = t->next)
          if (t->p == pa->p)
            t->num++;
      }
      else
      {
        t = newPath();
        t->p = pa->p;
        t->num = 1;
        t->next = p;
        p = t;
      }

      if (pa->p->isPathConnected(this))
      {
        struct path_data *t;
        for (t = pa->p->p; t; t = t->next)
          if (t->p == this)
            t->num++;
      }
      else
      {
        t = newPath();
        t->p = this;
        t->num = 1;
        t->next = pa->p->p;
        pa->p->p = t;
      }
    }
  }

  pa = newPath();
  pa->p = this;
  pa->next = DC::getInstance()->world[room].paths;
  DC::getInstance()->world[room].paths = pa;
  (*this)[room] = 0;
  if (ch)
    ch->sendln("Room successfully added to path.");
}

int do_newPath(Character *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
    ch->sendln("Syntax: newPath <name of path>\r\nNote that the room you are currently in will automatically be added to the path.");
    return eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
      break;
  if (p)
  {
    ch->sendln("That path already exists.");
    return eFAILURE;
  }
  p = new Path;
  p->name = str_dup(arg1);
  p->addRoom(ch, ch->in_room, true);
  p->next = mPathList;
  mPathList = p;
  return eSUCCESS;
}

int do_listPathsByZone(Character *ch, char *argument, int cmd)
{
  auto &zones = DC::getInstance()->zones;
  int i = DC::getInstance()->world[ch->in_room].zone;
  if (zones.contains(i) == false)
  {
    return eFAILURE;
  }

  auto &zone = DC::getInstance()->zones[i];
  int low = zone.getRealBottom(), high = zone.getRealTop();

  class Path *p;
  bool found = false;
  for (p = mPathList; p; p = p->next)
    for (std::map<int, int>::iterator iter = p->begin(); iter != p->end(); iter++)
      if ((*iter).first >= low && (*iter).first <= high)
      {
        ch->send(QStringLiteral("Path '%1' connects to this zone.\r\n").arg(p->name));
        struct path_data *pa;
        for (pa = p->p; pa; pa = pa->next)
          csendf(ch, " --- Path '%s' connects to that path in %d places.\r\n",
                 pa->p->name, pa->num);
        found = true;
        break;
      }
  if (!found)
    ch->sendln("No paths connecting to this zone has been found.");

  return eSUCCESS;
}

int do_listAllPaths(Character *ch, char *argument, int cmd)
{
  class Path *p;
  bool found = false;
  for (p = mPathList; p; p = p->next)
  {
    ch->send(QStringLiteral("Path '%1'.\r\n").arg(p->name));
    struct path_data *pa;
    for (pa = p->p; pa; pa = pa->next)
      csendf(ch, " --- Path '%s' connects to that path in %d places.\r\n",
             pa->p->name, pa->num);
    found = true;
  }
  if (!found)
    ch->sendln("No paths found.");

  return eSUCCESS;
}

int do_addRoom(Character *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
    ch->sendln("Syntax: addRoom <name of path>\r\nNote that the room you are currently in will automatically be added to the path.");
    return eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
      break;
  if (!p)
  {
    ch->sendln("No such path exists.");
    return eFAILURE;
  }
  p->addRoom(ch, ch->in_room, false);
  return eSUCCESS;
}

int do_findPath(Character *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
    ch->sendln("Syntax: findPath <name of path> <start vnum> <end vnum>\r\nNote that the room you are currently in will automatically be added to the path.");
    return eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
      break;
  if (!p)
  {
    ch->sendln("No such path exists.");
    return eFAILURE;
  }
  int start, end;
  argument = one_argument(argument, arg1);

  if (!arg1[0] || !is_number(arg1))
  {
    do_findPath(ch, "", CMD_DEFAULT);
    return eFAILURE;
  }
  start = atoi(arg1);
  argument = one_argument(argument, arg1);

  if (!arg1[0] || !is_number(arg1))
  {
    do_findPath(ch, "", CMD_DEFAULT);
    return eFAILURE;
  }
  end = atoi(arg1);
  char *path = p->determineRoute(ch, start, end);

  if (!path)
    return eFAILURE;

  return eSUCCESS;
}

int leastPathSteps(class Path *goal, class Path *at, int steps, int *beststeps)
{
  if (at->s < steps)
    return *beststeps; // bad
  if (steps > *beststeps)
    return *beststeps;
  // Determine path
  at->s = steps;
  struct path_data *pt;
  for (pt = at->p; pt; pt = pt->next)
  {
    if (pt->p == goal)
    {
      *beststeps = steps;
      return steps;
    }
    leastPathSteps(goal, pt->p, steps + 1, beststeps);
  }

  return 0; // otherwise, warning: control reaches end of non-void function
}

bool determinePath(class Path *goal, class Path *at, int beststeps, int steps, class Path **end)
{
  if (at->s < steps)
    return false;
  if (steps > beststeps)
    return false;
  // Determine path
  at->s = steps;
  struct path_data *pt;
  for (pt = at->p; pt; pt = pt->next)
  {
    if (pt->p == goal)
    {
      *end = at;
      *(&end[1]) = goal;
      return true;
    }
    if (determinePath(goal, pt->p, beststeps, steps + 1, &end[1]))
    {
      *end = at;
      return true;
    }
  }
  return false;
}

int do_pathpath(Character *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  class Path *pt = nullptr, *pt2 = nullptr;
  for (pt = mPathList; pt; pt = pt->next)
    if (!str_cmp(pt->name, arg1))
      break;

  for (pt2 = mPathList; pt2; pt2 = pt2->next)
    if (!str_cmp(pt2->name, arg2))
      break;
  if (!pt || !pt2)
  {
    ch->sendln("Missing path.");
    return eFAILURE;
  }
  class Path *pa;
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

  // Find the least # of steps needed
  int i = 1000;
  leastPathSteps(pt, pt2, 1, &i);

  ch->send(QStringLiteral("Least # of steps: %1\r\n").arg(i));

  if (i >= 50)
  {
    ch->sendln("Crazy #. Stopping.");
    return eFAILURE;
  }

  class Path *p[50]; // Maximum of 50 pathsteps atm
  for (int z = 0; z < 50; z++)
    p[z] = 0;
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

  determinePath(pt, pt2, i, 1, &p[0]);
  for (int z = 0; p[z]; z++)
  {
    csendf(ch, "%s -- \r\n", p[z]->name);
  }
  return eSUCCESS;
}

int find_closest_path(int from, int steps, char *buf, std::map<int, int> z)
{
  if (steps > 5)
    return 0;

  z[from] = steps;
  int zenew;

  for (int i = 0; i < MAX_DIRS; i++)
  {
    if (!DC::getInstance()->world[from].dir_option[i])
      continue;
    if (DC::getInstance()->world[from].dir_option[i]->to_room == DC::NOWHERE)
      continue;
    if (z[DC::getInstance()->world[from].dir_option[i]->to_room] <= steps && z[DC::getInstance()->world[from].dir_option[i]->to_room] != 0)
      continue;

    if (DC::getInstance()->world[DC::getInstance()->world[from].dir_option[i]->to_room].paths)
    {
      *buf = *dirs[i];
      *(buf + 1) = '\0';
      return DC::getInstance()->world[from].dir_option[i]->to_room;
    }
    if ((zenew = find_closest_path(DC::getInstance()->world[from].dir_option[i]->to_room, steps + 1, buf + 1, z)) != 0)
    {
      *buf = *dirs[i];
      return zenew;
    }
  }
  return 0;
}

int Path::connectRoom(class Path *z)
{
  struct path_data *pa;

  for (std::map<int, int>::iterator iter = this->begin(); iter != this->end(); iter++)
    for (pa = DC::getInstance()->world[(*iter).first].paths; pa; pa = pa->next)
      if (pa->p == z)
        return (*iter).first;

  return 0;
}

char *findPath(int from, int to, Character *ch = nullptr)
{
  char buf[MAX_STRING_LENGTH];
  static char endbuf[MAX_STRING_LENGTH];
  endbuf[0] = buf[0] = '\0';
  class Path *start, *stop;
  if (DC::getInstance()->world[from].paths)
  {
    csendf(ch, "Starting from path %s.\r\n", DC::getInstance()->world[from].paths->p->name);
  }
  else
  {
    std::map<int, int> z;
    from = find_closest_path(from, 1, &buf[0], z);
    if (from && DC::getInstance()->world[from].paths)
      csendf(ch, "Starting from path %s.\r\n", DC::getInstance()->world[from].paths->p->name);
  }
  strcat(endbuf, buf);
  start = DC::getInstance()->world[from].paths->p;
  if (DC::getInstance()->world[to].paths)
  {
    csendf(ch, "Ending in path %s.\r\n", DC::getInstance()->world[to].paths->p->name);
  }
  else
  {
    std::map<int, int> z;
    to = find_closest_path(to, 1, &buf[0], z);
    if (to && DC::getInstance()->world[to].paths)
      csendf(ch, "Ending in path %s.\r\n", DC::getInstance()->world[to].paths->p->name);
  }
  stop = DC::getInstance()->world[to].paths->p;
  if (!start || !stop)
    return "Invalid path";

  class Path *pa;
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

  // Find the least # of steps needed
  int i = 1000;
  leastPathSteps(start, stop, 1, &i);

  ch->send(QStringLiteral("Least # of steps: %1\r\n").arg(i));

  if (i >= 50)
  {
    ch->sendln("Crazy #. Stopping.");
    return "Crazy #";
  }

  class Path *p[50]; // Maximum of 50 pathsteps atm
  for (int z = 0; z < 50; z++)
    p[z] = 0;
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

  determinePath(start, stop, i, 1, &p[0]);
  int endto = to;
  for (int z = 49; z >= 0; z--)
  {
    if (!p[z])
      continue;
    csendf(ch, "%s -- \r\n", p[z]->name);
    if (z > 0 && p[z - 1])
      to = p[z]->connectRoom(p[z - 1]);
    else
      to = endto;
    strcat(endbuf, p[z]->determineRoute(ch, from, to));
    from = to;
    //	char *Path::determineRoute(Character *ch, int from, int to)
  }
  return &endbuf[0];
}

int do_findpath(Character *ch, char *argument, int cmd)
{
  Path *p;
  for (p = mPathList; p; p = p->next)
    for (std::map<int, int>::iterator iter = p->begin(); iter != p->end(); iter++)
      csendf(ch, "Hmm: %d\r\n", (*iter).first);
  return eSUCCESS;
  /*  argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    int i = atoi(arg1), z = atoi(arg2);
    if (!i || !z) { ch->sendln("BLeh!"); return eFAILURE; }
    char *t =  findPath(i, z, ch);
    ch->send(QStringLiteral("Final Path: %1\r\n").arg(t));
    return eSUCCESS;
  */
}

void save_paths()
{ // mkay..
  Path *p;
  for (p = mPathList; p; p = p->next)
  {
    // Save pathname
    for (std::map<int, int>::iterator iter = p->begin(); iter != p->end(); iter++)
      ; // Save room #, iter.first()
  }
}

/* END PATHFINDING */
