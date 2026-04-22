#include "DC/DC.h"

Room::Room(QObject *parent)
    : QObject(parent), dc_(qobject_cast<DC *>(parent))
{
}

Room::Room(QObject *parent, room_t room_number)
    : QObject(parent), dc_(qobject_cast<DC *>(parent)), number(room_number)
{
}

bool Room::isDark() const { return isSet(room_flags, DARK); }
bool Room::isNoHome() const { return isSet(room_flags, NOHOME); }
bool Room::isNoMob() const { return isSet(room_flags, NO_MOB); }
bool Room::isIndoors() const { return isSet(room_flags, INDOORS); }
bool Room::isTeleportBlocked() const { return isSet(room_flags, TELEPORT_BLOCK); }
bool Room::isNoKi() const { return isSet(room_flags, NO_KI); }
bool Room::isNoLearn() const { return isSet(room_flags, NOLEARN); }
bool Room::isNoMagic() const { return isSet(room_flags, NO_MAGIC); }
bool Room::isTunnel() const { return isSet(room_flags, TUNNEL); }
bool Room::isPrivate() const { return isSet(room_flags, PRIVATE); }
bool Room::isSafe() const { return isSet(room_flags, SAFE); }
bool Room::isNoSummon() const { return isSet(room_flags, NO_SUMMON); }
bool Room::isNoAstral() const { return isSet(room_flags, NO_ASTRAL); }
bool Room::isNoPortal() const { return isSet(room_flags, NO_PORTAL); }
bool Room::isImpOnly() const { return isSet(room_flags, IMP_ONLY); }
bool Room::isFallDown() const { return isSet(room_flags, FALL_DOWN); }
bool Room::isArena() const { return isSet(room_flags, ARENA); }
bool Room::isQuiet() const { return isSet(room_flags, QUIET); }
bool Room::isUnstable() const { return isSet(room_flags, UNSTABLE); }
bool Room::isNoQuit() const { return isSet(room_flags, NO_QUIT); }
bool Room::isFallUp() const { return isSet(room_flags, FALL_UP); }
bool Room::isFallEast() const { return isSet(room_flags, FALL_EAST); }
bool Room::isFallWest() const { return isSet(room_flags, FALL_WEST); }
bool Room::isFallSouth() const { return isSet(room_flags, FALL_SOUTH); }
bool Room::isFallNorth() const { return isSet(room_flags, FALL_NORTH); }
bool Room::isNoTeleport() const { return isSet(room_flags, NO_TELEPORT); }
bool Room::isNoTrack() const { return isSet(room_flags, NO_TRACK); }
bool Room::isClanRoom() const { return isSet(room_flags, CLAN_ROOM); }
bool Room::isNoScan() const { return isSet(room_flags, NO_SCAN); }
bool Room::isNoWhere() const { return isSet(room_flags, NO_WHERE); }
bool Room::isLightRoom() const { return isSet(room_flags, LIGHT_ROOM); }
