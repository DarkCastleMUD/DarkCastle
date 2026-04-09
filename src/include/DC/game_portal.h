#pragma once
#include <cstdint>

constexpr auto MAX_GAME_PORTALS = 9;
constexpr auto FOREVER = -5;

typedef quint64 room_t;

class game_portal
{
public:
  room_t to_room;     /* Room to make the portal to */
  qint32 *from_rooms; /* Rooms to make the portal from */
  qint32 num_rooms;   /* Number of rooms in from_rooms */
  qint32 obj_num;     /* Object to duplicate for portal */
  qint32 max_timer;   /* What does the timer reset to? -- game days */
  qint32 cur_timer;   /* What is the timer at now? -- game days */
};

void load_game_portals();
void process_portals();
