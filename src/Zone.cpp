#include "DC.h"

Zone::Zone(zone_t zone_key)
    : id(zone_key)
{
}

QString Zone::getFilename(void)
{
    return filename;
}

void Zone::setFilename(QString value)
{
    filename = value;
}

uint64_t Zone::getDiedThisTick(void)
{
    return died_this_tick;
}

void Zone::incrementDiedThisTick(void)
{
    died_this_tick++;
}

void Zone::setDiedThisTick(uint64_t died)
{
    died_this_tick = died;
}

bool Zone::isModified(void)
{
    return IS_SET(zone_flags, MODIFIED);
}

void Zone::setModified(bool flag)
{
    if (flag)
    {
        SET_BIT(zone_flags, MODIFIED);
    }
    else
    {
        REMOVE_BIT(zone_flags, MODIFIED);
    }
}

bool Zone::isTown(void)
{
    return IS_SET(zone_flags, IS_TOWN);
}

void Zone::setTown(bool flag)
{
    if (flag)
    {
        SET_BIT(zone_flags, IS_TOWN);
    }
    else
    {
        REMOVE_BIT(zone_flags, IS_TOWN);
    }
}

bool Zone::isNoTeleport(void)
{
    return IS_SET(zone_flags, NO_TELEPORT);
}

void Zone::setNoTeleport(bool flag)
{
    if (flag)
    {
        SET_BIT(zone_flags, NO_TELEPORT);
    }
    else
    {
        REMOVE_BIT(zone_flags, NO_TELEPORT);
    }
}

bool Zone::isNoClaim(void)
{
    return IS_SET(zone_flags, NOCLAIM);
}

void Zone::setNoClaim(bool flag)
{
    if (flag)
    {
        SET_BIT(zone_flags, NOCLAIM);
    }
    else
    {
        REMOVE_BIT(zone_flags, NOCLAIM);
    }
}

bool Zone::isNoHunt(void)
{
    return IS_SET(zone_flags, NOHUNT);
}

void Zone::setNoHunt(bool flag)
{
    if (flag)
    {
        SET_BIT(zone_flags, NOHUNT);
    }
    else
    {
        REMOVE_BIT(zone_flags, NOHUNT);
    }
}

void Zone::setZoneFlags(uint64_t flags)
{
    zone_flags = flags;
}

void Zone::setGold(uint64_t value)
{
    gold = value;
}

void Zone::addGold(uint64_t value)
{
    gold += value;
}

void Zone::incrementPlayers(void)
{
    players++;
}

void Zone::decrementPlayers(void)
{
    players--;
}

room_t Zone::getBottom(void)
{
    return bottom;
}

void Zone::setBottom(int room_key)
{
    bottom = room_key;
}

int Zone::getTop(void)
{
    return top;
}

void Zone::setTop(int room_key)
{
    top = room_key;
}

room_t Zone::getRealBottom(void)
{
    return bottom_rnum;
}

void Zone::setRealBottom(int room_key)
{
    bottom_rnum = room_key;
}

int Zone::getRealTop(void)
{
    return top_rnum;
}

void Zone::setRealTop(int room_key)
{
    top_rnum = room_key;
}
