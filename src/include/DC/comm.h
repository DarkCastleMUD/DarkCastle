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

void write_to_output(const QString txt, class Connection *t);
void write_to_output(QByteArray txt, class Connection *d);
void write_to_output(QString txt, class Connection *d);
void write_to_output(QString txt, class Connection *t);
qint32 write_to_descriptor(qint32 desc, QByteArray txt);
QString scramble_text(QString txt);
void send_info(QString messg);
void send_info(QString messg);
void send_info(const QString messg);
void new_string_add(class Connection *d, QString str);
void telnet_ga(Connection *d);
void telnet_sga(Connection *d);
void telnet_echo_off(class Connection *d);
void telnet_echo_on(class Connection *d);
void update_bard_singing(void);
void affect_update(qint32 duration_type); /* In spells.c */
const QString calc_color(qint32 hit, qint32 max_hit);
const QString calc_color_align(qint32 align);
