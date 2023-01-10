#ifndef WIZARD_H_
#define WIZARD_H_
/************************************************************************
| $Id: wizard.h,v 1.8 2012/02/08 22:55:55 jhhudso Exp $
| wizard.h
| Description:  This is NOT a global include file, it's used only
|   for the wiz_1*.C files to consolidate the header files they
|   need.
*/
extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
}

#include "character.h"
#include "act.h"

#include "levels.h"
#include "utility.h"
#include "mobile.h"
#include "interp.h"
#include "room.h"
#include "handler.h"
#include "obj.h"
#include "interp.h"
#include "terminal.h"
#include "player.h"
#include "connect.h"
#include "time.h"
#include "db.h"

/* Function headers */
void display_punishes(char_data *ch, char_data
                                         *vict);
char *str_str(char *first, char *second);
void setup_dir(FILE *fl, int room, int dir);
struct time_info_data age(char_data *ch);
char_data *get_pc_vis(char_data *ch, const char *name);
char_data *get_pc_vis_exact(char_data *ch, const char *name);
void update_wizlist(char_data *ch);
int real_roomb(int virt);
void save_ban_list(void);
void save_nonew_new_list(void);
int is_in_range(char_data *ch, int virt);
int create_one_room(char_data *ch, int vnum);
int mana_gain(char_data *ch);
int hit_gain(char_data *ch);
int move_gain(char_data *ch, int extra);
void isr_set(char_data *ch);
char_data *get_pc(char *name);
void mob_stat(char_data *ch, char_data *k);
void obj_stat(char_data *ch, struct obj_data *j);
int number_or_name(char **name, int *num);
int mob_in_index(char *name, int index);
int obj_in_index(char *name, int index);
void do_oload(char_data *ch, int rnum, int cnt, bool random = false);
void do_mload(char_data *ch, int rnum, int cnt);
void colorCharSend(char *s, char_data *ch);
obj_list_t oload(char_data *ch, int rnum, int cnt, bool random);
int show_zone_commands(char_data *ch, int i, int start = 0);

/* Our own constants */
const int MAX_MESSAGE_LENGTH = 4096;

#endif
