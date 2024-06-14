#ifndef __ROOM_H__
#define __ROOM_H__
/************************************************************************
| $Id: room.h,v 1.21 2015/06/14 02:38:12 pirahna Exp $
| room.h
| Description:  This file contains all of the room header file/constant
|   information.  It also contains information about the 'world' structs.
*/

#include <QSharedPointer>
#include <expected>
#include "DC/structs.h" // uint8_t
#include "DC/class.h"
#include "DC/Zone.h"
#include "DC/common.h"

/* Bitvector For 'room_flags' */

typedef uint64_t zone_t;
typedef uint64_t room_t;

const auto DARK = 1;
const auto NOHOME = 1 << 1;
const auto NO_MOB = 1 << 2;

const auto INDOORS = 1 << 3;
const auto TELEPORT_BLOCK = 1 << 4;
const auto NO_KI = 1 << 5;
const auto NOLEARN = 1 << 6;
const auto NO_MAGIC = 1 << 7;
const auto TUNNEL = 1 << 8;
const auto PRIVATE = 1 << 9;
const auto SAFE = 1 << 10;
const auto NO_SUMMON = 1 << 11;
const auto NO_ASTRAL = 1 << 12; // usused
const auto NO_PORTAL = 1 << 13;
const auto IMP_ONLY = 1 << 14;
const auto FALL_DOWN = 1 << 15;
const auto ARENA = 1 << 16;
const auto QUIET = 1 << 17;
const auto UNSTABLE = 1 << 18;
const auto NO_QUIT = 1 << 19;
const auto FALL_UP = 1 << 20;
const auto FALL_EAST = 1 << 21;
const auto FALL_WEST = 1 << 22;
const auto FALL_SOUTH = 1 << 23;
const auto FALL_NORTH = 1 << 24;
const auto NO_TELEPORT = 1 << 25;
const auto NO_TRACK = 1 << 26;
const auto CLAN_ROOM = 1 << 27;
const auto NO_SCAN = 1 << 28;
const auto NO_WHERE = 1 << 29;
const auto LIGHT_ROOM = 1 << 30;
// const auto NOHOME       = 1<<31;
// const auto NO_KI        = 1<<31;

/* Bitvector For 'temp_room_flags'
   temp_room_flags are NOT in the world file and cannot be saved or added at boot time
   These are for runtime flags, such as ETHEREAL_FOCUS
*/

;
const auto ROOM_ETHEREAL_FOCUS = 1;
const auto TEMP_ROOM_FLAG_AVAILABLE = 1 << 1;

/* Internal flags */
;
const auto iNO_TRACK = 1;
const auto iNO_MAGIC = 1 << 1;

/* For 'dir_option' */

;
const auto NORTH = 0;
const auto EAST = 1;
const auto SOUTH = 2;
const auto WEST = 3;
const auto UP = 4;
const auto DOWN = 5;
const auto MAX_DIRS = 6

    ;
const auto EX_ISDOOR = 1;
const auto EX_CLOSED = 2;
const auto EX_LOCKED = 4;
const auto EX_HIDDEN = 8;
const auto EX_IMM_ONLY = 16;
const auto EX_PICKPROOF = 32;
const auto EX_BROKEN = 64

    /* For 'Sector types' */

    ;
const auto SECT_INSIDE = 0;
const auto SECT_CITY = 1;
const auto SECT_FIELD = 2;
const auto SECT_FOREST = 3;
const auto SECT_HILLS = 4;
const auto SECT_MOUNTAIN = 5;
const auto SECT_WATER_SWIM = 6;
const auto SECT_WATER_NOSWIM = 7;
const auto SECT_BEACH = 8;
const auto SECT_PAVED_ROAD = 9;
const auto SECT_DESERT = 10;
const auto SECT_UNDERWATER = 11;
const auto SECT_SWAMP = 12;
const auto SECT_AIR = 13;
const auto SECT_FROZEN_TUNDRA = 14;
const auto SECT_ARCTIC = 15;
const auto SECT_MAX_SECT = 15; // update this if you add more
                               // and; const.c stuff for sectors

