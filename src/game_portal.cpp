#include "DC/DC.h"

const QStringList portal_bits = {
    "NO_LEAVE",
    "NO_ENTER"};

void DC::load_game_portals(void)
{

  // TODO - make this a read which portals from a file like it should instead of
  // being hard coded

  const QStringList portal_filenames =
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

  qint32 i, j;
  qint32 num_lines = {}; /* Temporary to count lines */
  qint32 file_pos;       /* Used to store position before counting length */
  QString buf;           /* Stores temp file names */
  QString log_buf;

  for (qsizetype portal_id = {}; portal_id < portal_filenames.size(); ++portal_id)
  {
    auto &portal_filename = portal_filenames[portal_id];
    QString full_filename = u"%1/%2"_s.arg(dc_->cf.library_directory).arg(portal_filename);
    QFile portal_file(portal_filename);

    if (!portal_file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
    {
      dc_->logentry(u"Could not open portal file: %1"_s.arg(portal_filename));
      break;
    }
    QTextStream in(&portal_file);
    in >> game_portals_[portal_id].to_room;
    in >> game_portals_[portal_id].obj_num;
    in >> game_portals_[portal_id].max_timer;

    /* Store the current file value and count line feeds */
    // game_portals_[i].num_rooms

    while (!in.atEnd())
    {
      room_t buffer;
      in >> buffer;
      game_portals_[portal_id].from_rooms.insert(buffer);
    }
    game_portals_[portal_id].cur_timer = {}; /* So that we get reset */

    if (game_portals_[portal_id].max_timer == -1)
      game_portals_[portal_id].max_timer = FOREVER;
  }
}

void DC::free_game_portals_from_memory(void)
{
}

/************************************************************************
| process_portals
| Description: Moves the game portals around if their timers have reached
|   0, otherwise it just decrements the timer.
| Returns: void
*/
void DC::process_portals(void)
{
  for (auto &portal : game_portals_)
  {
    if (portal.cur_timer == FOREVER)
      continue;
    portal.cur_timer--;

    /* This is sort of tricky.  Here's what happens:
    |  We set the timer of the portal to the max_timer of the object
    |  so that it gets removed by the game after that much time.  We
    |  then keep track of our own timer for re-creation of the portal
    |  after it's removed by the game.
    */
    if (portal.cur_timer <= 0)
    {
      room_t from_room = portal.from_rooms[number(0, portal.from_rooms.size() - 1)];

      /* So the portal is already gone, all we do is create a new one */
      if (make_arbitrary_portal(from_room, portal.to_room, portal.obj_num, portal.max_timer) == 0)
      {
        QString log_buf = {};
        dc_sprintf(log_buf, "Making portal from %d to %llu failed.", from_room,
                   portal.to_room);
        dc_->logentry(log_buf, OVERSEER, DC::LogChannel::LOG_BUG);
      }
      portal.cur_timer = portal.max_timer;
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
bool DC::make_room_portal(qint32 from_room, qint32 to_room, qint32 duplicate, qint32 timer)
{
  if (from_room == INVALID_ROOM)
  {
    QString log_buf;
    dc_sprintf(log_buf, "Cannot create portal: room %d doesn't exist.", from_room);
    logbug(log_buf);
    return false;
  }

  auto from_portal = ObjectPtr(new Object(this));
  clear_object(from_portal);

  if (from_room == to_room)
  {
    from_portal = {};
    logentry(u"Portal made to itself!"_s, OVERSEER, DC::LogChannel::LOG_BUG);
    return false;
  }

  if (duplicate < 0) /* Make a generic portal */
  {
    from_portal->name(u"portal"_s);
    from_portal->short_description("a path to a hidden world");
    from_portal->long_description("A mystical path to a hidden world "
                                  "shimmers in the air before you.");

    from_portal->flags_.type_flag = ITEM_PORTAL;
    from_portal->item_number = (-1);

    /* Only need to do this if I didn't clone it */
    from_portal->next = dc_->object_list;
    dc_->object_list = from_portal;
  }
  else /* Duplicate the object # duplicate */
  {
    from_portal = clone_object(real_object(duplicate));

    if (!from_portal->isPortal())
    {
      QString log_buf;
      dc_sprintf(log_buf, "Non-portal object (%d) sent to make_arbitrary_portal!", duplicate);
      from_portal = {};
      dc_->logentry(log_buf, OVERSEER, DC::LogChannel::LOG_BUG);
      return false;
    }
  }
  /* Nonspecific things -- done to all portals */

  from_portal->flags_.timer = timer;

  from_portal->in_room = INVALID_ROOM;

  /* The room that it goes to */
  from_portal->setPortalDestinationRoom(to_room);

  /* Make it game_portal flagged for do_enter and limits.C */
  from_portal->flags_.value[1] = 2;
  /* Make it non-zone wide for do_leave */
  from_portal->flags_.value[2] = -1;

  obj_to_room(from_portal, from_room);

  send_to_room("There is a violent flash of light as a portal "
               "shimmers into existence.\r\n",
               from_room);

  /* Success - presumably */
  return true;
}

void find_and_remove_player_portal(CharacterPtr ch)
{
  ObjectPtr k = {};
  ObjectPtr next_k = {};
  QString searchstr;

  if (GET_CLASS(ch) == CLASS_CLERIC)
    searchstr = u"cleric %1"_s.arg(qPrintable(ch->name()));
  else
    searchstr = u"only %1"_s.arg(qPrintable(ch->name()));

  for (k = dc_->object_list; k; k = next_k)
  {
    next_k = k->next;
    if (!k->isPortal() || !k->name().contains(searchstr))
      continue;

    // at this point, the portal belongs to the person that quit
    if (k->in_room < dc_->top_of_world && k->in_room > INVALID_ROOM && dc_->rooms.contains(k->in_room))
    {
      send_to_room("Its creator gone, the portal fades away prematurely.\r\n", k->in_room);
    }

    obj_from_room(k);
    extract_obj(k);
  }
}
