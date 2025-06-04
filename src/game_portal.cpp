/************************************************************************
| game_portal.C
| Description:  Handles creation and removal of the magical portals that
|   move throughout the game.
*/
#include <cstdio>

#include "DC/obj.h"
#include "DC/game_portal.h"
#include "DC/fileinfo.h"
#include "DC/structs.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/DC.h"
#include "DC/room.h"
#include "DC/db.h"
#include "DC/handler.h"

#include <cstring>

int make_arbitrary_portal(int from_room, int to_room, int duplicate, int timer);

struct game_portal game_portals[MAX_GAME_PORTALS];

/************************************************************************
| load_game_portals
| Description:  Loads the game portals from the list of filenames --
|   hard coded!
*/

const char *portal_bits[] = {
    "NO_LEAVE",
    "NO_ENTER",
    "\n"};

void load_game_portals()
{

  // TODO - make this a read which portals from a file like it should instead of
  // being hard coded

  char *portal_files[MAX_GAME_PORTALS] =
      {
          "portal/portal.wagon",
          "portal/portal.tower",
          "portal/portal.ship1",
          "portal/portal.ship2",
          "portal/portal.ship3",
          "portal/portal.ship4",
          "portal/portal.ship5",
          "portal/portal.ship6",
          "portal/portal.arcana"};

  extern struct game_portal game_portals[MAX_GAME_PORTALS];
  int i, j;
  int num_lines = 0; /* Temporary to count lines */
  int32_t file_pos;  /* Used to store position before counting length */
  FILE *cur_file;
  char buf[256]; /* Stores temp file names */
  char log_buf[256];

  for (i = 0; i < MAX_GAME_PORTALS; i++)
  {
    num_lines = 0;
    QString portal_filename = QStringLiteral("%1/%2").arg(DC::getInstance()->cf.library_directory).arg(portal_files[i]);
    if ((cur_file = fopen(portal_filename.toStdString().c_str(), "r")) == 0)
    {
      logentry(QStringLiteral("Could not open portal file: %1").arg(portal_filename));
      break;
    }
    /* Now we have a readable file.  Here's the structure:
    |  First Line:  to_room -- the room the portal goes to
    |  Second Line: object -- Object to duplicate for portal (-1 for none)
    |  Third Line:  timer  -- Length object sticks around (-1 for FOREVER)
    |  Subsequent Lines: from_rooms.  List of rooms the portal might be
    |    from.
    |  NOTE:  THERE SHOULD NOT BE A BLANK LINE AT THE END OF THE FILE.  THIS
    |    WILL CAUSE THE GAME TO CRASH.  I could build a sanity check, but
    |    if people read this it's not necessary.  -Morc 24 Apr 1997
    */
    if (fscanf(cur_file, "%d\n%d\n%d\n",
               &(game_portals[i].to_room),
               &(game_portals[i].obj_num),
               &(game_portals[i].max_timer)) != 3)
    {
      logentry(QStringLiteral("Error reading portal file: %1!").arg(buf));
      break;
    }
    /* Store the current file value and count line feeds */
    file_pos = ftell(cur_file);
    while (fscanf(cur_file, "%*d\n") != EOF)
      num_lines++;
    fseek(cur_file, file_pos, 0);
    game_portals[i].num_rooms = num_lines;
#ifdef LEAK_CHECK
    game_portals[i].from_rooms = (int *)calloc(game_portals[i].num_rooms, sizeof(int));
#else
    game_portals[i].from_rooms = (int *)dc_alloc(game_portals[i].num_rooms, sizeof(int));
#endif
    for (j = 0; j < game_portals[i].num_rooms; j++)
    {
      fscanf(cur_file, "%d\n", ((game_portals[i]).from_rooms + j));
    }
    /* Now set some other values that aren't set */
    game_portals[i].cur_timer = 0; /* So that we get reset */
    if (game_portals[i].max_timer == (-1))
      game_portals[i].max_timer = FOREVER;
    fclose(cur_file);
  }
}

void DC::free_game_portals_from_memory(void)
{
  for (int i = 0; i < MAX_GAME_PORTALS; i++)
  {
    if (game_portals[i].from_rooms)
    {
      dc_free(game_portals[i].from_rooms);
      game_portals[i].from_rooms = {};
    }
  }
}

