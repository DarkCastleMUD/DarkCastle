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

#include "DC/DC.h"

/*************************************************************************
 * Figures out if a fountain is present in the room                       *
 *************************************************************************/

qint32 FOUNTAINisPresent(CharacterPtr ch)
{
  ObjectPtr tmp;
  bool found = false;

  for (tmp = dc_->world[ch->in_room].contents;
       tmp != nullptr && !found;
       tmp = tmp->next_content)
  {
    if ((tmp->flags_.type_flag == ITEM_FOUNTAIN) && CAN_SEE_OBJ(ch, tmp))
    {
      return (true);
    }
  }
  return (false);
}

/************************************************************************
 *  Fill skins and any other drink containers.                           *
 ************************************************************************/

command_return_t do_fill(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  ObjectPtr to_obj;
  one_argument(argument, buf);

  if (buf.isEmpty()) /* No arguments */
  {
    act_to_character("What do you want to fill?", ch, 0, 0, 0);
    return 1;
  }

  if (!(to_obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
  {
    act_to_character("You can't find it!", ch, 0, 0, 0);
    return 1;
  }

  if (FOUNTAINisPresent(ch))
  {

    if (to_obj->flags_.type_flag != ITEM_DRINKCON)
    {
      act_to_character("You can't pour anything into that.", ch, 0, 0, 0);
      return 1;
    }

    if ((to_obj->flags_.value[1] != 0) &&
        (to_obj->flags_.value[2] != 0))
    {
      act_to_character("There is already another liquid in it!", ch, 0, 0, 0);
      return 1;
    }

    if (!(to_obj->flags_.value[1] < to_obj->flags_.value[0]))
    {
      act_to_character("There is no room for more.", ch, 0, 0, 0);
      return 1;
    }

    act_to_character("You fill $p!", ch, to_obj, 0, 0);

    /* First same type liq. */
    to_obj->flags_.value[2] = {};

    /* Then how much to pour */
    to_obj->flags_.value[1] = to_obj->flags_.value[0];
  }
  else
  {
    act_to_character("There is no fountain here!", ch, 0, 0, 0);
  }

  return 1;
}
