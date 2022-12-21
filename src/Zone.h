#ifndef ZONE_H
#define ZONE_H

#include <QDateTime>

#include "weather.h"

struct Zone
{
    Zone();
    char *name = {};   /* name of this zone                  */
    int lifespan = {}; /* how long between resets (minutes)  */
    QDateTime last_full_reset = {};
    int age = {}; /* current age of ths zone (minutes) */
    int top = {}; /* upper limit for room vnums in this zone */
    int bottom_rnum = {};
    int top_rnum = {};
    int32_t zone_flags = {}; /* flags for the entire zone eg: !teleport */

    int players = {}; // Number of PCs in the zone

    char *filename = {}; /* name of the file this zone is kept in */

    int reset_mode = {}; /* conditions for reset (see below)   */

    struct reset_com *cmd = {}; /* command table for reset             */
    int reset_total = {};       /* total number item in currently allocated
                                 * reset_com array.  This is used in the
                                 * do_zedit command so we don't have to realloc
                                 * every time we add/delete a command
                                 */
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
    int died_this_tick = {};      // number of mobs that have died in this zone this pop
    int clanowner = {};
    int gold = {};                  // gold (possibly the most descriptive comment of all time)
    int continent = {};             // what continent the zone belongs to
    int repops_without_deaths = {}; // Number of repops in a row with no deaths
    int repops_with_bonus = {};     // Number of repops where a 10% bonus occurred.
};

#endif