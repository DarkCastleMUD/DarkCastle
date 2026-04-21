#include "DC/DC.h"

Room::Room(QObject *parent)
    : QObject(parent), dc_(qobject_cast<DC *>(parent))
{
}

Room::Room(room_t room_nr, QObject *parent)
    : number(room_nr), dc_(qobject_cast<DC *>(parent)), QObject(parent)
{
}
