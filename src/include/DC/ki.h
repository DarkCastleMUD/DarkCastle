/*
 * ki.h contains the header files for the
 * ki powers, although the ki structure is in
 * spells.h, so that must be included
 */
/* $Id: ki.h,v 1.9 2009/05/02 22:41:34 shane Exp $ */

#pragma once
#include <QtTypes>

/************************************************************************
| These are pretty worthless, since I never did anything with them
|   and it was a pretty stupid idea anyway ;P We can take them out
|   at some point.  Morc XXX
*/
/* The following defs refer to player ki and its effects */
constexpr auto MAXIMUM_KI = 100;
constexpr auto MINIMUM_KI = 0;
constexpr auto MIN_REACT_KI = 20;
constexpr auto NO_EFFECT = 0;
constexpr auto DIVINE = 1;
constexpr auto MIRACLE = 2;
constexpr auto MAJOR_EFFECT = 3;
constexpr auto MINOR_EFFECT = 4;
const char *skip_spaces(const char *s);
qint32 find_skill_num(char *name);