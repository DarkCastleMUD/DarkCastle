#ifndef GAME_PORTAL_H_
#define GAME_PORTAL_H_

#define MAX_GAME_PORTALS 9
#define FOREVER          -5

#define PORTAL_NO_LEAVE   1<<0
#define PORTAL_NO_ENTER   1<<1

struct game_portal
{
  int to_room;       /* Room to make the portal to */
  int *from_rooms;   /* Rooms to make the portal from */
  int num_rooms;     /* Number of rooms in from_rooms */
  int obj_num;       /* Object to duplicate for portal */
  int max_timer;     /* What does the timer reset to? -- game days */
  int cur_timer;     /* What is the timer at now? -- game days */
};
 
void load_game_portals();
void process_portals();

#endif
