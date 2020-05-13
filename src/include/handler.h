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

#include <structs.h> // ubyte, etc..
#include <comm.h>

/* handling the affected-structures */
void affect_total(CHAR_DATA *ch);
void affect_modify(CHAR_DATA *ch, int32 loc, int32 mod, long bitv, bool add, int flag = 0);
void affect_to_char( CHAR_DATA *ch, struct affected_type *af, int32 duration_type = PULSE_TIME);
void affect_from_char( CHAR_DATA *ch, int skill, int flags = 0);
void affect_remove( CHAR_DATA *ch, struct affected_type *af, int flags);
affected_type * affected_by_spell( CHAR_DATA *ch, int skill );
void affect_join( CHAR_DATA *ch, struct affected_type *af,
		  bool avg_dur, bool avg_mod );
affected_type * affected_by_random(CHAR_DATA *ch);


/* flag bit values for affect_remove() */
#define SUPPRESS_CONSEQUENCES	1
#define SUPPRESS_MESSAGES	2
#define SUPPRESS_ALL		(SUPPRESS_CONSEQUENCES | SUPPRESS_MESSAGES)

/* utility */
struct obj_data *create_money( int amount );
char *fname(char *namelist);
int get_max_stat(char_data * ch, ubyte stat);
//TIMERS
bool isTimer(CHAR_DATA *ch, int spell);
void addTimer(CHAR_DATA *ch, int spell, int ticks);
//END TIMERS
/* ******** objects *********** */

int move_obj(obj_data * obj, int dest);
int move_obj(obj_data * obj, char_data * ch);
int move_obj(obj_data * obj, obj_data * dest_obj);

int obj_to_char(struct obj_data *object, CHAR_DATA *ch);
int obj_from_char(struct obj_data *object);

int obj_to_room(struct obj_data *object, int room);
int obj_from_room(struct obj_data *object);

int obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
int obj_from_obj(struct obj_data *obj);

int equip_char(CHAR_DATA *ch, struct obj_data *obj, int pos, int flag =0);
struct obj_data *unequip_char(CHAR_DATA *ch, int pos, int flag = 0);

struct obj_data *get_obj_in_list(char *name, struct obj_data *list);
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name);
struct obj_data *get_obj(int vnum);
struct obj_data *get_obj_num(int nr);

void object_list_new_new_owner(struct obj_data *list, CHAR_DATA *ch);

void extract_obj(struct obj_data *obj);

/* ******* characters ********* */

CHAR_DATA *get_char_room(char *name, int room, bool careful = FALSE);
CHAR_DATA *get_char_num(int nr);
CHAR_DATA *get_char(const char *name);
CHAR_DATA *get_mob(char *name);

CHAR_DATA *get_pc(char *name);

int char_from_room(CHAR_DATA *ch, bool stop_all_fighting);
int char_from_room(CHAR_DATA *ch);
int  char_to_room(CHAR_DATA *ch, int room, bool stop_all_action);
int  char_to_room(CHAR_DATA *ch, int room);

/* find if character can see */
CHAR_DATA *get_active_pc_vis(CHAR_DATA *ch, char *name);
CHAR_DATA *get_active_pc(const char *name);
CHAR_DATA *get_all_pc(char *name);
CHAR_DATA *get_char_room_vis(CHAR_DATA *ch, char *name);
CHAR_DATA *get_rand_other_char_room_vis(CHAR_DATA *ch);
CHAR_DATA *get_char_vis(CHAR_DATA *ch, char *name);
CHAR_DATA *get_pc_vis(CHAR_DATA *ch, char *name);
CHAR_DATA *get_pc_vis_exact(CHAR_DATA *ch, char *name);
CHAR_DATA *get_mob_vis(CHAR_DATA *ch, char *name);
CHAR_DATA *get_random_mob_vnum(int vnum);
CHAR_DATA *get_mob_room_vis(CHAR_DATA *ch, char *name);
CHAR_DATA *get_mob_vnum(int vnum);
OBJ_DATA *get_obj_vnum(int vnum);
struct obj_data *get_obj_in_list_vis(CHAR_DATA *ch, char *name, 
		struct obj_data *list, bool bf = FALSE);
struct obj_data *get_obj_in_list_vis(CHAR_DATA *ch, int item_num, 
		struct obj_data *list, bool bf = FALSE);
struct obj_data *get_obj_vis(CHAR_DATA *ch, char *name, bool loc = FALSE);

void extract_char(CHAR_DATA *ch, bool pull);
/* wiz_102.cpp */
int find_skill_num(char *name);

/* Generic Find */

int generic_find(char *arg, int bitvector, CHAR_DATA *ch,
		   CHAR_DATA **tar_ch, struct obj_data **tar_obj);

int get_number(char **name);

#define FIND_CHAR_ROOM      1
#define FIND_CHAR_WORLD     2
#define FIND_OBJ_INV        4
#define FIND_OBJ_ROOM       8
#define FIND_OBJ_WORLD     16
#define FIND_OBJ_EQUIP     32

bool is_wearing(CHAR_DATA *ch, OBJ_DATA *item);

class ErrorHandler {
 public:
  class underrun {};
  class overrun {};
};

void produce_coredump(void *ptr = 0);

bool objExists(OBJ_DATA *obj);
bool charge_moves(char_data *ch, int skill, double modifier = 1);

void die_follower(CHAR_DATA *ch);
void remove_from_bard_list(char_data * ch);
void stop_guarding_me(char_data * victim);
void stop_guarding(char_data * guard);

#endif

