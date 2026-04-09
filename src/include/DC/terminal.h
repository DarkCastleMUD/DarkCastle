#pragma once
/************************************************************************
| $Id: terminal.h,v 1.5 2006/04/25 10:36:13 dcastle Exp $
| Terminal.h
| Description: This file defines all of the terminal constants and
|   header information.
*/

/* Heres the codes.. with comments!  enjoy: */
/*
 * Ansi colors and VT100 codes
 * Used in #PLAYER
 */

/************************************************************************
| We're not #define'ing these because of the way they're used in
|   strings.  Maybe we'll fix it some day.
*/
#include <QString>
const auto BLACK = QStringLiteral("[30m");
const auto RED = QStringLiteral("[31m");
const auto GREEN = QStringLiteral("[32m");
const auto YELLOW = QStringLiteral("[33m");
const auto BLUE = QStringLiteral("[34m");
const auto PURPLE = QStringLiteral("[35m");
const auto CYAN = QStringLiteral("[36m");
const auto GREY = QStringLiteral("[37m");

const auto EEEE = QStringLiteral("#8");    /* Turns screen to EEEEs */
const auto CLRSCR = QStringLiteral("[2j"); /* Clear screen          */
const auto CLREOL = QStringLiteral("[");   /* Clear to end of line  */

const auto UPARR = QStringLiteral("[A");
const auto DOWNARR = QStringLiteral("[B");
const auto RIGHTARR = QStringLiteral("[C");
const auto LEFTARR = QStringLiteral("[D");
const auto HOMEPOS = QStringLiteral("[H");

const auto FLASH = QStringLiteral("[4m");
const auto BLINK = QStringLiteral("[5m");
const auto BOLD = QStringLiteral("[1m");
const auto INVERSE = QStringLiteral("[7m");
const auto NTEXT = QStringLiteral("[0m[37m"); /* Makes it normal */
