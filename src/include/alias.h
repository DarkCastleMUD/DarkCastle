#ifndef ALIAS_H_
#define ALIAS_H_
/************************************************************************
| $Id: alias.h,v 1.1 2002/06/13 04:32:22 dcastle Exp $
| alias.h
| Description: Contains header information for the aliasing routine.
*/

struct char_player_alias
{
    char *keyword;                   /* Keyword for aliases   */
    char *command;                   /* Actual command string */
    struct char_player_alias * next; // Next alias
};

#endif /* ALIAS_H_ */
