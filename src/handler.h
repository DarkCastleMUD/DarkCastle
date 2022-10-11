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

using namespace std;

/* handling the affected-structures */
void affect_total(char_data *ch);
void affect_modify(char_data *ch, int32_t loc, int32_t mod, int32_t bitv, bool add, int flag = 0);
void affect_to_char( char_data *ch, struct affected_type *af, int32_t duration_type = PULSE_TIME);
void affect_from_char( char_data *ch, int skill, int flags = 0);
void affect_remove( char_data *ch, struct affected_type *af, int flags);
affected_type * affected_by_spell( char_data *ch, int skill );
void affect_join( char_data *ch, struct affected_type *af,
		  bool avg_dur, bool avg_mod );
affected_type * affected_by_random(char_data *ch);


/* flag bit values for affect_remove() */
#define SUPPRESS_CONSEQUENCES	1
#define SUPPRESS_MESSAGES	2
#define SUPPRESS_ALL		(SUPPRESS_CONSEQUENCES | SUPPRESS_MESSAGES)

/* utility */
struct obj_data *create_money( int amount );
char *fname(char *namelist);
int get_max_stat(char_data * ch, uint8_t stat);
//TIMERS
bool isTimer(char_data *ch, int spell);
void addTimer(char_data *ch, int spell, int ticks);
//END TIMERS
/* ******** objects *********** */

int move_obj(obj_data * obj, int dest);
int move_obj(obj_data * obj, char_data * ch);
int move_obj(obj_data * obj, obj_data * dest_obj);

int obj_to_char(struct obj_data *object, char_data *ch);
int obj_from_char(struct obj_data *object);

int obj_to_room(struct obj_data *object, int room);
int obj_from_room(struct obj_data *object);

int obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
int obj_from_obj(struct obj_data *obj);

int equip_char(char_data *ch, struct obj_data *obj, int pos, int flag =0);
struct obj_data *unequip_char(char_data *ch, int pos, int flag = 0);

struct obj_data *get_obj_in_list(char *name, struct obj_data *list);
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name);
struct obj_data *get_obj(int vnum);
struct obj_data *get_obj_num(int nr);

void object_list_new_new_owner(struct obj_data *list, char_data *ch);

void extract_obj(struct obj_data *obj);

/* ******* characters ********* */

char_data *get_char_room(char *name, int room, bool careful = FALSE);
char_data *get_char_num(int nr);
char_data *get_char(string name);
char_data *get_mob(char *name);

char_data *get_pc(char *name);

int char_from_room(char_data *ch, bool stop_all_fighting);
int char_from_room(char_data *ch);
int  char_to_room(char_data *ch, int room, bool stop_all_action);
int  char_to_room(char_data *ch, int room);

/* find if character can see */
char_data *get_active_pc_vis(char_data *ch, const char *name);
char_data *get_active_pc(const char *name);
char_data *get_all_pc(char *name);
char_data *get_char_room_vis(char_data *ch, const char *name);
char_data *get_char_room_vis(char_data *ch, string name);
char_data *get_rand_other_char_room_vis(char_data *ch);
char_data *get_char_vis(char_data *ch, const char *name);
char_data *get_char_vis(char_data *ch, const string& name);
char_data *get_pc_vis(char_data *ch, const char *name);
char_data *get_pc_vis_exact(char_data *ch, const char *name);
char_data *get_mob_vis(char_data *ch, char *name);
char_data *get_random_mob_vnum(int vnum);
char_data *get_mob_room_vis(char_data *ch, char *name);
char_data *get_mob_vnum(int vnum);
obj_data *get_obj_vnum(int vnum);
struct obj_data *get_obj_in_list_vis(char_data *ch, const char *name, 
		struct obj_data *list, bool bf = FALSE);
struct obj_data *get_obj_in_list_vis(char_data *ch, int item_num, 
		struct obj_data *list, bool bf = FALSE);
struct obj_data *get_obj_vis(char_data *ch, char *name, bool loc = false);
struct obj_data *get_obj_vis(char_data *ch, string name, bool loc = false);

void extract_char(char_data *ch, bool pull, Trace t = Trace("unknown"));
/* wiz_102.cpp */
int find_skill_num(char *name);


typedef map<string, uint64_t> skill_results_t;
skill_results_t find_skills_by_name(string name);
char_data *get_pc_vis(char_data *ch, string name);

/* Generic Find */

int generic_find(const char *arg, int bitvector, char_data *ch, char_data **tar_ch, struct obj_data **tar_obj, bool verbose = false);

int get_number(char **name);
int get_number(string & name);

#define FIND_CHAR_ROOM      1
#define FIND_CHAR_WORLD     2
#define FIND_OBJ_INV        4
#define FIND_OBJ_ROOM       8
#define FIND_OBJ_WORLD     16
#define FIND_OBJ_EQUIP     32

bool is_wearing(char_data *ch, obj_data *item);

class ErrorHandler {
 public:
  class underrun {};
  class overrun {};
};

bool objExists(obj_data *obj);
bool charge_moves(char_data *ch, int skill, double modifier = 1);

void die_follower(char_data *ch);
void remove_from_bard_list(char_data * ch);
void stop_guarding_me(char_data * victim);
void stop_guarding(char_data * guard);
void remove_memory(char_data *ch, char type);
void remove_memory(char_data *ch, char type, char_data *vict);
void add_memory(char_data *ch, char *victim, char type);

#endif

