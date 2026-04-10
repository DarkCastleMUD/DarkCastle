
#include <cstring>

#include "DC/structs.h"

#include "DC/DC.h"

#include "DC/interp.h"
#include "DC/returnvals.h"
#include "DC/const.h"

// Externs

// Locals
class Path *mPathList = {};

/* PATHFINDING */

QSharedPointer<path_data> newPath(void)
{
  return QSharedPointer<path_data>::create();
}

bool Path::isRoomConnected(qint32 room)
{
  path_data *p;
  qint32 i;

  for (i = {}; i < MAX_DIRS; i++)
    if (DC::getInstance()->world[room].dir_option[i] && DC::getInstance()->world[room].dir_option[i]->to_room >= 0)
      for (p = DC::getInstance()->world[DC::getInstance()->world[room].dir_option[i]->to_room].paths; p; p = p->next)
        if (p->p == this)
          return true;

  return false;
}

bool Path::isRoomPathed(qint32 room)
{
  path_data *p;

  for (p = DC::getInstance()->world[room].paths; p; p = p->next)
    if (p->p == this)
      return true;

  return false;
}

QString Path::determineRoute(CharacterPtr ch, qint32 from, qint32 to)
{
  qint32 i;

  if (!isRoomPathed(from) || !isRoomPathed(to))
  {
    if (ch)
      ch->sendln("Error::determineBestRoute:: Room 'to' or 'from' is not connected to the path.");
    return {};
  }
  i = 1000;
  resetPath();
  leastSteps(from, to, 1, &i);
  static QString buf;
  buf[0] = {};
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
  for (QMap<qint32, qint32>::iterator iter = begin(); iter != end(); iter++)
    (*iter).second = 1000;
}

