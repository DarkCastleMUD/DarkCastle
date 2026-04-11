/************************************************************************
| game_portal.C
| Description:  Handles creation and removal of the magical portals that
|   move throughout the game.
*/
#include <cstdio>

#include "DC/class.h"
#include "DC/obj.h"
#include "DC/game_portal.h"
#include "DC/structs.h"
#include "DC/DC.h"
#include "DC/db.h"
#include "DC/handler.h"
#include "DC/utility.h"

qint32 make_arbitrary_portal(qint32 from_room, qint32 to_room, qint32 duplicate, qint32 timer);

game_portal game_portals[MAX_GAME_PORTALS];

/************************************************************************
| load_game_portals
| Description:  Loads the game portals from the list of filenames --
|   hard coded!
*/

const QStringList portal_bits = {
    "NO_LEAVE",
    "NO_ENTER",
    "\n"};

void load_game_portals()
{

  // TODO - make this a read which portals from a file like it should instead of
  // being hard coded

  const QStringList portal_files =
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

  extern game_portal game_portals[MAX_GAME_PORTALS];
  qint32 i, j;
  qint32 num_lines = {}; /* Temporary to count lines */
  qint32 file_pos;       /* Used to store position before counting length */
  FILE *cur_file;
  QString buf; /* Stores temp file names */
  QString log_buf;

  for (i = {}; i < MAX_GAME_PORTALS; i++)
  {
    num_lines = {};
    QString portal_filename = QStringLiteral("%1/%2").arg(DC::getInstance()->cf.library_directory).arg(portal_files[i]);
    if ((cur_file = fopen(qPrintable(portal_filename), "r")) == 0)
    {
      DC::getInstance()->logentry(QStringLiteral("Could not open portal file: %1").arg(portal_filename));
      break;
    }
    /* Now we have a readable file.  Here's the ure:
    |  First Line:  to_room -- the room the portal goes to
    |  Second Line: object -- Object to duplicate for portal (-1 for none)
    |  Third Line:  timer  -- Length object sticks around (-1 for FOREVER)
    |  Subsequent Lines: from_rooms.  List of rooms the portal might be
    |    from.
    |  NOTE:  THERE SHOULD NOT BE A BLANK LINE AT THE END OF THE FILE.  THIS
    |    WILL CAUSE THE GAME TO CRASH.  I could build a sanity check, but
    |    if people read this it's not necessary.  -Morc 24 Apr 1997
    */
    if (fscanf(cur_file, "%lld\n%d\n%d\n",
               &(game_portals[i].to_room),
               &(game_portals[i].obj_num),
               &(game_portals[i].max_timer)) != 3)
    {
      DC::getInstance()->logentry(QStringLiteral("Error reading portal file: %1!").arg(buf));
      break;
    }
    /* Store the current file value and count line feeds */
    file_pos = ftell(cur_file);
    while (fscanf(cur_file, "%*d\n") != EOF)
      num_lines++;
    fseek(cur_file, file_pos, 0);
    game_portals[i].num_rooms = num_lines;
    game_portals[i].from_rooms = new qint32[game_portals[i].num_rooms];
    for (j = {}; j < game_portals[i].num_rooms; j++)
    {
      fscanf(cur_file, "%d\n", ((game_portals[i]).from_rooms + j));
    }
    /* Now set some other values that aren't set */
    game_portals[i].cur_timer = {}; /* So that we get reset */
    if (game_portals[i].max_timer == (-1))
      game_portals[i].max_timer = FOREVER;
    fclose(cur_file);
  }
}

void DC::free_game_portals_from_memory(void)
{
  for (qint32 i = {}; i < MAX_GAME_PORTALS; i++)
  {
    if (game_portals[i].from_rooms)
    {
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
  qint32 i;
  //  extern  game_portal game_portals[];

  for (i = {}; i < MAX_GAME_PORTALS; i++)
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
      qint32 from_room =
          game_portals[i].from_rooms[number(0, game_portals[i].num_rooms - 1)];

      /* So the portal is already gone, all we do is create a new one */
      if (make_arbitrary_portal(
              from_room,
              game_portals[i].to_room,
              game_portals[i].obj_num,
              game_portals[i].max_timer) == 0)
      {
        QString log_buf = {};
        dc_sprintf(log_buf, "Making portal from %d to %llu failed.", from_room,
                   game_portals[i].to_room);
        DC::getInstance()->logentry(log_buf, OVERSEER, DC::LogChannel::LOG_BUG);
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
qint32 make_arbitrary_portal(qint32 from_room, qint32 to_room, qint32 duplicate, qint32 timer)
{

  QString log_buf;

  auto from_portal = new Object;
  clear_object(from_portal);

  if (real_room(from_room) == DC::NOWHERE)
  {
    dc_sprintf(log_buf, "Cannot create arbitrary portal: room %d doesn't exist.", from_room);
    from_portal = {};
    DC::getInstance()->logentry(log_buf, OVERSEER, DC::LogChannel::LOG_BUG);
    return {};
  }

  if (from_room == to_room)
  {
    from_portal = {};
    DC::getInstance()->logentry(QStringLiteral("Arbitrary portal made to itself!"), OVERSEER, DC::LogChannel::LOG_BUG);
    return {};
  }

  if (duplicate < 0) /* Make a generic portal */
  {
    from_portal->name(QStringLiteral("portal"));
    from_portal->short_description("a path to a hidden world");
    from_portal->long_description("A mystical path to a hidden world "
                                  "shimmers in the air before you.");

    from_portal->obj_flags.type_flag = ITEM_PORTAL;
    from_portal->item_number = (-1);

    /* Only need to do this if I didn't clone it */
    from_portal->next = DC::getInstance()->object_list;
    DC::getInstance()->object_list = from_portal;
  }
  else /* Duplicate the object # duplicate */
  {
    from_portal = clone_object(real_object(duplicate));

    if (!from_portal->isPortal())
    {
      dc_sprintf(log_buf, "Non-portal object (%d) sent to make_arbitrary_portal!", duplicate);
      from_portal = {};
      DC::getInstance()->logentry(log_buf, OVERSEER, DC::LogChannel::LOG_BUG);
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

void find_and_remove_player_portal(CharacterPtr ch)
{
  ObjectPtr k = {};
  ObjectPtr next_k = {};
  QString searchstr;

  if (GET_CLASS(ch) == CLASS_CLERIC)
    searchstr = QStringLiteral("cleric %1").arg(qPrintable(ch->name()));
  else
    searchstr = QStringLiteral("only %1").arg(qPrintable(ch->name()));

  for (k = DC::getInstance()->object_list; k; k = next_k)
  {
    next_k = k->next;
    if (!k->isPortal() || !k->name().contains(searchstr))
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
