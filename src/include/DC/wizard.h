#ifndef WIZARD_H_
#define WIZARD_H_
/************************************************************************
| $Id: wizard.h,v 1.8 2012/02/08 22:55:55 jhhudso Exp $
| wizard.h
| Description:  This is NOT a global include file, it's used only
|   for the wiz_1*.C files to consolidate the header files they
|   need.
*/
#include <qtypes.h>     // for quint64
#include "DC/DC.h"      // for obj_list_t, room_t, zone_t
#include "DC/dcstdio.h" // for FILEPtr
#include "DC/interp.h"  // for command_return_t
class Character;
class Zone;

/* Function headers */
void display_punishes(Character *ch, Character *vict);
char *str_str(char *first, char *second);
void setup_dir(FILEPtr fl, room_t room, int dir);
int real_roomb(int virt);
void save_ban_list(void);
void save_nonew_new_list(void);
int is_in_range(Character *ch, int virt);
void isr_set(Character *ch);
command_return_t mob_stat(Character *ch, Character *k);
void obj_stat(Character *ch, class Object *j);
int number_or_name(char **name, int *num);
int mob_in_index(char *name, int index);
int obj_in_index(char *name, int index);
void do_oload(Character *ch, int rnum, int cnt, bool random = false);
void do_mload(Character *ch, int rnum, int cnt);
void colorCharSend(char *s, Character *ch);
obj_list_t oload(Character *ch, int rnum, int cnt, bool random);
int show_zone_commands(Character *ch, const Zone &zone, quint64 start = 0, quint64 num_to_show = 0, bool stats = false);
int show_zone_commands(Character *ch, zone_t zone_key, quint64 start = 0, quint64 num_to_show = 0, bool stats = false);

/* Our own constants */
const int MAX_MESSAGE_LENGTH = 4096;

#endif