bool Path::findRoom(qint32 from, qint32 to, qint32 steps, qint32 leastSteps, QString buf)
{
  if (steps > leastSteps)
    return false; // Longer than the shortest path known. fuck it.
  if ((*this)[from] <= steps)
    return false; // already checked this room, circly paths

  (*this)[from] = steps;

  for (qint32 i = {}; i < MAX_DIRS; i++)
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

qint32 Path::leastSteps(qint32 from, qint32 to, qint32 val, qint32 *bestval)
{
  if (val > (*this)[from])
    return *bestval; // Already been here.
  (*this)[from] = val;

  for (qint32 i = {}; i < MAX_DIRS; i++)
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
  path_data *t;
  for (t = p; t; t = t->next)
    if (t->p == pa)
      return true;

  return false;
}

void Path::addRoom(CharacterPtr ch, qint32 room, bool IgnoreConnectingIssues)
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
  path_data *pa;

  if (DC::getInstance()->world[room].paths)
  {
    path_data *t;
    for (pa = DC::getInstance()->world[room].paths; pa; pa = pa->next)
    {
      if (isPathConnected(pa->p))
      {
        path_data *t;
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
        path_data *t;
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
  (*this)[room] = {};
  if (ch)
    ch->sendln("Room successfully added to path.");
}

command_return_t do_newPath(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
    ch->sendln("Syntax: newPath <name of path>\r\nNote that the room you are currently in will automatically be added to the path.");
    return ReturnValue::eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
      break;
  if (p)
  {
    ch->sendln("That path already exists.");
    return ReturnValue::eFAILURE;
  }
  p = new Path;
  p->name = (arg1);
  p->addRoom(ch, ch->in_room, true);
  p->next = mPathList;
  mPathList = p;
  return ReturnValue::eSUCCESS;
}

command_return_t do_listPathsByZone(CharacterPtr ch, QString argument, cmd_t cmd)
{
  auto &zones = DC::getInstance()->zones;
  qint32 i = DC::getInstance()->world[ch->in_room].zone;
  if (zones.contains(i) == false)
  {
    return ReturnValue::eFAILURE;
  }

  auto &zone = DC::getInstance()->zones[i];
  qint32 low = zone.getRealBottom(), high = zone.getRealTop();

  class Path *p;
  bool found = false;
  for (p = mPathList; p; p = p->next)
    for (QMap<qint32, qint32>::iterator iter = p->begin(); iter != p->end(); iter++)
      if ((*iter).first >= low && (*iter).first <= high)
      {
        ch->send(QStringLiteral("Path '%1' connects to this zone.\r\n").arg(p->name));
        path_data *pa;
        for (pa = p->p; pa; pa = pa->next)
          ch->send(QStringLiteral(" --- Path '%s' connects to that path in %d places.\r\n").arg(pa->p->name).arg(pa->num));
        found = true;
        break;
      }
  if (!found)
    ch->sendln("No paths connecting to this zone has been found.");

  return ReturnValue::eSUCCESS;
}

command_return_t do_listAllPaths(CharacterPtr ch, QString argument, cmd_t cmd)
{
  class Path *p;
  bool found = false;
  for (p = mPathList; p; p = p->next)
  {
    ch->send(QStringLiteral("Path '%1'.\r\n").arg(p->name));
    path_data *pa;
    for (pa = p->p; pa; pa = pa->next)
      ch->send(QStringLiteral(" --- Path '%s' connects to that path in %d places.\r\n").arg(pa->p->name).arg(pa->num));
    found = true;
  }
  if (!found)
    ch->sendln("No paths found.");

  return ReturnValue::eSUCCESS;
}

command_return_t do_addRoom(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
    ch->sendln("Syntax: addRoom <name of path>\r\nNote that the room you are currently in will automatically be added to the path.");
    return ReturnValue::eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
      break;
  if (!p)
  {
    ch->sendln("No such path exists.");
    return ReturnValue::eFAILURE;
  }
  p->addRoom(ch, ch->in_room, false);
  return ReturnValue::eSUCCESS;
}

command_return_t do_findPath(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
    ch->sendln("Syntax: findPath <name of path> <start vnum> <end vnum>\r\nNote that the room you are currently in will automatically be added to the path.");
    return ReturnValue::eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
      break;
  if (!p)
  {
    ch->sendln("No such path exists.");
    return ReturnValue::eFAILURE;
  }
  qint32 start, end;
  argument = one_argument(argument, arg1);

  if (!arg1[0] || !is_number(arg1))
  {
    do_findPath(ch, "");
    return ReturnValue::eFAILURE;
  }
  start = atoi(arg1);
  argument = one_argument(argument, arg1);

  if (!arg1[0] || !is_number(arg1))
  {
    do_findPath(ch, "");
    return ReturnValue::eFAILURE;
  }
  end = atoi(arg1);
  QString path = p->determineRoute(ch, start, end);

  if (!path)
    return ReturnValue::eFAILURE;

  return ReturnValue::eSUCCESS;
}

qint32 leastPathSteps(class Path *goal, class Path *at, qint32 steps, qint32 *beststeps)
{
  if (at->s < steps)
    return *beststeps; // bad
  if (steps > *beststeps)
    return *beststeps;
  // Determine path
  at->s = steps;
  path_data *pt;
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

bool determinePath(class Path *goal, class Path *at, qint32 beststeps, qint32 steps, class Path **end)
{
  if (at->s < steps)
    return false;
  if (steps > beststeps)
    return false;
  // Determine path
  at->s = steps;
  path_data *pt;
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

command_return_t do_pathpath(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1, arg2[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  class Path *pt = {}, *pt2 = {};
  for (pt = mPathList; pt; pt = pt->next)
    if (!str_cmp(pt->name, arg1))
      break;

  for (pt2 = mPathList; pt2; pt2 = pt2->next)
    if (!str_cmp(pt2->name, arg2))
      break;
  if (!pt || !pt2)
  {
    ch->sendln("Missing path.");
    return ReturnValue::eFAILURE;
  }
  class Path *pa;
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

  // Find the least # of steps needed
  qint32 i = 1000;
  leastPathSteps(pt, pt2, 1, &i);

  ch->send(QStringLiteral("Least # of steps: %1\r\n").arg(i));

  if (i >= 50)
  {
    ch->sendln("Crazy #. Stopping.");
    return ReturnValue::eFAILURE;
  }

  class Path *p[50]; // Maximum of 50 pathsteps atm
  for (qint32 z = {}; z < 50; z++)
    p[z] = {};
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

  determinePath(pt, pt2, i, 1, &p[0]);
  for (qint32 z = {}; p[z]; z++)
  {
    ch->sendln(QStringLiteral("%1 -- ").arg(p[z]->name));
  }
  return ReturnValue::eSUCCESS;
}

qint32 find_closest_path(qint32 from, qint32 steps, QString buf, QMap<qint32, qint32> z)
{
  if (steps > 5)
    return 0;

  z[from] = steps;
  qint32 zenew;

  for (qint32 i = {}; i < MAX_DIRS; i++)
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

qint32 Path::connectRoom(class Path *z)
{
  path_data *pa;

  for (QMap<qint32, qint32>::iterator iter = this->begin(); iter != this->end(); iter++)
    for (pa = DC::getInstance()->world[(*iter).first].paths; pa; pa = pa->next)
      if (pa->p == z)
        return (*iter).first;

  return 0;
}

QString findPath(qint32 from, qint32 to, CharacterPtr ch = {})
{
  QString buf;
  static QString endbuf;
  endbuf[0] = buf[0] = '\0';
  class Path *start, *stop;
  if (DC::getInstance()->world[from].paths)
  {
    ch->send(QStringLiteral("Starting from path %s.\r\n").arg(DC::getInstance()->world[from].paths->p->name));
  }
  else
  {
    QMap<qint32, qint32> z;
    from = find_closest_path(from, 1, &buf[0], z);
    if (from && DC::getInstance()->world[from].paths)
      ch->send(QStringLiteral("Starting from path %s.\r\n").arg(DC::getInstance()->world[from].paths->p->name));
  }
  strcat(endbuf, buf);
  start = DC::getInstance()->world[from].paths->p;
  if (DC::getInstance()->world[to].paths)
  {
    ch->send(QStringLiteral("Ending in path %s.\r\n").arg(DC::getInstance()->world[to].paths->p->name));
  }
  else
  {
    QMap<qint32, qint32> z;
    to = find_closest_path(to, 1, &buf[0], z);
    if (to && DC::getInstance()->world[to].paths)
      ch->send(QStringLiteral("Ending in path %s.\r\n").arg(DC::getInstance()->world[to].paths->p->name));
  }
  stop = DC::getInstance()->world[to].paths->p;
  if (!start || !stop)
    return "Invalid path";

  class Path *pa;
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

  // Find the least # of steps needed
  qint32 i = 1000;
  leastPathSteps(start, stop, 1, &i);

  ch->send(QStringLiteral("Least # of steps: %1\r\n").arg(i));

  if (i >= 50)
  {
    ch->sendln("Crazy #. Stopping.");
    return "Crazy #";
  }

  class Path *p[50]; // Maximum of 50 pathsteps atm
  for (qint32 z = {}; z < 50; z++)
    p[z] = {};
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

  determinePath(start, stop, i, 1, &p[0]);
  qint32 endto = to;
  for (qint32 z = 49; z >= 0; z--)
  {
    if (!p[z])
      continue;
    ch->sendln(QStringLiteral("%1 -- ").arg(p[z]->name));
    if (z > 0 && p[z - 1])
      to = p[z]->connectRoom(p[z - 1]);
    else
      to = endto;
    strcat(endbuf, p[z]->determineRoute(ch, from, to));
    from = to;
    //	QString Path::determineRoute(CharacterPtr ch, qint32 from, qint32 to)
  }
  return &endbuf[0];
}

command_return_t do_findpath(CharacterPtr ch, QString argument, cmd_t cmd)
{
  Path *p;
  for (p = mPathList; p; p = p->next)
    for (QMap<qint32, qint32>::iterator iter = p->begin(); iter != p->end(); iter++)
      ch->send(QStringLiteral("Hmm: %d\r\n").arg((*iter).first));
  return ReturnValue::eSUCCESS;
  /*  argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    qint32 i = atoi(arg1), z = atoi(arg2);
    if (!i || !z) { ch->sendln("BLeh!"); return ReturnValue::eFAILURE; }
    QString t =  findPath(i, z, ch);
    ch->send(QStringLiteral("Final Path: %1\r\n").arg(t));
    return ReturnValue::eSUCCESS;
  */
}

void save_paths()
{ // mkay..
  Path *p;
  for (p = mPathList; p; p = p->next)
  {
    // Save pathname
    for (QMap<qint32, qint32>::iterator iter = p->begin(); iter != p->end(); iter++)
      ; // Save room #, iter.first()
  }
}

/* END PATHFINDING */
