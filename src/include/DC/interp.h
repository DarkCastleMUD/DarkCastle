/***************************************************************************
 *  file: Interpreter.h , Command interpreter module.      Part of DIKUMUD *
 *  Usage: Procedures interpreting user command                            *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: interp.h,v 1.115 2011/12/28 01:26:50 jhhudso Exp $ */

#pragma once

#include <QString>
#include <QStringList>
#include <qsize.h>
#include <qtypes.h>

QString remove_trailing_spaces(QString arg);

qsizetype search_list(QString argument, const QStringList list);
qsizetype old_search_block(QString argument, qint32 begin, qint32 length, const QStringList list, qint32 mode);

void argument_interpreter(QString argument, QString first_arg, QString second_arg);
QString one_argumentnolow(QString argument, QString first_arg);
void half_chop(const QString str, QString arg1, QString arg2);
std::tuple<QString, QString> last_argument(QString arguments);
std::tuple<QString, QString> half_chop(QString arguments, const QChar token = ' ');
void chop_half(QString str, QString arg1, QString arg2);
void update_max_who(void);
bool is_abbrev(QString abbrev, QString word);
// bool is_abbrev(const QString &abbrev, const QString &word);
// bool is_abbrev(const QString arg1, const QString arg2);

QString ltrim(QString str);
QString rtrim(QString str);

typedef qint32 command_return_t;
