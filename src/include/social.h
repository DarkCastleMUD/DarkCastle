#ifndef SOCIAL_H_
#define SOCIAL_H_
/************************************************************************
| $Id: social.h,v 1.2 2002/06/13 04:41:15 dcastle Exp $
| social.h
| Description:  This file defines the header information for the
|   social functions to work properly.
*/

struct social_messg
{
    char *name;
    int hide;
    int min_victim_position; /* Position of victim */

    /* No argument was supplied */
    char *char_no_arg;
    char *others_no_arg;

    /* An argument was there, and a victim was found */
    char *char_found;       /* if NULL, read no further, ignore args */
    char *others_found;
    char *vict_found;

    /* An argument was there, but no victim was found */
    char *not_found;

    /* The victim turned out to be the character */
    char *char_auto;
    char *others_auto;
};

#endif
