#include "DC.h"

Zone::Zone(zone_t zone_key)
    : id_(zone_key)
{
}

QStringList Zone::zone_bits =
    {
        "NO_TELEPORT",
        "IS_TOWN(keep out STAY_NO_TOWN mobs)",
        "MODIFIED",
        "UNUSED",
        "BPORT",
        "NOCLAIM",
        "NOHUNT"};

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
    return isSet(zone_flags, MODIFIED);
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
    return isSet(zone_flags, IS_TOWN);
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
    return isSet(zone_flags, NO_TELEPORT);
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
    return isSet(zone_flags, NOCLAIM);
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
    return isSet(zone_flags, NOHUNT);
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

bool operator==(ResetCommand a, ResetCommand b)
{
    if (a.command == b.command && a.if_flag == b.if_flag && a.arg1 == b.arg1 && a.arg2 == b.arg2 && a.arg3 == b.arg3 && a.comment == b.comment)
    {
        return true;
    }

    return false;
}

zone_t getZoneKey(Character *ch, const QString input, bool *ok)
{
    zone_t zone_key = input.toULongLong(ok);
    if (!isValidZoneKey(ch, zone_key) && ok)
    {
        *ok = false;
    }

    return zone_key;
}

bool isValidZoneKey(Character *ch, const zone_t zone_key)
{
    const auto dc = DC::getInstance();
    if (!dc->zones.contains(zone_key))
    {
        if (ch)
        {
            ch->send(QStringLiteral("Zone %1 not found.\r\n").arg(zone_key));
            if (dc->zones.isEmpty())
            {
                ch->send("There are no zones currently.\r\n");
            }
            else
            {
                ch->send(QStringLiteral("Valid values are 1-%1\r\n").arg(dc->zones.last().getID()));
            }
        }
        return false;
    }
    return true;
}

uint64_t getZoneCommandKey(Character *ch, const Zone &zone, const QString input, bool *ok)
{
    uint64_t zone_command_key = input.toULongLong(ok);
    if (!isValidZoneCommandKey(ch, zone, zone_command_key - 1) && ok)
    {
        *ok = false;
    }
    return zone_command_key - 1;
}

bool isValidZoneCommandKey(Character *ch, const Zone &zone, const qsizetype zone_command_key)
{
    if (zone.cmd.isEmpty() || zone_command_key >= zone.cmd.size())
    {
        if (ch)
        {
            ch->send(QStringLiteral("Zone command %1 not found.\r\n").arg(zone_command_key + 1));
            if (zone.cmd.isEmpty())
            {
                ch->send("There are no zone commands.\r\n");
            }
            else
            {
                ch->send(QStringLiteral("Valid values are 1-%1\r\n").arg(zone.cmd.size()));
            }
        }
        return false;
    }
    return true;
}

qsizetype getZoneLastCommandNumber(const Zone &zone)
{
    return zone.cmd.size();
}
