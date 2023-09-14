#ifndef ZONE_H
#define ZONE_H

#include <QString>
#include <QDateTime>
#include <QList>
#include "weather.h"

typedef uint64_t zone_t;
typedef uint64_t room_t;

class ResetCommand
{
public:
    ResetCommand(){};
    ResetCommand(char comm) : command(comm), active(1){};
    char command = {}; /* current command                      */
    int if_flag = {};  // 0=always 1=if prev exe'd  2=if prev DIDN'T exe   3=ONLY on reboot
    int arg1 = {};
    int arg2 = {};
    int arg3 = {};
    QString comment = {}; /* Any comments that went with the command */
    int active = {};      // is it active? alot aren't on the builders' port
    time_t last = {};     // when was it last reset
    class Character *lastPop = {};
    time_t lastSuccess = {};
    uint64_t attempts = {};
    uint64_t successes = {};
    /*
     *  Commands:              *
     *  'M': Read a mobile     *
     *  'O': Read an object    *
     *  'P': Put obj in obj    *
     *  'G': Obj to char       *
     *  'E': Obj to char equip *
     *  'D': Set state of door *
     *  '%': arg1 in arg2 chance of being true *
     *       (this is used for putting a %chance on next command *
     */
};

bool operator==(ResetCommand a, ResetCommand b);
typedef QList<ResetCommand> zone_commands_t;

#define MAX_INDEX 6000

/* Zone Flag Bits */
class Zone
{

public:
    enum class ResetType
    {
        normal,
        full
    };

    // Remember to update const.C  zone_bits[] if you change this
    enum Flag
    {
        NO_TELEPORT = 1,
        IS_TOWN = 1 << 1, // Keep out the really bad baddies that are STAY_NO_TOWN
        MODIFIED = 1 << 2,
        UNUSED = 1 << 3,
        BPORT = 1 << 4,
        NOCLAIM = 1 << 5, // cannot claim this area
        NOHUNT = 1 << 6,
    };

    Zone(uint64_t zone_key = 0);
    char *name = {};   /* name of this zone                  */
    int lifespan = {}; /* how long between resets (minutes)  */
    QDateTime last_full_reset = {};
    int age = {}; /* current age of ths zone (minutes) */

    int players = {}; // Number of PCs in the zone

    int reset_mode = {}; /* conditions for reset (see below)   */

    zone_commands_t cmd = {}; /* command table for reset             */

    /*
     *  Reset mode:                              *
     *  0: Don't reset, and don't update age.    *
     *  1: Reset if no PC's are located in zone. *
     *  2: Just reset.                           *
     *  Update char * zone_modes[] (const.C) if you change this *
     */

    struct weather_data weather_info = {}; // for zones with unique weather

    int num_mob_first_repop = {}; // number of mobs in this zone that were repoped in first repop
    int num_mob_on_repop = {};    // number of mobs in this zone that were repoped in last repop
    int death_counter = {};       // +- counter for how often mobs in zone are killed
    int counter_mod = {};         // how quickly mobs are taken off the death_counter

    int clanowner = {};
    int gold = {};                  // gold (possibly the most descriptive comment of all time)
    int continent = {};             // what continent the zone belongs to
    int repops_without_deaths = {}; // Number of repops in a row with no deaths
    int repops_with_bonus = {};     // Number of repops where a 10% bonus occurred.

    void reset(ResetType type = ResetType::normal);
    bool isEmpty(void);

    bool isTown(void);
    void setTown(bool flag = true);

    bool isNoTeleport(void);
    void setNoTeleport(bool flag = true);

    bool isNoClaim(void);
    void setNoClaim(bool flag = true);

    bool isNoHunt(void);
    void setNoHunt(bool flag = true);

    bool isModified(void);
    void setModified(bool flag = true);

    void incrementDiedThisTick(void);
    void setDiedThisTick(uint64_t died = {});
    uint64_t getDiedThisTick(void);

    QString getFilename(void);
    void setFilename(QString);

    void setZoneFlags(uint64_t);

    void setGold(uint64_t value);
    void addGold(uint64_t value);
    void incrementPlayers(void);
    void decrementPlayers(void);

    room_t getBottom(void);
    void setBottom(int room_key);

    int getTop(void);
    void setTop(int room_key);

    room_t getRealBottom(void);
    void setRealBottom(int room_key);

    int getRealTop(void);
    void setRealTop(int room_key);

    void write(FILE *fl);
    int show_info(Character *ch);

    zone_t getID(void) const
    {
        return id_;
    }

private:
    zone_t id_ = {};
    uint64_t died_this_tick = {}; // number of mobs that have died in this zone this pop
    uint64_t zone_flags = {};     /* flags for the entire zone eg: !teleport */
    QString filename = {};        /* name of the file this zone is kept in */
    room_t bottom = {};           /* bottom limit for room vnums in this zone */
    room_t top = {};              /* upper limit for room vnums in this zone */
    room_t bottom_rnum = {};
    room_t top_rnum = {};
};

bool isValidZoneKey(Character *ch, const zone_t zone_key);
bool isValidZoneCommandKey(Character *ch, const Zone &zone, const uint64_t zone_command_key);
qsizetype getZoneLastCommandNumber(const Zone &zone);
zone_t getZoneKey(Character *ch, const QString input, bool *ok = nullptr);
uint64_t getZoneCommandKey(Character *ch, const Zone &zone, const QString input, bool *ok = nullptr);

zone_t zedit_add(Character *ch, QStringList arguments, Zone &zone);
#endif