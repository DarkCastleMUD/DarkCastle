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
#include "DC/utility.h"

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
  bool backupFile(void);
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
  QString filepath_;
  QString error_message_;

private:
};

class LegacyFileWorld : public LegacyFile
{
public:
  LegacyFileWorld(QString filename)
      : LegacyFile("world", filename, "Unable to open world file '%1")
  {
  }
  ~LegacyFileWorld()
  {
    if (file_handle_)
    {
      fprintf(file_handle_, "$~\n");
    }
  }
};

/* public procedures in db.c */
bool can_modify_this_room(Character *ch, int32_t room);
bool can_modify_room(Character *ch, int32_t room);
bool can_modify_mobile(Character *ch, int32_t room);
bool can_modify_object(Character *ch, vnum_t vnum);

void write_one_room(LegacyFile &fl, int nr);
void write_mobile(LegacyFile &lf, Character *mob);
void write_object(LegacyFile &lf, Object *obj);
int load_new_help(FILE *fl, int reload = 0, Character *ch = nullptr);
int count_hash_records(FILE *fl);
void load_hints();
void find_unordered_mobiles(void);
char *mprog_type_to_name(int type);
void write_wizlist(std::stringstream &filename);
void write_wizlist(std::string filename);
void write_wizlist(const char filename[]);
void string_to_file(QTextStream &fl, QString str);
void string_to_file(FILE *fl, QString str);
void string_to_file(auto &fl, QString str)
{
  fl << str.remove('\r').toStdString() << "~\n";
}

auto &operator<<(auto &out, const obj_flag_data &of)
{
  out << of.type_flag << " " << of.extra_flags << " " << of.wear_flags << " " << of.size << "\n";
  out << of.value[0] << " " << of.value[1] << " " << of.value[2] << " " << of.value[3] << " " << of.eq_level << "\n";
  out << of.weight << " " << of.cost << " " << of.more_flags << "\n";
  return out;
}

auto &operator<<(auto &out, extra_descr_data *currdesc)
{
  while (currdesc)
  {
    out << "E\n";
    string_to_file(out, currdesc->keyword);
    string_to_file(out, currdesc->description);
    currdesc = currdesc->next;
  }
  return out;
}

void affects_to_file(auto &out, Object *obj)
{
  for (int i = 0; i < obj->num_affects; i++)
  {
    out << "A\n";
    out << obj->affected[i].location << " " << obj->affected[i].modifier << "\n";
  }
}

auto &operator<<(auto &out, QSharedPointer<class MobProgram> mobprogs)
{
  if (mobprogs)
  {
    write_mprog_recur(out, mobprogs, false);
    out << "|\n";
  }
  return out;
}

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
QString fread_qstring(QTextStream &stream, bool *ok = nullptr);
QString fread_qstring(FILE *stream, bool *ok = nullptr);
char *fread_string(FILE *fl, int hasher);
char *fread_string(std::ifstream &in, int hasher);
char *fread_word(FILE *, int);
QString fread_word(QTextStream &);
auto create_blank_item(vnum_t nr) -> std::expected<vnum_t, create_error>;
int create_blank_mobile(int nr);
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
void write_object_csv(Object *obj, std::ofstream &fout);

extern struct skill_quest *skill_list;
#define REAL 0
#define VIRTUAL 1

class Object *read_object(vnum_t vnum, FILE *fl, bool zz);
class Object *read_object(vnum_t vnum, QTextStream &fl, bool zz);
Character *read_mobile(int nr, FILE *fl);
Character *clone_mobile(int nr);
void randomize_object(Object *obj);
std::ofstream &operator<<(std::ofstream &out, Object *obj);
std::ifstream &operator>>(std::ifstream &in, Object *obj);
std::string lf_to_crlf(std::string &s1);
QString lf_to_crlf(QString s1);
void copySaveData(Object *new_obj, Object *obj);
bool fullItemMatch(Object *obj, Object *obj2);
bool has_random(Object *obj);
FILE *legacyFileOpen(QString directory, QString filename, QString error_message);
void load_messages(char *file, int base = 0);
void boot_social_messages(void);
void boot_clans(void);
void assign_clan_rooms(void);

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

#define WORLD_FILE_MODIFIED 1U
#define WORLD_FILE_IN_PROGRESS 1U << 1U
#define WORLD_FILE_READY 1U << 2U
#define WORLD_FILE_APPROVED 1U << 3U
#define WORLD_FILE_REMOVED 1U << 4U

void delete_item_from_index(auto index, vnum_t vnum)
{
  if (!index.contains(vnum))
  {
    return;
  }

  delete index[vnum].item;
  index.remove(vnum);
}

#endif
