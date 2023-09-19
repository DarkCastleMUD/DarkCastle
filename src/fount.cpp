/***************************************************************************
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: fount.cpp,v 1.6 2014/07/04 22:00:04 jhhudso Exp $ */

extern "C"
{
#include <string.h>
#include <stdio.h>
#include <ctype.h>
}

#include "room.h"
#include "obj.h"
#include "character.h"
#include "utility.h"
#include "act.h"
#include "db.h"
#include "handler.h"
#include "interp.h"

extern World world;

/*************************************************************************
 * Figures out if a fountain is present in the room                       *
 *************************************************************************/

int FOUNTAINisPresent(Character *ch)
{
  class Object *tmp;
  bool found = false;

  for (tmp = world[ch->in_room].contents;
       tmp != nullptr && !found;
       tmp = tmp->next_content)
  {
    if ((tmp->obj_flags.type_flag == ITEM_FOUNTAIN) && CAN_SEE_OBJ(ch, tmp))
    {
      return (true);
    }
  }
  return (false);
}

/************************************************************************
 *  Fill skins and any other drink containers.                           *
 ************************************************************************/

int do_fill(Character *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  class Object *to_obj;
  void name_to_drinkcon(class Object * obj, int type);

  one_argument(argument, buf);

  if (!*buf) /* No arguments */
  {
    act("What do you want to fill?", ch, 0, 0, TO_CHAR, 0);
    return 1;
  }

  if (!(to_obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
  {
    act("You can't find it!", ch, 0, 0, TO_CHAR, 0);
    return 1;
  }

  if (FOUNTAINisPresent(ch))
  {

    if (to_obj->obj_flags.type_flag != ITEM_DRINKCON)
    {
      act("You can't pour anything into that.", ch, 0, 0, TO_CHAR, 0);
      return 1;
    }

    if ((to_obj->obj_flags.value[1] != 0) &&
        (to_obj->obj_flags.value[2] != 0))
    {
      act("There is already another liquid in it!", ch, 0, 0, TO_CHAR, 0);
      return 1;
    }

    if (!(to_obj->obj_flags.value[1] < to_obj->obj_flags.value[0]))
    {
      act("There is no room for more.", ch, 0, 0, TO_CHAR, 0);
      return 1;
    }

    act("You fill $p!", ch, to_obj, 0, TO_CHAR, 0);

    /* First same type liq. */
    to_obj->obj_flags.value[2] = 0;

    /* Then how much to pour */
    to_obj->obj_flags.value[1] = to_obj->obj_flags.value[0];
  }
  else
  {
    act("There is no fountain here!", ch, 0, 0, TO_CHAR, 0);
  }

  return 1;
}
