/***************************************************************************
| comm.h: New comm stuff to make the rewrite of comm.C work properly
*/
#pragma once
#include <QString>
#include <QStringList>
#include "DC/terminal.h"

constexpr auto SMALL_BUFSIZE = 1024;
#define LARGE_BUFSIZE (24 * 2048)
constexpr auto GARBAGE_SPACE = 32;
constexpr auto NUM_RESERVED_DESCS = 8;
constexpr auto HOST_LENGTH = 30;

enum pulse_type
{
  TIMER,
  MOBILE,
  OBJECT,
  VIOLENCE,
  BARD,
  TENSEC,
  WEATHER,
  TIME,
  REGEN,
  SHORT
};

class pulse_info
{
public:
  pulse_type pulse;
  quint64 duration;
  QString name;
};

const QStringList cond_colorcodes = {
    BOLD + GREEN,
    GREEN,
    BOLD + YELLOW,
    YELLOW,
    RED,
    BOLD + RED,
    BOLD + GREY};

qint32 write_to_descriptor(qint32 desc, QByteArray txt);
QString scramble_text(QString txt);
void send_info(QString messg);
void send_info(QString messg);
void send_info(const QString messg);
void update_bard_singing(void);
void affect_update(qint32 duration_type); /* In spells.c */
const QString calc_color(qint32 hit, qint32 max_hit);
const QString calc_color_align(qint32 align);
