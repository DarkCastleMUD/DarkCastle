#include "DC/DC.h"

Room::Room(DCPtr dc)
    : dc_(dc)
{
  if (dc_)
    QObject(dynamic_cast<QObject *>(dc.data()));
}

Room::Room(room_t room_nr, DCPtr dc)
    : number(room_nr), dc_(dc)
{
  if (dc_)
    QObject(dynamic_cast<QObject *>(dc.data()));
}

auto &operator<<(auto &out, const Room &room);
{
  auto temp_room_flags = room.room_flags;
  if (room.iFlags)
  {
    REMOVE_BIT(temp_room_flags, room.iFlags);
  }

  extra_descr_data *extra;
  if (!DC::getInstance()->rooms.contains(room.number))
    return out;

  out << "#" << room.number << "\n";
  string_to_file(out, room.name);
  string_to_file(out, room.description);

  out << room.zone << " " << room.room_flags << " " << room.sector_type << "\n";

  /* exits */
  for (qint32 b = {}; b <= 5; b++)
  {
    if (!(room.dir_option[b]))
      continue;
    out << "D" << b << "\n";
    if (room.dir_option[b]->general_description)
      string_to_file(out, room.dir_option[b]->general_description);
    else
      out << "~\n"; // print blank
    if (room.dir_option[b]->keyword)
      string_to_file(out, room.dir_option[b]->keyword);
    else
      out << "~\n"; // print blank
    out << room.dir_option[b]->exit_info << " " << room.dir_option[b]->key << " " << room.dir_option[b]->to_room << "\n";
  } /* exits */

  /* extra descriptions */
  for (extra = room.ex_description; extra; extra = extra->next)
  {
    if (!extra)
      break;
    out << "E\n";
    if (extra->keyword)
      string_to_file(out, extra->keyword);
    else
      out << "~\n"; // print blank
    if (extra->description)
      string_to_file(out, extra->description);
    else
      out << "~\n"; // print blank
  } /* extra descriptions */

  deny_data *deni;
  for (deni = room.denied; deni; deni = deni->next)
  {
    out << "B\n"
        << deni->vnum << "\n";
  }

  // Write out allowed classes if any
  for (qint32 i = {}; i < CLASS_MAX; i++)
  {
    if (room.allow_class[i] == true)
    {
      out << "C" << i << "\n";
    }
  }

  out << "S\n";
  return out;
}