/************************************************************************
| process_portals
| Description: Moves the game portals around if their timers have reached
|   0, otherwise it just decrements the timer.
| Returns: void
*/
void process_portals()
{
  int i;
  //  extern struct game_portal game_portals[];

  for (i = 0; i < MAX_GAME_PORTALS; i++)
  {
    if (game_portals[i].cur_timer == FOREVER)
      continue;
    game_portals[i].cur_timer--;
    /* This is sort of tricky.  Here's what happens:
    |  We set the timer of the portal to the max_timer of the object
    |  so that it gets removed by the game after that much time.  We
    |  then keep track of our own timer for re-creation of the portal
    |  after it's removed by the game.
    */
    if (game_portals[i].cur_timer <= 0)
    {
      int from_room =
          game_portals[i].from_rooms[number(0, game_portals[i].num_rooms - 1)];

      /* So the portal is already gone, all we do is create a new one */
      if (make_arbitrary_portal(
              from_room,
              game_portals[i].to_room,
              game_portals[i].obj_num,
              game_portals[i].max_timer) == 0)
      {
        char log_buf[MAX_STRING_LENGTH] = {};
        sprintf(log_buf, "Making portal from %d to %d failed.", from_room,
                game_portals[i].to_room);
        logentry(log_buf, OVERSEER, DC::LogChannel::LOG_BUG);
      }
      game_portals[i].cur_timer = game_portals[i].max_timer;
    }
  }
}

/************************************************************************
| make_arbitrary_portal
| Description: Makes a portal from from_room to to_room, in
|   real_room() numbers (not virtual).  Duplicate is the object
|   to duplicate -- if it's < 0, we make a generic portal, otherwise,
|   we copy the object given and do some minor checking to make sure it's
|   a portal.  Timer is the length of time this particular portal lives --
|   in days.
|
| Returns: 0 on error, non-zero on success
*/
int make_arbitrary_portal(int from_room, int to_room, int duplicate, int timer)
{

  class Object *from_portal;
  char log_buf[256];

#ifdef LEAK_CHECK
  from_portal = (class Object *)calloc(1, sizeof(class Object));
#else
  from_portal = (class Object *)dc_alloc(1, sizeof(class Object));
#endif
  clear_object(from_portal);

  if (real_room(from_room) == DC::NOWHERE)
  {
    sprintf(log_buf, "Cannot create arbitrary portal: room %d doesn't exist.", from_room);
    dc_free(from_portal);
    logentry(log_buf, OVERSEER, DC::LogChannel::LOG_BUG);
    return (0);
  }

  if (from_room == to_room)
  {
    dc_free(from_portal);
    logentry(QStringLiteral("Arbitrary portal made to itself!"), OVERSEER, DC::LogChannel::LOG_BUG);
    return (0);
  }

  if (duplicate < 0) /* Make a generic portal */
  {
    from_portal->name = str_hsh("portal");
    from_portal->short_description = str_hsh("a path to a hidden world");
    from_portal->description = str_hsh("A mystical path to a hidden world "
                                       "shimmers in the air before you.");

    from_portal->obj_flags.type_flag = ITEM_PORTAL;
    from_portal->vnum = 0;

    /* Only need to do this if I didn't clone it */
    from_portal->next = DC::getInstance()->object_list;
    DC::getInstance()->object_list = from_portal;
  }
  else /* Duplicate the object # duplicate */
  {
    from_portal = DC::getInstance()->clone_object(duplicate);

    if (!from_portal->isPortal())
    {
      sprintf(log_buf, "Non-portal object (%d) sent to make_arbitrary_portal!", duplicate);
      dc_free(from_portal);
      logentry(log_buf, OVERSEER, DC::LogChannel::LOG_BUG);
      return 0;
    }
  }
  /* Nonspecific things -- done to all portals */

  from_portal->obj_flags.timer = timer;

  from_portal->in_room = DC::NOWHERE;

  /* The room that it goes to */
  from_portal->setPortalDestinationRoom(to_room);

  /* Make it game_portal flagged for do_enter and limits.C */
  from_portal->obj_flags.value[1] = 2;
  /* Make it non-zone wide for do_leave */
  from_portal->obj_flags.value[2] = (-1);

  obj_to_room(from_portal, real_room(from_room));

  send_to_room("There is a violent flash of light as a portal "
               "shimmers into existence.\r\n",
               real_room(from_room));

  /* Success - presumably */
  return (1);
}

void find_and_remove_player_portal(Character *ch)
{
  class Object *k;
  class Object *next_k;
  char searchstr[180];

  if (GET_CLASS(ch) == CLASS_CLERIC)
    sprintf(searchstr, "cleric %s", GET_NAME(ch));
  else
    sprintf(searchstr, "only %s", GET_NAME(ch));

  for (k = DC::getInstance()->object_list; k; k = next_k)
  {
    next_k = k->next;
    if (!k->isPortal() || !strstr(k->name, searchstr))
      continue;

    // at this point, the portal belongs to the person that quit
    if (k->in_room < DC::getInstance()->top_of_world && k->in_room > DC::NOWHERE && DC::getInstance()->rooms.contains(k->in_room))
    {
      send_to_room("Its creator gone, the portal fades away prematurely.\r\n", k->in_room);
    }

    obj_from_room(k);
    extract_obj(k);
  }
}
