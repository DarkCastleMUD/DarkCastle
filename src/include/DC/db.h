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
#pragma once
#include <QString>
#include <QObject>
class error_eof
{
};
class error_negative_int
{
};
class error_range_int
{
};
class error_range_under
{
};
class error_range_over
{
};

const quint64 WORLD_MAX_ROOM = 50000; // should never get this high...
                                      // it's just to keep builders/imps from
                                      // doing a 'goto 1831919131928' and
                                      // creating it

const qint32 VERSION_NUMBER = 2; /* used for changing pfile format */

extern QList<QString> continent_names;

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
qint32 count_hash_records(FILE *fl);
void load_hints();
void find_unordered_mobiles(void);
void write_wizlist(QString filename);
void string_to_file(QTextStream &fl, QString str);

void string_to_file(auto &fl, QString str)
{
  fl << str.remove('\r').toStdString() << "~\n";
}

void load_emoting_objects(void);
qint32 create_entry(QString name);
QString fread_string(QTextStream &stream, bool *ok = {});
QString fread_word(QTextStream &);
void delete_item_from_index(qint32 nr);
void delete_mob_from_index(qint32 nr);
qint32 real_object(qint32 virt);
qint32 real_mobile(qint32 virt);

quint64 fread_uint(auto &in, quint64 minval = std::numeric_limits<quint64>::min(), quint64 maxval = std::numeric_limits<quint64>::max())
{
  quint64 val;
  in >> val;
  return val;
}

QChar fread_char(QTextStream &fl);

template <class T>
T fread_bitvector(auto &in)
{
  auto value = fread_uint(in);
  T flags = T::fromInt(value);

  return flags;
}

void add_mobspec(qint32 i);

constexpr auto REAL = 0;
constexpr auto VIRTUAL = 1;

void string_to_file(FILE *fl, QString str);
void string_to_file(QTextStream &fl, QString str);
QString lf_to_crlf(QString &s1);
QString lf_to_crlf(QString s1);

FILE *legacyFileOpen(QString directory, QString filename, QString error_message);

extern qint32 top_of_objt;
extern time_t start_time; /* mud start time */

extern qint32 exp_table[61 + 1];

constexpr auto WORLD_FILE_MODIFIED = 1;
constexpr auto WORLD_FILE_IN_PROGRESS = 1 << 1;
constexpr auto WORLD_FILE_READY = 1 << 2;
constexpr auto WORLD_FILE_APPROVED = 1 << 3;
