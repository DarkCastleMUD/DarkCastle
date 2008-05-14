extern "C"
{
#include <ctype.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <structs.h>
#include <room.h>
#include <character.h>
#include <obj.h>
#include <utility.h>
#include <terminal.h>
#include <player.h>
#include <levels.h>
#include <mobile.h>
#include <clan.h>
#include <handler.h>
#include <db.h> // exp_table
#include <interp.h>
#include <connect.h>
#include <spells.h>
#include <race.h>
#include <act.h>
#include <set.h>
#include <returnvals.h>
#include <fileinfo.h>
#include <MobActivity.h>

// Externs
extern CWorld world;
extern zone_data *zone_table;


// Locals
class Path *mPathList = NULL;


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
    if (world[room].dir_option[i] && world[room].dir_option[i]->to_room >= 0)
      for (p = world[world[room].dir_option[i]->to_room].paths; p; p = p->next)
        if (p->p == this) return TRUE;

  return FALSE;
}

bool Path::isRoomPathed(int room)
{
  struct path_data *p;

  for (p = world[room].paths; p; p = p->next)
    if (p->p == this) return TRUE;

  return FALSE;
}


char *Path::determineRoute(char_data *ch, int from, int to)
{
  int i;

  if (!isRoomPathed(from) || !isRoomPathed(to))
  {
    if (ch) send_to_char("Error::determineBestRoute:: Room 'to' or 'from' is not connected to the path.\r\n",ch);
    return NULL;
  }
  i = 1000;
  resetPath();
  leastSteps(from, to, 1, &i);
  static char buf[MAX_STRING_LENGTH];
  buf[0] = 0;
  if (ch && GET_LEVEL(ch) >= 105)
    csendf(ch, "# of steps: %d\r\n",i);
  resetPath();
  findRoom(from, to, 1, i, &buf[0]);
  if (ch && GET_LEVEL(ch) >= 105)
    csendf(ch, "Best route: %s\r\n",buf);

  return &buf[0];
}


void Path::resetPath()
{
  for( map<int, int>::iterator iter = begin(); iter != end(); iter++ )
    (*iter).second = 1000;
}

extern const char *dirs[];
extern int rev_dir[];

bool Path::findRoom(int from, int to, int steps, int leastSteps, char *buf)
{
  if (steps > leastSteps) return FALSE; // Longer than the shortest path known. fuck it.
  if ((*this)[from] <= steps) return FALSE; // already checked this room, circly paths

  (*this)[from] = steps;

  for (int i = 0; i < MAX_DIRS; i++)
  {
    if (!world[from].dir_option[i]) continue;
    if (world[from].dir_option[i]->to_room < 0) continue;
    if (!isRoomPathed(world[from].dir_option[i]->to_room)) continue;

    if (world[from].dir_option[i]->to_room == to) { *buf = dirs[i][0]; *(buf+1) = '\0'; return TRUE; }
    
    if (findRoom(world[from].dir_option[i]->to_room, to, steps+1, leastSteps, buf+1)) 
    {
       *buf = dirs[i][0];
       return TRUE;
    }
  }
  return FALSE;
}

int Path::leastSteps(int from, int to, int val, int *bestval)
{
  if (val > (*this)[from]) return *bestval; // Already been here.
  (*this)[from] = val;
  
  for (int i = 0; i < MAX_DIRS; i++)
  {
    if (!world[from].dir_option[i]) continue;
    if (world[from].dir_option[i]->to_room < 0) continue;
    if (!isRoomPathed(world[from].dir_option[i]->to_room)) continue;

    if (world[from].dir_option[i]->to_room == to) 
	{
	   if (val < *bestval) *bestval = val;
  	   return val;
	} else
    leastSteps(world[from].dir_option[i]->to_room, to, val+1, bestval);
  }
  return *bestval;
}

bool Path::isPathConnected(class Path *pa)
{
  struct path_data *t;
  for (t = p; t; t = t->next)
     if (t->p == pa) return TRUE;

  return FALSE;
}

