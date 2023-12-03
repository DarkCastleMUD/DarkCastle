#ifndef __MOBACTIVITYH__
#define __MOBACTIVITYH__
/*
  New DC Mob AI
*/

#include <map>
#include "character.h"


/*			 PATHFINDING 				*/
/*			 PATHFINDING 				*/
/*			 PATHFINDING 				*/
struct path_data
{ // Keeps track of paths connecting to a room or path.
  class Path *p;
  struct path_data *next;
  int num;
};

class Path : public std::map<int, int>
{
private:
  bool findRoom(int from, int to, int steps, int leaststeps, char *buf);
  void resetPath();
  int leastSteps(int from, int to, int val, int *bestval);

public:
  class Path *next; // main Path list

  char *determineRoute(Character *, int, int); // ch, from, to
  void addRoom(Character *, int, bool);        // ch, room, IgnoreConnectingIssues

  bool isRoomPathed(int room);
  bool isRoomConnected(int room);
  bool isPathConnected(class Path *pa);
  int connectRoom(class Path *);
  struct path_data *p;
  char *name;
  int s;
  Path() : next(nullptr), p(nullptr), name(nullptr), s(0) {}
};

/*			 END PATHFINDING 			*/
/*			 END PATHFINDING 			*/
/*			 END PATHFINDING 			*/
#endif
