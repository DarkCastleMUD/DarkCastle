#ifndef WIZARD_H_
#define WIZARD_H_
/************************************************************************
| $Id: wizard.h,v 1.4 2004/05/18 00:17:47 urizen Exp $
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

#include <character.h>
#include <act.h>

#include <levels.h>
#include <utility.h>
#include <mobile.h>
#include <interp.h>
#include <room.h>
#include <handler.h>
#include <obj.h>
#include <interp.h>
#include <terminal.h>
#include <player.h>
#include <connect.h>
#include <time.h>
#include <db.h>

/* External variables */
extern CWorld world;
extern int max_who;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern int top_of_world;
extern struct descriptor_data *descriptor_list;
extern char globalBuf[MAX_STRING_LENGTH];
extern bool   wizlock;
extern struct race_shit race_info[];
extern struct descriptor_data *descriptor_list;
extern CHAR_DATA *character_list;
extern CHAR_DATA *combat_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern char *ban_list[30];
extern char *nonew_new_list[30];


/* Function headers */
void display_punishes(CHAR_DATA *ch, CHAR_DATA
 *vict);
char *str_str(char *first, char *second);
void setup_dir(FILE *fl, int room, int dir);
int str_cmp(char *arg1, char *arg2);
struct time_info_data age(CHAR_DATA *ch);
CHAR_DATA *get_pc_vis(CHAR_DATA *ch, char *name);
CHAR_DATA *get_pc_vis_exact(CHAR_DATA *ch, char *name);
void update_wizlist(CHAR_DATA *ch);
int real_roomb(int virt);
void string_to_file(FILE *f, char *string);
void save_ban_list(void);
void save_nonew_new_list(void);
void sprinttype(int type, char *names[], char *result);
void remove_memory(CHAR_DATA *ch, char type);
int is_in_range(CHAR_DATA *ch, int virt);
int create_one_room(CHAR_DATA *ch, int vnum);
void sprintbit(long vektor, char *names[], char *result);
int mana_gain(CHAR_DATA *ch);
int hit_gain(CHAR_DATA *ch);
int move_gain(CHAR_DATA *ch);
void reset_zone(int zone);
void isr_set(CHAR_DATA *ch);
CHAR_DATA *get_pc(char *name);
void mob_stat(CHAR_DATA *ch, CHAR_DATA *k);
void obj_stat(CHAR_DATA *ch, struct obj_data *j);
int number_or_name(char **name, int *num);
int mob_in_index(char *name, int index);
int obj_in_index(char *name, int index);
void do_oload(CHAR_DATA *ch, int rnum, int cnt);
void do_mload(CHAR_DATA *ch, int rnum, int cnt);
void colorCharSend(char* s, struct char_data* ch);


/* Our own constants */
const int MAX_MESSAGE_LENGTH = 4096;

#endif