void Path::addRoom(char_data *ch, int room, bool IgnoreConnectingIssues)
{

 // Used at times with ch == NULL

  if (!IgnoreConnectingIssues)
  {
    if (!isRoomConnected(room))
    {
	if (ch) send_to_char("This room is does not connect to that path.\r\n",ch);
	return;
    }
  }
  if (isRoomPathed(room))
  {
     if (ch) send_to_char("This room is already connected to that path.\r\n",ch);
     return;
  }
  struct path_data *pa;

  if (world[room].paths)
  {
     struct path_data *t;
     for (pa = world[room].paths; pa; pa = pa->next)
     {
	if (isPathConnected(pa->p))
	{
	  struct path_data *t;
	  for (t = this->p; t; t = t->next)
	   if (t->p == pa->p) t->num++;
	} else {
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
	   if (t->p == this) t->num++;
	} else {
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
  pa->next = world[room].paths;
  world[room].paths = pa;
  (*this)[room] = 0;
  if (ch) send_to_char("Room successfully added to path.\r\n",ch);
}


int do_newPath(char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
	send_to_char("Syntax: newPath <name of path>\r\nNote that the room you are currently in will automatically be added to the path.\r\n",ch);
	return eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
     break;
  if (p)
  {
	send_to_char("That path already exists.\r\n",ch);
	return eFAILURE;
  }
  p = new Path;
  p->name = str_dup(arg1);
  p->addRoom(ch, ch->in_room, TRUE);
  p->next = mPathList;
  mPathList = p;
  return eSUCCESS;
}

int do_listPathsByZone(char_data *ch, char *argument, int cmd)
{
  int i = world[ch->in_room].zone, low = zone_table[i].bottom_rnum, high = zone_table[i].top_rnum;

  class Path *p;
  bool found = FALSE;
  for (p = mPathList; p; p = p->next)
    for( map<int, int>::iterator iter = p->begin(); iter != p->end(); iter++ )
	if ((*iter).first >= low && (*iter).first <= high)
	{
	  csendf(ch, "Path '%s' connects to this zone.\r\n",p->name);
	    struct path_data *pa;
	    for (pa = p->p; pa; pa = pa->next)
		csendf(ch, " --- Path '%s' connects to that path in %d places.\r\n",
			pa->p->name, pa->num);
	  found = TRUE;
	  break;
	}
  if (!found)
    send_to_char("No paths connecting to this zone has been found.\r\n",ch);

  return eSUCCESS;
}

int do_listAllPaths(char_data *ch, char *argument, int cmd)
{
  class Path *p;
  bool found = FALSE;
  for (p = mPathList; p; p = p->next)
  {
    csendf(ch, "Path '%s'.\r\n",p->name);
    struct path_data *pa;
    for (pa = p->p; pa; pa = pa->next)
	csendf(ch, " --- Path '%s' connects to that path in %d places.\r\n",
		pa->p->name, pa->num);
    found = TRUE;

  }
  if (!found)
    send_to_char("No paths found.\r\n",ch);

  return eSUCCESS;
}

int do_addRoom(char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
	send_to_char("Syntax: addRoom <name of path>\r\nNote that the room you are currently in will automatically be added to the path.\r\n",ch);
	return eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
     break;
  if (!p)
  {
	send_to_char("No such path exists.\r\n",ch);
	return eFAILURE;
  }
  p->addRoom(ch, ch->in_room, FALSE);
  return eSUCCESS;
}

int do_findPath(char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  if (!arg1[0])
  {
	send_to_char("Syntax: findPath <name of path> <start vnum> <end vnum>\r\nNote that the room you are currently in will automatically be added to the path.\r\n",ch);
	return eFAILURE;
  }
  class Path *p;
  for (p = mPathList; p; p = p->next)
    if (!str_cmp(p->name, arg1))
     break;
  if (!p)
  {
	send_to_char("No such path exists.\r\n",ch);
	return eFAILURE;
  }
  int start, end;
  argument = one_argument(argument, arg1);

  if (!arg1[0] || !is_number(arg1)) {do_findPath(ch, "", 9); return eFAILURE; }
  start = atoi(arg1);
  argument = one_argument(argument, arg1);

  if (!arg1[0] || !is_number(arg1)) {do_findPath(ch, "", 9); return eFAILURE; }
  end = atoi(arg1);
  char *path = p->determineRoute(ch, start, end);

  if (!path) return eFAILURE;

  return eSUCCESS;
}

int leastPathSteps(class Path *goal, class Path *at, int steps, int *beststeps)
{
 if (at->s < steps) return *beststeps; // bad
 if (steps > *beststeps) return *beststeps;
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
    leastPathSteps(goal, pt->p, steps+1, beststeps);
 }

 return 0; //otherwise, warning: control reaches end of non-void function
}

bool determinePath(class Path *goal, class Path *at, int beststeps, int steps, class Path **end)
{
 if (at->s < steps) return FALSE;
 if (steps > beststeps) return FALSE;
 // Determine path
 at->s = steps;
 struct path_data *pt;
 for (pt = at->p; pt; pt = pt->next)
 {
    if (pt->p == goal)
    {
     *end = at;
     *(&end[1]) = goal;
      return TRUE;
    }
    if (determinePath(goal, pt->p, beststeps, steps+1, &end[1]))
    {  *end = at; return TRUE; }
 }
 return FALSE;
}


int do_pathpath(char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  class Path *pt = NULL, *pt2 = NULL;
  for (pt = mPathList;pt;pt = pt->next)
    if (!str_cmp(pt->name, arg1)) break;

  for (pt2 = mPathList;pt2;pt2 = pt2->next)
    if (!str_cmp(pt2->name, arg2)) break;
  if (!pt || !pt2)
  {
    send_to_char("Missing path.\r\n",ch);
    return eFAILURE;
  }
  class Path *pa;
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

 // Find the least # of steps needed
  int i = 1000;
  leastPathSteps(pt, pt2, 1, &i);

  csendf(ch, "Least # of steps: %d\r\n", i);

  if (i >= 50)
  {
    send_to_char("Crazy #. Stopping.\r\n",ch);
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
	csendf(ch, "%s -- \r\n",p[z]->name);
  }
  return eSUCCESS;
}


int find_closest_path(int from, int steps, char *buf, map<int,int> z)
{
  if (steps > 5) return 0;

  z[from] = steps;
  int zenew;

  for (int i = 0; i < MAX_DIRS; i++)
  {
    if (!world[from].dir_option[i]) continue;
    if (world[from].dir_option[i]->to_room < 0) continue;
    if (z[world[from].dir_option[i]->to_room] <= steps
	&& z[world[from].dir_option[i]->to_room] != 0) continue;

    if (world[world[from].dir_option[i]->to_room].paths)
    {
	   *buf = *dirs[i];
	   *(buf+1) = '\0';
	   return world[from].dir_option[i]->to_room;
    }
    if ((zenew = find_closest_path(world[from].dir_option[i]->to_room, steps+1, buf+1, z)) != 0)
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

  for( map<int, int>::iterator iter = this->begin(); iter != this->end(); iter++ )
    for (pa = world[(*iter).first].paths; pa; pa = pa->next)
	if (pa->p == z) return (*iter).first;

  return 0;
}


char *findPath(int from, int to, char_data *ch = NULL)
{
  char buf[MAX_STRING_LENGTH];
  static char endbuf[MAX_STRING_LENGTH];
  endbuf[0] = buf[0] = '\0';
  class Path *start,*stop;  
  if (world[from].paths)
  {
     csendf(ch, "Starting from path %s.\r\n", world[from].paths->p->name);
  } 
  else 
  {
    map<int,int> z; 
    from = find_closest_path(from, 1, &buf[0], z);
    if (from && world[from].paths)
    csendf(ch, "Starting from path %s.\r\n", world[from].paths->p->name);
  }
  strcat(endbuf, buf);
  start = world[from].paths->p;
  if (world[to].paths)
  {
     csendf(ch, "Ending in path %s.\r\n", world[to].paths->p->name);
  } 
  else 
  {
    map<int,int> z; 
    to = find_closest_path(to, 1, &buf[0], z);
    if (to && world[to].paths)
    csendf(ch, "Ending in path %s.\r\n", world[to].paths->p->name);
  }
  stop = world[to].paths->p;
  if (!start || !stop)
    return "Invalid path";

  class Path *pa;
  for (pa = mPathList; pa; pa = pa->next)
    pa->s = 1000;

 // Find the least # of steps needed
  int i = 1000;
  leastPathSteps(start, stop, 1, &i);

  csendf(ch, "Least # of steps: %d\r\n", i);

  if (i >= 50)
  {
    send_to_char("Crazy #. Stopping.\r\n",ch);
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
	if (!p[z]) continue;
	csendf(ch, "%s -- \r\n",p[z]->name);
	if (z>0 && p[z-1])
  	  to = p[z]->connectRoom(p[z-1]);
        else to = endto;
	strcat(endbuf, p[z]->determineRoute(ch, from, to));
	from = to;
//	char *Path::determineRoute(char_data *ch, int from, int to)
        
  }
  return &endbuf[0];
  
}

int do_findpath(char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  class Path *p;
  for (p = mPathList; p; p = p->next)
    for( map<int, int>::iterator iter = p->begin(); iter != p->end(); iter++ )
	csendf(ch, "Hmm: %d\r\n", (*iter).first);
 return eSUCCESS;
/*  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  int i = atoi(arg1), z = atoi(arg2);
  if (!i || !z) { send_to_char("BLeh!\r\n",ch); return eFAILURE; }
  char *t =  findPath(i, z, ch);
  csendf(ch, "Final Path: %s\r\n",t);
  return eSUCCESS;
*/
}

void save_paths()
{ //mkay..
  class Path *p;
  bool found = FALSE;
  for (p = mPathList;p; p=p->next)
  {
     // Save pathname
     for (map<int,int>::iterator iter = p->begin(); iter != p->end(); iter++)
       ; // Save room #, iter.first()
  }


}



/* END PATHFINDING */