struct room_direction_data
{
    char *general_description; /* When look DIR.                  */
    char *keyword;             /* for open/close                  */
    int16_t exit_info;         /* Exit info                       */
    Character *bracee;         /* This is who is bracing the door */
    int16_t key;               /* Key's number (-1 for no key)    */
    int16_t to_room;           /* Where direction leeds (NOWHERE) */
};

struct room_track_data
{
    int weight;
    int race;
    int direction;
    int sex;
    int condition;
    QString trackee;

    room_track_data *next;
    room_track_data *previous;
};

struct deny_data
{
    struct deny_data *next;
    int vnum;
};

// ========================= Structure for room ==========================
class Room
{
public:
    Room(void)
    {
    }
    explicit Room(int16_t roomNumber, class DC *parent)
        : number(roomNumber), dc_(parent)
    {
    }
    int16_t number = {}; // Rooms number
    zone_t zone = {};    // Room zone (for resetting)
    QSharedPointer<Zone> zonePtr = {};
    int sector_type = {}; // sector type (move/hide)
    struct deny_data *denied = {};
    char *name = {};                                // Rooms name 'You are ...'
    char *description = {};                         // Shown when entered
    struct extra_descr_data *ex_description = {};   // for examine/look
    room_direction_data *dir_option[MAX_DIRS] = {}; // Directions
    uint32_t room_flags = {};                       // DEATH, DARK ... etc
    constexpr auto isDark() const -> bool { return isSet(room_flags, DARK); }
    constexpr auto isNoHome() const -> bool { return isSet(room_flags, NOHOME); }
    constexpr auto isNoMob() const -> bool { return isSet(room_flags, NO_MOB); }
    constexpr auto isIndoors() const -> bool { return isSet(room_flags, INDOORS); }
    constexpr auto isTeleportBlocked() const -> bool { return isSet(room_flags, TELEPORT_BLOCK); }
    constexpr auto isNoKi() const -> bool { return isSet(room_flags, NO_KI); }
    constexpr auto isNoLearn() const -> bool { return isSet(room_flags, NOLEARN); }
    constexpr auto isNoMagic() const -> bool { return isSet(room_flags, NO_MAGIC); }
    constexpr auto isTunnel() const -> bool { return isSet(room_flags, TUNNEL); }
    constexpr auto isPrivate() const -> bool { return isSet(room_flags, PRIVATE); }
    constexpr auto isSafe() const -> bool { return isSet(room_flags, SAFE); }
    constexpr auto isNoSummon() const -> bool { return isSet(room_flags, NO_SUMMON); }
    constexpr auto isNoAstral() const -> bool { return isSet(room_flags, NO_ASTRAL); }
    constexpr auto isNoPortal() const -> bool { return isSet(room_flags, NO_PORTAL); }
    constexpr auto isImpOnly() const -> bool { return isSet(room_flags, IMP_ONLY); }
    constexpr auto isFallDown() const -> bool { return isSet(room_flags, FALL_DOWN); }
    constexpr auto isArena() const -> bool { return isSet(room_flags, ARENA); }
    constexpr auto isQuiet() const -> bool { return isSet(room_flags, QUIET); }
    constexpr auto isUnstable() const -> bool { return isSet(room_flags, UNSTABLE); }
    constexpr auto isNoQuit() const -> bool { return isSet(room_flags, NO_QUIT); }
    constexpr auto isFallUp() const -> bool { return isSet(room_flags, FALL_UP); }
    constexpr auto isFallEast() const -> bool { return isSet(room_flags, FALL_EAST); }
    constexpr auto isFallWest() const -> bool { return isSet(room_flags, FALL_WEST); }
    constexpr auto isFallSouth() const -> bool { return isSet(room_flags, FALL_SOUTH); }
    constexpr auto isFallNorth() const -> bool { return isSet(room_flags, FALL_NORTH); }
    constexpr auto isNoTeleport() const -> bool { return isSet(room_flags, NO_TELEPORT); }
    constexpr auto isNoTrack() const -> bool { return isSet(room_flags, NO_TRACK); }
    constexpr auto isClanRoom() const -> bool { return isSet(room_flags, CLAN_ROOM); }
    constexpr auto isNoScan() const -> bool { return isSet(room_flags, NO_SCAN); }
    constexpr auto isNoWhere() const -> bool { return isSet(room_flags, NO_WHERE); }
    constexpr auto isLightRoom() const -> bool { return isSet(room_flags, LIGHT_ROOM); }

