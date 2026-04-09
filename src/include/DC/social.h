#pragma once
/************************************************************************
| $Id: social.h,v 1.2 2002/06/13 04:41:15 dcastle Exp $
| social.h
| Description:  This file defines the header information for the
|   social functions to work properly.
*/

#include "DC/common.h"
class social_messg
{
public:
  QString name;
  qint32 hide = {};
  position_t min_victim_position = {}; /* Position of victim */

  /* No argument was supplied */
  QString char_no_arg;
  QString others_no_arg;

  /* An argument was there, and a victim was found */
  QString char_found; /* if nullptr, read no further, ignore args */
  QString others_found;
  QString vict_found;

  /* An argument was there, but no victim was found */
  QString not_found;

  /* The victim turned out to be the character */
  QString char_auto;
  QString others_auto;
};
