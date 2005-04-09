#ifndef TERMINAL_H_
#define TERMINAL_H_
/************************************************************************
| $Id: terminal.h,v 1.4 2005/04/09 21:15:35 urizen Exp $
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
#define BLACK   "[30m"
#define RED     "[31m"
#define GREEN   "[32m"
#define YELLOW  "[33m"
#define BLUE    "[34m"
#define PURPLE  "[35m"
#define CYAN    "[36m"
#define GREY    "[37m"

#define EEEE    "#8"           /* Turns screen to EEEEs */
#define CLRSCR  "[2j"          /* Clear screen          */
#define CLREOL   "["          /* Clear to end of line  */

#define UPARR    "[A"
#define DOWNARR  "[B"
#define RIGHTARR "[C"
#define LEFTARR  "[D"
#define HOMEPOS  "[H"

#define FLASH     "[4m"
#define BOLD     "[1m"
#define INVERSE  "[7m"
#define NTEXT    "[0m[37m"      /* Makes it normal */

#endif
