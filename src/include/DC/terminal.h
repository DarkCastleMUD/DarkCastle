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

using namespace Qt::StringLiterals;
const auto BLACK = "[30m"_ba;
const auto RED = "[31m"_ba;
const auto GREEN = "[32m"_ba;
const auto YELLOW = "[33m"_ba;
const auto BLUE = "[34m"_ba;
const auto PURPLE = "[35m"_ba;
const auto CYAN = "[36m"_ba;
const auto GREY = "[37m"_ba;

const auto EEEE = "#8"_ba;    /* Turns screen to EEEEs */
const auto CLRSCR = "[2j"_ba; /* Clear screen          */
const auto CLREOL = "["_ba;   /* Clear to end of line  */

const auto UPARR = "[A"_ba;
const auto DOWNARR = "[B"_ba;
const auto RIGHTARR = "[C"_ba;
const auto LEFTARR = "[D"_ba;
const auto HOMEPOS = "[H"_ba;

const auto FLASH = "[4m"_ba;
const auto BLINK = "[5m"_ba;
const auto BOLD = "[1m"_ba;
const auto INVERSE = "[7m"_ba;
const auto NTEXT = "[0m[37m"_ba; /* Makes it normal */
