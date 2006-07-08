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

extern CWorld world;
extern zone_data *zone_table;


class Path *mPathList = NULL;


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
  char buf[MAX_STRING_LENGTH];
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
  if (steps > leastSteps) return FALSE;
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

bool Path::isPathConnected(struct path_data *pa)
{
  struct path_data *t;
  for (t = p; t; t = t->next)
     if (t->p == pa->p) return TRUE;

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
     for (pa = world[room].paths; pa; pa = p->next)
     {
	if (isPathConnected(pa))
	{
		pa->num++;
		continue;
	}
	t = newPath();
	t->p = pa->p;
	t->num = 1;
	t->next = p;
	p = t;
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
