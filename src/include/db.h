/***************************************************************************
 *  file: db.h , Database module.                          Part of DIKUMUD *
 *  Usage: Loading/Saving chars booting world.                             *
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
/* $Id: db.h,v 1.10 2004/05/31 11:57:18 urizen Exp $ */
#ifndef DB_H_
#define DB_H_

extern "C" {
  #include <stdio.h>
  #include <time.h>
}

#include <character.h>

#define WORLD_MAX_ROOM        50000  // should never get this high...
                                     // it's just to keep builders/imps from 
                                     // doing a 'goto 1831919131928' and 
                                     // creating it

#define MAX_ZONE    165 
#define MAX_INDEX   4000
#define MAX_RESET  16383
#ifndef LONG_MAX
#define LONG_MAX    2147483467
#define LONG_MIN    -2147483467
#endif

/* Zone Flag Bits */

#define ZONE_NO_TELEPORT   1
#define ZONE_IS_TOWN       1<<1  // Keep out the really bad baddies that are STAY_NO_TOWN
#define ZONE_MODIFIED      1<<2
#define ZONE_UNUSED        1<<3
// Remember to update const.C  zone_bits[] if you change this


#define VERSION_NUMBER    2     /* used for changing pfile format */

/* ban system stuff */

#define BAN_NOT        0
#define BAN_NEW        1
#define BAN_SELECT     2
#define BAN_ALL        3

#define BANNED_FILE   "banned"

#define BANNED_SITE_LENGTH 100 

struct ban_list_element {
   char site[BANNED_SITE_LENGTH + 1];
   int type;
   time_t date;
   char name[100];
   struct ban_list_element *next;
};


/* public procedures in db.c */
void set_zone_modified_zone(long room);
void set_zone_saved_zone(long room);
void set_zone_modified_world(long room);
void set_zone_saved_world(long room);
void set_zone_modified_mob(long room);
void set_zone_saved_mob(long room);
void set_zone_modified_obj(long room);
void set_zone_saved_obj(long room);
bool can_modify_this_room(char_data * ch, long room);
bool can_modify_room(char_data * ch, long room);
bool can_modify_mobile(char_data * ch, long room);
bool can_modify_object(char_data * ch, long room);
void write_one_zone(FILE * fl, int zon);
void write_one_room(FILE * fl, int nr);
void write_mobile(char_data * mob, FILE * fl);
void write_object(obj_data * obj, FILE * fl);
void load_emoting_objects(void);
void boot_db(void);
int  create_entry(char *name);
void zone_update(void);
void init_char(CHAR_DATA *ch);
void clear_char(CHAR_DATA *ch);
void clear_object(struct obj_data *obj);
void reset_char(CHAR_DATA *ch);
void free_char(CHAR_DATA *ch);
int  real_room(int virt);
char *fread_string(FILE *fl, int hasher);
int create_blank_item(int nr);
int create_blank_mobile(int nr);
void delete_item_from_index(int nr);
void delete_mob_from_index(int nr);
int  real_object(int virt);
int  real_mobile(int virt);
int  fread_int(FILE *fl, long minval, long maxval);
char fread_char (FILE *fl);
extern struct skill_quest *skill_list; 
#define REAL 0
#define VIRTUAL 1

struct obj_data  *read_object(int nr, FILE *fl);
CHAR_DATA *read_mobile(int nr, FILE *fl);
struct obj_data  *clone_object(int nr);
CHAR_DATA *clone_mobile(int nr);


extern time_t start_time; /* mud start time */

struct pulse_data { /* list for keeping tract of 'pulsing' chars */
    char_data * thechar;
    pulse_data * next;
};

/* structure for the reset commands */
struct reset_com
{
    char command;   /* current command                      */ 
    int if_flag;    // 0=always 1=if prev exe'd  2=if prev DIDN'T exe   3=ONLY on reboot
    int arg1;
    int arg2;
    int arg3;
    char * comment; /* Any comments that went with the command */

    /* 
    *  Commands:              *
    *  'M': Read a mobile     *
    *  'O': Read an object    *
    *  'P': Put obj in obj    *
    *  'G': Obj to char       *
    *  'E': Obj to char equip *
    *  'D': Set state of door *
    *  '%': arg1 in arg2 chance of being true *
    *       (this is used for putting a %chance on next command *
    */
};



/* zone definition structure. for the 'zone-table'   */
struct zone_data
{
    char *name;             /* name of this zone                  */
    int lifespan;           /* how long between resets (minutes)  */
    int age;                /* current age of ths zone (minutes) */
    int top;                /* upper limit for room vnums in this zone */
    int bottom_rnum;
    int top_rnum;
    long zone_flags;        /* flags for the entire zone eg: !teleport */

    int players;            // Number of PCs in the zone

    char * filename;        /* name of the file this zone is kept in */
    
    int reset_mode;         /* conditions for reset (see below)   */

    struct reset_com *cmd;  /* command table for reset             */
    int reset_total;        /* total number item in currently allocated
                             * reset_com array.  This is used in the 
                             * do_zedit command so we don't have to realloc
                             * every time we add/delete a command
                             */
    /*
    *  Reset mode:                              *
    *  0: Don't reset, and don't update age.    *
    *  1: Reset if no PC's are located in zone. *
    *  2: Just reset.                           *
    *  Update char * zone_modes[] (const.C) if you change this *
    */

    int num_mob_first_repop; // number of mobs in this zone that were repoped in first repop
    int num_mob_on_repop;    // number of mobs in this zone that were repoped in last repop
    int death_counter;       // +- counter for how often mobs in zone are killed
    int counter_mod;         // how quickly mobs are taken off the death_counter
    int died_this_tick;      // number of mobs that have died in this zone this pop
};




/* element in monster and object index-tables   */
struct index_data
{
    int virt;    /* virt number of ths mob/obj           */
    int number;     /* number of existing units of ths mob/obj */
    int (*non_combat_func)(CHAR_DATA*, struct obj_data *, int, char*, CHAR_DATA*); // non Combat special proc
    int (*combat_func)(CHAR_DATA*, struct obj_data *, int, char*, CHAR_DATA*); // combat special proc
    void *item;     /* the mobile/object itself                 */

    // TODO - clean this up so it's only in the MOB index data instead of both
    MPROG_DATA *        mobprogs;
    MPROG_DATA *	mobspec;
    int			progtypes;
};


struct help_index_element
{
    char *keyword;
    long pos;
};

extern int exp_table[61+1];

#define WORLD_FILE_MODIFIED        1
#define WORLD_FILE_IN_PROGRESS     1<<1
#define WORLD_FILE_READY           1<<2
#define WORLD_FILE_APPROVED        1<<3

struct world_file_list_item
{
    char * filename;
    long firstnum;
    long lastnum;
    long flags;
    world_file_list_item * next;
};

// The CWorld class, to control the world a bit better.

class CWorld
{
public:
   room_data & operator[](int rnum);
};

#endif
