
#pragma once
/******************************************************************************
| $Id: character.h,v 1.85 2014/07/26 23:21:23 jhhudso Exp $
| Description: This file contains the header information for the character
|   class implementation.
*/
#include "DC/db.h"

bool on_forbidden_name_list(const QString name);
QString color_to_code(QString color);

constexpr auto MAX_GOLEMS = 2; // amount of golems above +1

constexpr auto START_ROOM = 3001;        // Where you login
constexpr auto CFLAG_HOME = 3014;        // Where the champion flag normally rests
constexpr auto SECOND_START_ROOM = 3059; // Where you go if killed in start room
constexpr auto FARREACH_START_ROOM = 17868;
constexpr auto THALOS_START_ROOM = 5317;

constexpr auto DESC_LENGTH = 80;
constexpr auto CHAR_VERSION = -4;
constexpr auto MAX_NAME_LENGTH = 12;

/************************************************************************
| max stuff - this is needed almost everywhere
*/

constexpr auto CHAMPION_ITEM = 45;

// * ------- Begin MOBProg stuff ----------- *

/* Used in CHAR_FILE_U *DO*NOT*CHANGE* */

class char_file_u
{
public:
  qint8 sex;     /* Sex */
  qint8 c_class; /* Class */
  qint8 race;    /* Race */
  qint8 level;   /* Level */

  qint8 raw_str;
  qint8 raw_intel;
  qint8 raw_wis;
  qint8 raw_dex;
  qint8 raw_con;
  qint8 conditions[3];

  quint8 weight;
  quint8 height;

  qint16 hometown;
  quint32 gold;
  quint32 plat;
  qint64 exp;
  quint32 immune;
  quint32 resist;
  quint32 suscept;

  qint32 mana;     // current
  qint32 raw_mana; // max without eq/stat bonuses
  qint32 hit;
  qint32 raw_hit;
  qint32 move;
  qint32 raw_move;
  qint32 ki;
  qint32 raw_ki;

  qint16 alignment;
  quint32 hpmetas; // Used by familiars too... why not.
  quint32 manametas;
  quint32 movemetas;

  qint16 armor; // have to save these since mobs have different bases
  qint16 hitroll;
  qint16 damroll;
  qint32 afected_by;
  qint32 afected_by2;
  quint32 misc; // channel flags

  qint16 clan;
  qint32 load_room; // Which room to place character in

  quint32 acmetas;
  qint32 agemetas;
  qint32 extra_ints[3]; // available just in case
};

class profession
{
public:
  QString name;
  QString Name;
  quint16 skillno;
  quint8 c_class;
};
