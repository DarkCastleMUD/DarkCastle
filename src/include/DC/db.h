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
#ifndef DC_DB_H_
#define DC_DB_H_

#include <cstdio>
#include <ctime>

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

#include <QDateTime>

#include "DC/character.h"
#include "DC/weather.h"
#include "DC/handler.h"
#include "DC/DC.h"

extern class Object *object_list;
extern room_t top_of_world;

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

class LegacyFile : public QObject
{
  Q_OBJECT
public:
  LegacyFile(QString directory, QString filename, QString error_message);
  ~LegacyFile();
  FILE *openFile(void);
  bool isOpen(void)
  {
    if (!file_handle_ || feof(file_handle_) || ferror(file_handle_))
    {
      return false;
    }
    return true;
  }
  FILE *file_handle_;
  QString directory_;
  QString filename_;
  QString error_message_;

private:
};

class LegacyFileWorld : public LegacyFile
{
public:
  LegacyFileWorld(QString filename)
      : LegacyFile("world/%1", filename, "Unable to open world file '%1")
  {
  }
  ~LegacyFileWorld()
  {
    fprintf(file_handle_, "$~\n");
  }
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

void write_one_room(LegacyFile &fl, int nr);
void write_mobile(LegacyFile &lf, Character *mob);
void write_object(LegacyFile &lf, Object *obj);

void write_object(Object *obj, QTextStream &fl);
void load_emoting_objects(void);
int create_entry(char *name);
void zone_update(void);
void init_char(Character *ch);
void clear_char(Character *ch);
void clear_object(class Object *obj);
void reset_char(Character *ch);
void free_char(Character *ch, Trace trace = Trace("Unknown"));
room_t real_room(room_t virt);
char *fread_string(QTextStream &stream, bool hasher, bool *ok = nullptr);
QString fread_string(QTextStream &stream, bool *ok = nullptr);
char *fread_string(FILE *fl, int hasher);
char *fread_string(std::ifstream &in, int hasher);
char *fread_word(FILE *, int);
QString fread_word(QTextStream &);
enum class create_error
{
  index_full,
  entry_exists
};
auto create_blank_item(int nr) -> std::expected<int, create_error>;
int create_blank_mobile(int nr);
void delete_item_from_index(int nr);
void delete_mob_from_index(int nr);
int real_object(int virt);
int real_mobile(int virt);
QString qDebugQTextStreamLine(QTextStream &stream, QString message = "Current line");

int64_t fread_int(FILE *fl, int64_t minval, int64_t maxval);
int64_t fread_int(std::ifstream &in, int64_t beg_range, int64_t end_range);
template <class T>
T fread_int(QTextStream &in, T minval = std::numeric_limits<T>::min(), T maxval = std::numeric_limits<T>::max());
uint64_t fread_uint(FILE *fl, uint64_t minval, uint64_t maxval);
char fread_char(FILE *fl);
char fread_char(QTextStream &fl);
int fread_bitvector(FILE *fl, int32_t minval, int32_t maxval);
int fread_bitvector(std::ifstream &fl, int32_t minval, int32_t maxval);

void add_mobspec(int i);
void write_object_csv(Object *obj, std::ofstream &fout);
index_data *generate_obj_indices(int *top, index_data *index);
index_data *generate_mob_indices(int *top, index_data *index);

extern struct skill_quest *skill_list;
extern index_data mob_index_array[MAX_INDEX];
#define REAL 0
#define VIRTUAL 1

class Object *read_object(int nr, FILE *fl, bool zz);
class Object *read_object(int nr, QTextStream &fl, bool zz);
Character *read_mobile(int nr, FILE *fl);
class Object *clone_object(int nr);
Character *clone_mobile(int nr);
void randomize_object(Object *obj);
void string_to_file(FILE *fl, QString str);
void string_to_file(std::ofstream &fl, QString str);
void string_to_file(QTextStream &fl, QString str);
std::ofstream &operator<<(std::ofstream &out, Object *obj);
std::ifstream &operator>>(std::ifstream &in, Object *obj);
std::string lf_to_crlf(std::string &s1);
void copySaveData(Object *new_obj, Object *obj);
bool verify_item(class Object **obj);
bool fullItemMatch(Object *obj, Object *obj2);
bool has_random(Object *obj);
FILE *legacyFileOpen(QString directory, QString filename, QString error_message);
void load_messages(char *file, int base = 0);
void boot_social_messages(void);
void boot_clans(void);
void assign_clan_rooms(void);
void find_unordered_objects(void);

extern int top_of_objt;
extern time_t start_time; /* mud start time */

struct pulse_data
{ /* list for keeping tract of 'pulsing' chars */
  Character *thechar;
  pulse_data *next;
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

#endif
