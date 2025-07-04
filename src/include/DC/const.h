#ifndef CONST_H
#define CONST_H
extern const char *dirs[];
extern struct race_data races[];
extern const char *spells[];
extern const char *apply_types[];
extern item_types_t item_types;
extern const char *race_types[];
extern const char *zone_modes[];
extern const char *isr_bits[];
extern const char *sector_types[];
extern const char *room_bits[];
extern const char *sector_types[];
extern struct class_skill_defines g_skills[];
extern struct class_skill_defines w_skills[];
extern struct class_skill_defines t_skills[];
extern struct class_skill_defines d_skills[];
extern struct class_skill_defines b_skills[];
extern struct class_skill_defines a_skills[];
extern struct class_skill_defines p_skills[];
extern struct class_skill_defines r_skills[];
extern struct class_skill_defines k_skills[];
extern struct class_skill_defines u_skills[];
extern struct class_skill_defines c_skills[];
extern struct class_skill_defines m_skills[];
extern const char *pc_clss_types[];
extern const char *pc_clss_types2[];
extern const char *pc_clss_types3[];
extern const char *pc_clss_abbrev[];

/* External variables */

extern int max_who;

extern char globalBuf[MAX_STRING_LENGTH];
extern bool wizlock;

extern Character *character_list;
extern Character *combat_list;
extern char *ban_list[30];
extern char *nonew_new_list[30];
extern const char *zone_modes[];
extern const char *equipment_types[];
extern const char *utility_item_types[];

extern int top_of_mobt;
extern const char *action_bits[];
extern const char *affected_bits[];
extern const char *size_bitfields[];
extern char *strs_damage_types[];
extern char *obj_types[];
extern std::vector<std::string> continent_names;

extern const char *drinks[];
extern const char *portal_bits[];
extern const char *player_bits[];
extern const char *combat_bits[];
extern const char *connected_types[];
extern const char *mob_types[];
extern const char *exit_bits[];
extern int mob_race_mod[][5];

extern const char *race_abbrev[];

extern int max_who;
extern int movement_loss[];
extern const char *skills[];
extern const char *ki[];
extern const char *innate_skills[];
extern const char *reserved[];
extern room_t IMM_PIRAHNA_ROOM;
extern char *time_look[];
#endif