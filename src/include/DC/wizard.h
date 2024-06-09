#ifndef WIZARD_H_
#define WIZARD_H_
/************************************************************************
| $Id: wizard.h,v 1.8 2012/02/08 22:55:55 jhhudso Exp $
| wizard.h
| Description:  This is NOT a global include file, it's used only
|   for the wiz_1*.C files to consolidate the header files they
|   need.
*/

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>

#include "DC/character.h"
#include "DC/act.h"

#include "DC/levels.h"
#include "DC/utility.h"
#include "DC/mobile.h"
#include "DC/interp.h"
#include "DC/room.h"
#include "DC/handler.h"
#include "DC/interp.h"
#include "DC/terminal.h"
#include "DC/player.h"
#include "DC/connect.h"
#include "ctime"
#include "DC/db.h"

/* Function headers */
void display_punishes(Character *ch, Character
                                         *vict);
char *str_str(char *first, char *second);
void setup_dir(FILE *fl, int room, int dir);
void update_wizlist(Character *ch);
int real_roomb(int virt);
void save_ban_list(void);
void save_nonew_new_list(void);
int is_in_range(Character *ch, int virt);
int create_one_room(Character *ch, int vnum);
void isr_set(Character *ch);
void mob_stat(Character *ch, Character *k);
void obj_stat(Character *ch, class Object *j);
int number_or_name(char **name, int *num);
int mob_in_index(char *name, int index);
int obj_in_index(char *name, int index);
void do_oload(Character *ch, int rnum, int cnt, bool random = false);
void do_mload(Character *ch, int rnum, int cnt);
void colorCharSend(char *s, Character *ch);
obj_list_t oload(Character *ch, int rnum, int cnt, bool random);
int show_zone_commands(Character *ch, const Zone &zone, uint64_t start = 0, uint64_t num_to_show = 0, bool stats = false);
int show_zone_commands(Character *ch, zone_t zone_key, uint64_t start = 0, uint64_t num_to_show = 0, bool stats = false);

/* Our own constants */
const int MAX_MESSAGE_LENGTH = 4096;

#endif
