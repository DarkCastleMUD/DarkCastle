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

#include <qstring.h>
#include <qtypes.h>
#include <stdint.h>
#include <map>
#include <string>

#include "DC/structs.h" // uint8_t, etc..
#include "DC/comm.h"
#include "DC/Trace.h"
#include "DC/DC.h"
#include "DC/Index.h"

class Character;
class Object;
class affected_type;
enum class attribute_t : uint_fast8_t;

/* handling the affected-structures */
void affect_total(CharacterPtr ch);
void affect_modify(CharacterPtr ch, int32_t loc, int32_t mod, int32_t bitv, bool add, int flag = 0);
void affect_to_char(CharacterPtr ch, affected_type *af, int32_t duration_type = DC::PULSE_TIME);
void affect_from_char(CharacterPtr ch, int skill, int flags = 0);
void affect_remove(CharacterPtr ch, affected_type *af, int flags);
void affect_join(CharacterPtr ch, affected_type *af, bool avg_dur, bool avg_mod);
affected_type *affected_by_random(CharacterPtr ch);

/* flag bit values for affect_remove() */
#define SUPPRESS_CONSEQUENCES 1
#define SUPPRESS_MESSAGES 2
#define SUPPRESS_ALL (SUPPRESS_CONSEQUENCES | SUPPRESS_MESSAGES)

/* utility */
ObjectPtr create_money(int amount);
QString fname(QString namelist);
int isprefix(const char *str, const char *namel);
int get_max_stat(CharacterPtr ch, attribute_t stat);
// TIMERS
bool isTimer(CharacterPtr ch, int spell);
void addTimer(CharacterPtr ch, int spell, int ticks);
// END TIMERS
/* ******** objects *********** */

int move_obj(ObjectPtr obj, int dest);
int move_obj(ObjectPtr obj, CharacterPtr ch);
int move_obj(ObjectPtr obj, ObjectPtr dest_obj);

int obj_to_char(ObjectPtr object, CharacterPtr ch);
int obj_from_char(ObjectPtr object);

int obj_to_room(ObjectPtr object, room_t room);
int obj_from_room(ObjectPtr object);

int obj_to_obj(ObjectPtr obj, ObjectPtr obj_to);
int obj_from_obj(ObjectPtr obj);

ObjectPtr get_obj_in_list(char *name, ObjectPtr list);
ObjectPtr get_obj_in_list_num(int num, ObjectPtr list);
ObjectPtr get_obj(char *name);
ObjectPtr get_obj(int vnum);
ObjectPtr get_obj_num(int nr);

void object_list_new_new_owner(ObjectPtr list, CharacterPtr ch);

void extract_obj(ObjectPtr obj);

/* ******* characters ********* */

CharacterPtr get_char_room(QString name, room_t room, bool careful = false);
CharacterPtr get_char_room(const char *name, room_t room, bool careful = false);

CharacterPtr get_char_num(int nr);
CharacterPtr get_char(QString name);
CharacterPtr get_mob(char *name);
CharacterPtr get_pc(QString name);

int char_from_room(CharacterPtr ch, bool stop_all_fighting);
int char_from_room(CharacterPtr ch);
int char_to_room(CharacterPtr ch, room_t room, bool stop_all_fighting = true);

/* find if character can see */
CharacterPtr get_active_pc(const char *name);
CharacterPtr get_active_pc(QString name);
CharacterPtr get_all_pc(char *name);
CharacterPtr get_char_room_vis(CharacterPtr ch, QString name);
CharacterPtr get_char_vis(CharacterPtr ch, const char *name);
CharacterPtr get_char_vis(CharacterPtr ch, const std::string &name);
CharacterPtr get_char_vis(CharacterPtr ch, const QString &name);
CharacterPtr get_pc_vis(CharacterPtr ch, const char *name);
CharacterPtr get_pc_vis(CharacterPtr ch, QString name);
CharacterPtr get_pc_vis_exact(CharacterPtr ch, const char *name);
CharacterPtr get_mob_vis(CharacterPtr ch, char *name);
CharacterPtr get_random_mob_vnum(int vnum);
CharacterPtr get_mob_room_vis(CharacterPtr ch, const char *name);
CharacterPtr get_mob_vnum(int vnum);
ObjectPtr get_obj_vnum(int vnum);
ObjectPtr get_obj_vnum(QString vnum);
ObjectPtr get_objindex_vnum(vnum_t vnum);
ObjectPtr get_objindex_vnum(QString vnum);
vnum_t get_vnum(QString vnum_str);
ObjectPtr get_obj_in_list_vis(CharacterPtr ch, QString name, ObjectPtr list, bool bf = false);
ObjectPtr get_obj_in_list_vis(CharacterPtr ch, int item_num, ObjectPtr list, bool bf = false);
ObjectPtr get_obj_vis(CharacterPtr ch, const char *name, bool loc = false);
ObjectPtr get_obj_vis(CharacterPtr ch, std::string name, bool loc = false);

void extract_char(CharacterPtr ch, bool pull, Trace t = Trace("unknown"));
/* wiz_102.cpp */
int find_skill_num(char *name);

typedef QMap<QString, quint64> skill_results_t;
skill_results_t find_skills_by_name(QString name);
CharacterPtr get_pc_vis(CharacterPtr ch, std::string name);

/* Generic Find */

int generic_find(const char *arg, int bitvector, CharacterPtr ch, CharacterPtr *tar_ch, ObjectPtr *tar_obj, bool verbose = false);

int get_number(char **name);
int get_number(std::string &name);
int get_number(QString &name);

#define FIND_CHAR_ROOM 1
#define FIND_CHAR_WORLD 2
#define FIND_OBJ_INV 4
#define FIND_OBJ_ROOM 8
#define FIND_OBJ_WORLD 16
#define FIND_OBJ_EQUIP 32

bool is_wearing(CharacterPtr ch, ObjectPtr item);

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

bool objExists(ObjectPtr obj);
bool charge_moves(CharacterPtr ch, int skill, double modifier = 1);

void die_follower(CharacterPtr ch);
void stop_guarding_me(CharacterPtr victim);
void stop_guarding(CharacterPtr guard);
void remove_memory(CharacterPtr ch, char type);
void remove_memory(CharacterPtr ch, char type, CharacterPtr vict);

#endif
