/***************************************************************************
 *  file: handler.h , Handler module.                      Part of DIKUMUD *
 *  Usage: Various routines for moving about objects/players               *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: handler.h,v 1.33 2012/01/17 01:03:26 jhhudso Exp $ */

#ifndef HANDLER_H_
#define HANDLER_H_

#include <map>

#include "structs.h" // uint8_t, etc..
#include "comm.h"
#include "Trace.h"

/* handling the affected-structures */
void affect_total(Character *ch);
void affect_modify(Character *ch, int32_t loc, int32_t mod, int32_t bitv, bool add, int flag = 0);
void affect_to_char(Character *ch, struct affected_type *af, int32_t duration_type = DC::PULSE_TIME);
void affect_from_char(Character *ch, int skill, int flags = 0);
void affect_remove(Character *ch, struct affected_type *af, int flags);
affected_type *affected_by_spell(Character *ch, uint32_t skill);
void affect_join(Character *ch, struct affected_type *af,
				 bool avg_dur, bool avg_mod);
affected_type *affected_by_random(Character *ch);

/* flag bit values for affect_remove() */
#define SUPPRESS_CONSEQUENCES 1
#define SUPPRESS_MESSAGES 2
#define SUPPRESS_ALL (SUPPRESS_CONSEQUENCES | SUPPRESS_MESSAGES)

/* utility */
class Object *create_money(int amount);
QString fname(QString namelist);
int get_max_stat(Character *ch, attribute_t stat);
// TIMERS
bool isTimer(Character *ch, int spell);
void addTimer(Character *ch, int spell, int ticks);
// END TIMERS
/* ******** objects *********** */

int move_obj(Object *obj, int dest);
int move_obj(Object *obj, Character *ch);
int move_obj(Object *obj, Object *dest_obj);

int obj_to_char(class Object *object, Character *ch);
int obj_from_char(class Object *object);

int obj_to_room(class Object *object, int room);
int obj_from_room(class Object *object);

int obj_to_obj(class Object *obj, class Object *obj_to);
int obj_from_obj(class Object *obj);

int equip_char(Character *ch, class Object *obj, int pos, int flag = 0);
class Object *unequip_char(Character *ch, int pos, int flag = 0);

class Object *get_obj_in_list(char *name, class Object *list);
class Object *get_obj_in_list_num(int num, class Object *list);
class Object *get_obj(char *name);
class Object *get_obj(int vnum);
class Object *get_obj_num(int nr);

void object_list_new_new_owner(class Object *list, Character *ch);

void extract_obj(class Object *obj);

/* ******* characters ********* */

Character *get_char_room(QString name, room_t room, bool careful = false);
Character *get_char_room(const char *name, room_t room, bool careful = false);

Character *get_char_num(int nr);
Character *get_char(QString name);
Character *get_mob(char *name);

Character *get_pc(QString name);

int char_from_room(Character *ch, bool stop_all_fighting);
int char_from_room(Character *ch);
int char_to_room(Character *ch, room_t room, bool stop_all_fighting = true);

/* find if character can see */
Character *get_active_pc(const char *name);
Character *get_active_pc(QString name);
Character *get_all_pc(char *name);
Character *get_char_room_vis(Character *ch, QString name);
Character *get_char_vis(Character *ch, const char *name);
Character *get_char_vis(Character *ch, const std::string &name);
Character *get_pc_vis(Character *ch, const char *name);
Character *get_pc_vis(Character *ch, QString name);
Character *get_pc_vis_exact(Character *ch, const char *name);
Character *get_mob_vis(Character *ch, char *name);
Character *get_random_mob_vnum(int vnum);
Character *get_mob_room_vis(Character *ch, char *name);
Character *get_mob_vnum(int vnum);
Object *get_obj_vnum(int vnum);
class Object *get_obj_in_list_vis(Character *ch, QString name, class Object *list, bool bf = false);
class Object *get_obj_in_list_vis(Character *ch, int item_num, class Object *list, bool bf = false);
class Object *get_obj_vis(Character *ch, const char *name, bool loc = false);
class Object *get_obj_vis(Character *ch, std::string name, bool loc = false);

void extract_char(Character *ch, bool pull, Trace t = Trace("unknown"));
/* wiz_102.cpp */
int find_skill_num(char *name);

typedef std::map<std::string, uint64_t> skill_results_t;
skill_results_t find_skills_by_name(std::string name);
Character *get_pc_vis(Character *ch, std::string name);

/* Generic Find */

int generic_find(const char *arg, int bitvector, Character *ch, Character **tar_ch, class Object **tar_obj, bool verbose = false);

int get_number(char **name);
int get_number(std::string &name);
int get_number(QString &name);

#define FIND_CHAR_ROOM 1
#define FIND_CHAR_WORLD 2
#define FIND_OBJ_INV 4
#define FIND_OBJ_ROOM 8
#define FIND_OBJ_WORLD 16
#define FIND_OBJ_EQUIP 32

bool is_wearing(Character *ch, Object *item);

class ErrorHandler
{
public:
	class underrun
	{
	};
	class overrun
	{
	};
};

bool objExists(Object *obj);
bool charge_moves(Character *ch, int skill, double modifier = 1);

void die_follower(Character *ch);
void remove_from_bard_list(Character *ch);
void stop_guarding_me(Character *victim);
void stop_guarding(Character *guard);
void remove_memory(Character *ch, char type);
void remove_memory(Character *ch, char type, Character *vict);

#endif
