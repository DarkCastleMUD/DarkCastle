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
/* $Id: db.h,v 1.40 2012/02/08 22:54:25 jhhudso Exp $ */
#ifndef DB_H_
#define DB_H_

#include <stdio.h>
#include <time.h>

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

#include <QDateTime>

#include "character.h"
#include "weather.h"
#include "handler.h"
#include "DC.h"

using namespace std;

extern struct obj_data *object_list;
extern int top_of_world;

struct index_data;
struct error_eof
{
};
struct error_negative_int
{
};
struct error_range_int
{
};
struct error_range_under
{
};
struct error_range_over
{
};

const uint64_t WORLD_MAX_ROOM = 50000; // should never get this high...
                                       // it's just to keep builders/imps from
                                       // doing a 'goto 1831919131928' and
                                       // creating it

const int VERSION_NUMBER = 2; /* used for changing pfile format */

/* ban system stuff */

#define BAN_NOT 0
#define BAN_NEW 1
#define BAN_SELECT 2
#define BAN_ALL 3

#define BANNED_FILE "banned"

#define BANNED_SITE_LENGTH 100

struct ban_list_element
{
  char site[BANNED_SITE_LENGTH + 1];
  int type;
  time_t date;
  char name[100];
  struct ban_list_element *next;
};

extern std::vector<std::string> continent_names;

enum Continents
{
  NO_CONTINENT = 1,    // 1
  SORPIGAL_CONTINENT,  // 2
  FAR_REACH,           // 3
  DIAMOND_ISLE,        // 4
  UNDERDARK,           // 5
  BEHIND_THE_MIRROR,   // 6
  PLANES_OF_EXISTANCE, // 7
  FORBIDDEN_ISLAND,    // 8
  OTHER_CONTINENT,     // 9
  MAX_CONTINENTS       // for iteration
};

/* public procedures in db.c */
void set_zone_modified_zone(int32_t room);
void set_zone_saved_zone(int32_t room);
void set_zone_modified_world(int32_t room);
void set_zone_saved_world(int32_t room);
void set_zone_modified_mob(int32_t room);
void set_zone_saved_mob(int32_t room);
void set_zone_modified_obj(int32_t room);
void set_zone_saved_obj(int32_t room);
bool can_modify_this_room(Character *ch, int32_t room);
bool can_modify_room(Character *ch, int32_t room);
bool can_modify_mobile(Character *ch, int32_t room);
bool can_modify_object(Character *ch, int32_t room);

void write_one_room(FILE *fl, int nr);
void write_mobile(Character *mob, FILE *fl);
void write_object(obj_data *obj, FILE *fl);
void load_emoting_objects(void);
int create_entry(char *name);
void zone_update(void);
void init_char(Character *ch);
void clear_char(Character *ch);
void clear_object(struct obj_data *obj);
void reset_char(Character *ch);
void free_char(Character *ch, Trace trace = Trace("Unknown"));
room_t real_room(room_t virt);
char *fread_string(FILE *fl, int hasher);
char *fread_string(ifstream &in, int hasher);
char *fread_word(FILE *, int);
int create_blank_item(int nr);
int create_blank_mobile(int nr);
void delete_item_from_index(int nr);
void delete_mob_from_index(int nr);
int real_object(int virt);
int real_mobile(int virt);
int64_t fread_int(FILE *fl, int64_t minval, int64_t maxval);
int64_t fread_int(ifstream &in, int64_t beg_range, int64_t end_range);
uint64_t fread_uint(FILE *fl, uint64_t minval, uint64_t maxval);
char fread_char(FILE *fl);
void add_mobspec(int i);
void write_object_csv(obj_data *obj, ofstream &fout);
index_data *generate_obj_indices(int *top, index_data *index);
index_data *generate_mob_indices(int *top, struct index_data *index);

extern struct skill_quest *skill_list;
extern index_data mob_index_array[MAX_INDEX];
#define REAL 0
#define VIRTUAL 1

struct obj_data *read_object(int nr, FILE *fl, bool zz);
Character *read_mobile(int nr, FILE *fl);
struct obj_data *clone_object(int nr);
Character *clone_mobile(int nr);
void randomize_object(obj_data *obj);
void string_to_file(FILE *f, char *string);
void string_to_file(ofstream &f, char *string);
ofstream &operator<<(ofstream &out, obj_data *obj);
ofstream &operator<<(ofstream &out, QString);
ofstream &operator<<(ofstream &out, const char *);
ofstream &operator<<(ofstream &out, char *);
ifstream &operator>>(ifstream &in, obj_data *obj);
string lf_to_crlf(string &s1);
void copySaveData(obj_data *new_obj, obj_data *obj);
bool verify_item(struct obj_data **obj);
bool fullItemMatch(obj_data *obj, obj_data *obj2);
bool has_random(obj_data *obj);
FILE *legacyFileOpen(QString directory, QString filename, QString error_message);

extern int top_of_objt;
extern time_t start_time; /* mud start time */

struct pulse_data
{ /* list for keeping tract of 'pulsing' chars */
  Character *thechar;
  pulse_data *next;
};

/* structure for the reset commands */
struct reset_com
{
  char command; /* current command                      */
  int if_flag;  // 0=always 1=if prev exe'd  2=if prev DIDN'T exe   3=ONLY on reboot
  int arg1;
  int arg2;
  int arg3;
  char *comment; /* Any comments that went with the command */
  int active;    // is it active? alot aren't on the builders' port
  time_t last;   // when was it last reset
  Character *lastPop;
  time_t lastSuccess;
  uint64_t attempts;
  uint64_t successes;
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

/* element in monster and object index-tables   */
struct index_data
{
  int virt;                                                                               /* virt number of ths mob/obj           */
  int number;                                                                             /* number of existing units of ths mob/obj */
  int (*non_combat_func)(Character *, struct obj_data *, int, const char *, Character *); // non Combat special proc
  int (*combat_func)(Character *, struct obj_data *, int, const char *, Character *);     // combat special proc
  void *item;                                                                             /* the mobile/object itself                 */

  mob_prog_data *mobprogs;
  mob_prog_data *mobspec;
  int progtypes;
};

struct help_index_element
{
  char *keyword;
  int32_t pos;
};

extern int exp_table[61 + 1];

#define WORLD_FILE_MODIFIED 1
#define WORLD_FILE_IN_PROGRESS 1 << 1
#define WORLD_FILE_READY 1 << 2
#define WORLD_FILE_APPROVED 1 << 3

struct world_file_list_item
{
  QString filename;
  int32_t firstnum;
  int32_t lastnum;
  int32_t flags;
  world_file_list_item *next;
};

// The CWorld class, to control the world a bit better.

class CWorld : public ErrorHandler
{
public:
  room_data &operator[](int rnum);
};

#endif
