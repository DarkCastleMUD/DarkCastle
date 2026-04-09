#pragma once
#include "DC/mobile.h"
#include "DC/structs.h"

extern const QStringList dirs;
extern race_data races[];
extern const QStringList spells;
extern const char *apply_types[];
extern const char *race_types[];
extern const char *zone_modes[];
extern const char *isr_bits[];
extern const char *sector_types[];
extern const char *room_bits[];
extern const char *sector_types[];
extern const char *pc_clss_types[];
extern const char *pc_clss_types2[];
extern const char *pc_clss_types3[];
extern const char *pc_clss_abbrev[];

/* External variables */

extern qint32 max_who;

extern char globalBuf[MAX_STRING_LENGTH];
extern bool wizlock;

extern char *nonew_new_list[30];
extern const char *zone_modes[];
extern const char *equipment_types[];
extern const char *utility_item_types[];

extern qint32 top_of_mobt;
extern qint32 top_of_objt;
extern const char *action_bits[];
extern const char *affected_bits[];
extern const char *size_bitfields[];
extern char *strs_damage_types[];
extern char *obj_types[];
extern QList<QString> continent_names;
extern qint32 top_of_objt;

extern const QStringList drinks;
extern const char *portal_bits[];
extern const char *player_bits[];
extern const char *combat_bits[];
extern const char *connected_types[];
extern const char *mob_types[];
extern const char *exit_bits[];
extern qint32 mob_race_mod[][5];

extern const char *race_abbrev[];

extern qint32 max_who;
extern qint32 movement_loss[];
extern const QStringList skills;
extern const QStringList ki;
extern const char *innate_skills[];
extern const QStringList reserved;
extern char *time_look[];
