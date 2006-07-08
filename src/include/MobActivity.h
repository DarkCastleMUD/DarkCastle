#ifndef __MOBACTIVITYH__
#define __MOBACTIVITYH__
/*
  New DC Mob AI
*/

#include <map>
using namespace std;


/*			 PATHFINDING 				*/
/*			 PATHFINDING 				*/
/*			 PATHFINDING 				*/
struct path_data
{ // Keeps track of paths connecting to a room or path.
   class Path *p;
   struct path_data *next;
   int num;
};

class Path : public map<int, int>
{
  private:
    bool findRoom(int from, int to, int steps, int leaststeps, char *buf);
    void resetPath();
    int leastSteps(int from, int to, int val, int *bestval);
  public:
    class Path *next;		       // main Path list

    char* determineRoute(char_data *, int, int); // ch, from, to
    void addRoom(char_data *, int, bool);           // ch, room, IgnoreConnectingIssues
    
    bool isRoomPathed(int room);
    bool isRoomConnected(int room);
    bool isPathConnected(struct path_data *pa);
    struct path_data *p;
    char *name;
    Path() { p = 0; name = 0; }
};


/*			 END PATHFINDING 			*/
/*			 END PATHFINDING 			*/
/*			 END PATHFINDING 			*/
#endif
