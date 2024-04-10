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

#ifndef INTERP_H_
#define INTERP_H_

#include <QString>
#include <QStringList>

#include "common.h"
#include "character.h"
#include "returnvals.h"

class Character;

char *remove_trailing_spaces(char *arg);
int search_block(const char *arg, const char **l, bool exact);

int old_search_block(const char *arg, const QStringList list, bool exact);
int old_search_block(const char *argument, int begin, int length, const QStringList list, int mode);
int old_search_block(const char *argument, int begin, int length, const char **list, int mode);
void argument_interpreter(const char *argument, char *first_arg, char *second_arg);
char *one_argument(char *argument, char *first_arg);
const char *one_argument(const char *argument, char *first_arg);
char *one_argument_long(char *argument, char *first_arg);
char *one_argumentnolow(char *argument, char *first_arg);
int fill_word(char *argument);
void half_chop(const char *str, char *arg1, char *arg2);
std::tuple<std::string, std::string> last_argument(std::string arguments);
std::tuple<std::string, std::string> half_chop(std::string arguments, const char token = ' ');
std::tuple<std::string, std::string> half_chop(const char *c_arg, const char token = ' ');
std::tuple<QString, QString> half_chop(QString arguments, const char token = ' ');
void chop_half(char *str, char *arg1, char *arg2);
void update_max_who(void);
void nanny(class Connection *d, std::string arg = "");
bool is_abbrev(QString abbrev, QString word);
// bool is_abbrev(const std::string &abbrev, const std::string &word);
// bool is_abbrev(const char *arg1, const char *arg2);
void add_command_lag(Character *ch, int cmdnum, int lag);
std::string ltrim(std::string str);
std::string rtrim(std::string str);

// Temp removal to perfect system. 1/25/06 Eas
// WARNING WARNING WARNING WARNING WARNING
// The command list was modified to account for toggle_hide.
// The last integer will affect a char being removed from hide when they perform the command.

/*
 * Command functions.
 */
typedef int DO_FUN(Character *ch, char *argument, int cmd);
typedef int command_return_t;

#endif
