#pragma once
/************************************************************************
| $Id: wizard.h,v 1.8 2012/02/08 22:55:55 jhhudso Exp $
| wizard.h
| Description:  This is NOT a global include file, it's used only
|   for the wiz_1*.C files to consolidate the header files they
|   need.
*/
#include <QtTypes>
#include <cstdio>
QString str_str(QString first, QString second);
void setup_dir(FILE *fl, qint32 room, qint32 dir);
qint32 real_roomb(qint32 virt);
void save_ban_list(void);
void save_nonew_new_list(void);
