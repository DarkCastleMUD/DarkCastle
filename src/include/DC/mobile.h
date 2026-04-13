#pragma once
/************************************************************************
| $Id: mobile.h,v 1.32 2010/01/01 03:03:14 jhhudso Exp $
| mobile.h
| Description:  This file contains the header information for mobile
|   control.
*/
#include <QString>
#include <QString>
#include <fmt/format.h>
#include <fmt/ostream.h>

void rebuild_rnum_references(qint32 startAt, qint32 type);

QString mprog_next_command(QString clist);

typedef class SelfPurge selfpurge_t;
extern selfpurge_t selfpurge;

class race_data
{
public:
  const QString singular_name; /* Dwarf, Elf, etc.     */
  QString lowercase_name;      /* dwarf, elf, etc.     */
  const QString plural_name;   /* dwarves, elves, etc. */
  bool playable;               /* Can a player play as this race? */
  qint32 body_parts;           /* bitvector for body parts       */
  qint32 immune;               /* bitvector for immunities       */
  qint32 resist;               /* bitvector for resistances      */
  qint32 suscept;              /* bitvector for susceptibilities */
  qint32 hate_fear;            /* bitvector for hate/fear        */
  qint32 friendly;             /* bitvector for friendliness     */
  qint32 min_weight;           /* min weight */
  qint32 max_weight;

  qint32 min_height;
  qint32 max_height;

  quint32 min_str;
  quint32 max_str;
  qint32 mod_str;

  quint32 min_dex;
  quint32 max_dex;
  qint32 mod_dex;

  quint32 min_con;
  quint32 max_con;
  qint32 mod_con;

  quint32 min_int;
  quint32 max_int;
  qint32 mod_int;

  quint32 min_wis;
  quint32 max_wis;
  qint32 mod_wis;

  qint32 affects;        /* automatically added affects   */
  const QString unarmed; // unarmed attack message
};

class mob_matrix_data
{
public:
  qint64 experience;
  qint32 hitpoints;
  qint32 tohit;
  qint32 todam;
  qint32 armor;
  qint32 gold;
};