    auto arena() -> class Arena &;

    uint32_t temp_room_flags = {}; // A second bitvector for flags that do NOT get saved.  These are temporary runtime flags.
    int16_t light = {};            // Light factor of room

    int (*funct)(Character *, int, const char *) = {}; // special procedure

    class Object *contents = {}; // List of items in room
    Character *people = nullptr; // List of NPC / PC in room

    int nTracks = {};                 // number of tracks in the room
    room_track_data *tracks = {};     // beginning of the list of scents
    room_track_data *last_track = {}; // last in the scent list
    int iFlags = {};                  // Internal flags. These do NOT save.
    struct path_data *paths = {};
    bool allow_class[CLASS_MAX] = {};

    void AddTrackItem(room_track_data *newTrack);
    room_track_data *TrackItem(int nIndex);
    void FreeTracks();

    enum class room_errors_t
    {
        direction,
        number,
        zone,
        zonePtr,
        sector_type,
        denied,
        name,
        description,
        ex_description,
        room_flags,
        temp_room_flags,
        light,
        funct,
        alllow_class
    };
    /*
            static auto compare(Room &r1, Room &r2) -> std::expected<bool, room_errors_t>
            {
                for (int direction = 0; direction < MAX_DIRS; ++direction)
                {
                    if (r1.dir_option[direction] == r2.dir_option[direction])
                    {
                        continue;
                    }
                    else if (r1.dir_option[direction] && r2.dir_option[direction] &&
                             *r1.dir_option[direction] == *r2.dir_option[direction])
                    {
                        continue;
                    }
                    else
                    {
                        return std::unexpected(Room::room_errors_t::direction);
                    }
                }
                if (r1.number != r2.number)
                    return std::unexpected(Room::room_errors_t::number);
                if (r1.zone != r2.zone)
                    return std::unexpected(Room::room_errors_t::zone);
                if (r1.zonePtr != r2.zonePtr)
                    return std::unexpected(Room::room_errors_t::zonePtr);
                if (r1.sector_type != r2.sector_type)
                    return std::unexpected(Room::room_errors_t::sector_type);
                if (r1.denied != r2.denied)
                    return std::unexpected(Room::room_errors_t::denied);
                if (QString(r1.name) != QString(r2.name))
                    return std::unexpected(Room::room_errors_t::name);
                if (QString(r1.description) != QString(r2.description))
                    return std::unexpected(Room::room_errors_t::description);
                if (r1.ex_description != r2.ex_description)
                    return std::unexpected(Room::room_errors_t::ex_description);
                if (r1.room_flags != r2.room_flags)
                    return std::unexpected(Room::room_errors_t::room_flags);
                if (r1.temp_room_flags != r2.temp_room_flags)
                    return std::unexpected(Room::room_errors_t::temp_room_flags);
                if (r1.light != r2.light)
                    return std::unexpected(Room::room_errors_t::light);
                if (r1.funct != r2.funct)
                    return std::unexpected(Room::room_errors_t::funct);
                // r1.contents == r2.contents &&
                // r1.people == r2.people &&
                // r1.nTracks == r2.nTracks &&
                // r1.tracks == r2.tracks &&
                // r1.iFlags == r2.iFlags &&
                // ((r1.paths == r2.paths) || (r1.paths && r2.paths && *r1.paths == *r2.paths)) &&
                if (memcmp(r1.allow_class, r2.allow_class, sizeof(r1.allow_class)))
                    return std::unexpected(Room::room_errors_t::alllow_class);

                return true;
            }
        */
private:
    class DC *dc_{};
};

bool operator==(const Room &r1, const Room &r2);
struct Entity
{
    auto room(void) -> Room &;
    room_t in_room = {};
};

#endif // __ROOM_H__
