/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include <cctype>
#include <cstring>

#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <QStringLiteral>

#include "DC/DC.h"
#include "DC/structs.h"
#include "DC/spells.h"

#include "DC/interp.h"
#include "DC/db.h"
#include "DC/innate.h"
#include "DC/const.h"

// external vars

extern bool MOBtrigger;
extern mprog_throw_type *g_mprog_throw_list;

extern CharacterPtr activeActor;
extern CharacterPtr activeRndm;
extern CharacterPtr activeTarget;
extern ObjectPtr activeObj;
extern void *activeVo;
extern qint32 mprog_line_num;    // From mob_prog.cpp
extern qint32 mprog_command_num; // From mob_prog.cpp

/* A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MOBprograms which are set.
 */

void mpstat(CharacterPtr ch, CharacterPtr victim)
{
  QString buf;
  QString buf2;
  mob_prog_data *mprg = {};
  qint32 i;

  buf = QString::asprintf("$3Name$R: %s  $3Vnum$R: %llu.\r\n", qPrintable(victim->name()), DC::getInstance()->mob_index[victim->mobdata->nr].vnum());
  ch->send(buf);

  buf = QString::asprintf("$3Short description$R: %s\r\n$3Long  description$R: %s\r\n", victim->short_description(), victim->long_description());
  ch->send(buf);

  if (!(DC::getInstance()->mob_index[victim->mobdata->nr].progtypes))
  {
    ch->sendln("That mob has no programs set.");
    return;
  }

  for (mprg = DC::getInstance()->mob_index[victim->mobdata->nr].mobprogs, i = 1; mprg != nullptr;
       i++, mprg = mprg->next)
  {
    buf = QString::asprintf("$3%d$R>$3$B", i);
    ch->send(buf);
    send_to_char(Program::mprog_type_to_name(mprg->type), ch);
    ch->send("$R ");
    dc_sprintf(buf, "$B$5%s$R\r\n", mprg->arglist);
    ch->send(buf);
    ch->sendRaw(QString(mprg->comlist) + "\r\n");
  }
}

/* prints the argument to all the rooms aroud the mobile */

command_return_t do_mpasound(CharacterPtr ch, QString argument, cmd_t cmd)
{

  qint32 was_in_room;
  qint32 door;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  if (argument[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpasound - No argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  was_in_room = ch->in_room;
  for (door = {}; door <= 5; door++)
  {
    if (CAN_GO(ch, door))
    {
      ch->in_room = DC::getInstance()->world[was_in_room].dir_option[door]->to_room;
      if (ch->in_room == was_in_room)
        continue;
      MOBtrigger = false;
      // argument +1 so we skip the leading ' '
      act(argument + 1, ch, nullptr, nullptr, TO_ROOM, 0);
      ch->in_room = was_in_room;
    }
  }

  ch->in_room = was_in_room;
  return ReturnValue::eSUCCESS;
}

/* lets the mobile kill any player or mobile without murder*/

command_return_t do_mpkill(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    ch->prog_error(QStringLiteral("MpKill - no argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if ((victim = ch->get_char_room_vis(arg)) == nullptr)
  {
    ch->prog_error(QStringLiteral("MpKill - Victim not in room."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (victim == ch)
  {
    ch->prog_error(QStringLiteral("MpKill - Bad victim to attack."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
  {
    ch->prog_error(QStringLiteral("MpKill - Charmed mob attacking master."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (ch->isFighting())
  {
    ch->prog_error(QStringLiteral("MpKill - Already fighting"));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  return attack(ch, victim, TYPE_UNDEFINED);
}

command_return_t do_mphit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    ch->prog_error(QStringLiteral("MpHit - no argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if ((victim = ch->get_char_room_vis(arg)) == nullptr)
  {
    ch->prog_error(QStringLiteral("MpHit - Victim not in room."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (GET_POS(victim) == position_t::DEAD)
  {
    ch->prog_error(QStringLiteral("MpHit - Victim already dead."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (victim == ch)
  {
    ch->prog_error(QStringLiteral("MpHit - Bad victim to attack."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
  {
    ch->prog_error(QStringLiteral("MpHit - Charmed mob attacking master."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  return one_hit(ch, victim, TYPE_UNDEFINED, WEAR_WIELD);
}

command_return_t do_mpaddlag(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  QString arg1;
  CharacterPtr victim;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);

  if (arg[0] == '\0')
  {
    ch->prog_error(QStringLiteral("MpAddlag - no argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if ((victim = ch->get_char_room_vis(arg)) == nullptr)
  {
    ch->prog_error(QStringLiteral("MpAddlag - Victim not in room."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  if (!arg1[0] || !is_number(arg1))
  {
    ch->prog_error(QStringLiteral("MpAddlag - Invalid duration."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  WAIT_STATE(victim, atoi(arg1));
  return ReturnValue::eSUCCESS;
}

/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy
   just plain everything  */

command_return_t do_mpjunk(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  ObjectPtr obj;
  qint32 location;
  bool dot = false;
  QString dotbuf;
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }
  dotbuf[0] = '\0';
  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpjunk - No argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (str_cmp(arg, "all") && !sscanf(arg, "all.%s", dotbuf))
  {
    if ((obj = ch->get_object_in_equip_vis(arg, ch->equipment, &location, false)))
    {
      extract_obj(ch->unequip_char(location));
      return ReturnValue::eSUCCESS;
    }
    if ((obj = get_obj_in_list(arg, ch->carrying)))
    {
      extract_obj(obj);
      return ReturnValue::eSUCCESS;
    }
  }
  else
  {
    if (dotbuf[0] != '\0')
      dot = true;

    for (qint32 l = {}; l < MAX_WEAR; l++)
      if (ch->equipment[l])
        if (!dot || isexact(dotbuf, ch->equipment[l]->name()))
          extract_obj(ch->unequip_char(l));

    ObjectPtr x, v;
    for (x = ch->carrying; x; x = v)
    {
      v = x->next_content;
      if (!dot || isexact(dotbuf, x->name()))
        extract_obj(x);
    }
    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eFAILURE;
}

/* prints the message to everyone in the room other than the mob and victim */

command_return_t do_mpechoaround(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpechoaround - No argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (!(victim = get_char_room(arg, ch->in_room, true)))
  {
    ch->prog_error(QStringLiteral("Mpechoaround - victim does not exist."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  if (CAN_SEE(ch, victim))
    act(argument + 1, ch, nullptr, victim, TO_ROOM, NOTVICT);
  return ReturnValue::eSUCCESS;
}

command_return_t do_mpechoaroundnotbad(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg, arg1[MAX_INPUT_LENGTH];
  CharacterPtr victim, victim2;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);
  if (arg[0] == '\0' || arg1[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpechoaroundnotbad - No argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (!(victim = get_char_room(arg, ch->in_room)))
  {
    ch->prog_error(QStringLiteral("Mpechoaroundnotbad - victim does not exist."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  if (!(victim2 = get_char_room(arg1, ch->in_room)))
  {
    ch->prog_error(QStringLiteral("Mpechoaroundnotbad - victim does not exist."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  //     if (CAN_SEE(ch,victim))
  act(argument + 1, victim, nullptr, victim2, TO_ROOM, NOTVICT);
  return ReturnValue::eSUCCESS;
}

/* prints the message to only the victim */

command_return_t do_mpechoat(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpechoat - No argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (!(victim = get_char_room(arg, ch->in_room, true)))
  {
    ch->prog_error(QStringLiteral("Mpechoat - victim does not exist."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  act(argument + 1, ch, nullptr, victim, TO_VICT, 0);
  return ReturnValue::eSUCCESS;
}

/* prints the message to the room at large */

command_return_t do_mpecho(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  if (argument[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpecho - called w/o argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  act(argument + 1, ch, nullptr, nullptr, TO_ROOM, 0);
  return ReturnValue::eSUCCESS;
}

/* lets the mobile load an item or mobile.  All items
are loaded into inventory.  you can specify a level with
the load object portion as well. */

command_return_t do_mpmload(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  qint32 realnum;
  CharacterPtr victim;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0' || !is_number(arg))
  {
    ch->prog_error(QStringLiteral("Mpmload - Bad vnum as arg."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if ((realnum = real_mobile(atoi(arg))) < 0)
  {
    ch->prog_error(QStringLiteral("Mpmload - Bad mob vnum."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  victim = ch->getDC()->clone_mobile(realnum);
  victim->hometown = ch->getDC()->world[ch->in_room].number;
  char_to_room(victim, ch->in_room);
  mprog_load_trigger(victim); // victim not used after, no selfpurge checks, leave the selfpurge of the mobprog that is causing this load intact as whatever it is

  return ReturnValue::eSUCCESS;
}

command_return_t do_mpoload(CharacterPtr ch, QString argument, cmd_t cmd)
{
  auto &arena = DC::getInstance()->arena_;
  QString arg1 = {0};
  QString arg2 = {0};
  ObjectPtr obj;
  qint32 realnum;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg1);
  one_argument(argument, arg2);

  if (arg1[0] == '\0' || !is_number(arg1))
  {
    ch->prog_error(QStringLiteral("Mpoload - Bad syntax."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if ((realnum = real_object(atoi(arg1))) < 0)
  {
    ch->prog_error(QStringLiteral("Mpoload - Bad vnum arg."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  obj = clone_object(realnum);

  if (DC::getInstance()->obj_index[obj->item_number].vnum() == 393 && ch->room().isArena() && arena.isPotato() && arena.isOpened())
  {
    return ReturnValue::eFAILURE;
  }

  if (!strcasecmp(arg2, "random"))
  {
    randomize_object(obj);
  }

  if (CAN_WEAR(obj, TAKE))
  {
    obj_to_char(obj, ch);
  }
  else
  {
    obj_to_room(obj, ch->in_room);
  }

  return ReturnValue::eSUCCESS;
}

/* lets the mobile purge all objects and other npcs in the room,
   or purge a specified object or mob in the room.  It can purge
   itself, but this had best be the last command in the MOBprogram
   otherwise ugly stuff will happen */

command_return_t do_mppurge(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;
  ObjectPtr obj;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    /* 'purge' */
    CharacterPtr vnext;
    ObjectPtr obj_next;

    for (victim = DC::getInstance()->world[ch->in_room].people; victim != nullptr; victim = vnext)
    {
      vnext = victim->next_in_room;
      if (victim->isNonPlayer() && victim != ch)
      {
        extract_char(victim, true);
      }
    }

    for (obj = DC::getInstance()->world[ch->in_room].contents; obj != nullptr; obj = obj_next)
    {
      obj_next = obj->next_content;
      extract_obj(obj);
    }

    return ReturnValue::eSUCCESS;
  }

  if (!(victim = get_char_room(arg, ch->in_room)))
  {
    if ((obj = get_obj_in_list(arg, DC::getInstance()->world[ch->in_room].contents)) ||
        (obj = get_obj_in_list(arg, ch->carrying)))
    {
      extract_obj(obj);
    }

    //      - We don't want to log this since it is quite possible that the item we want to purge
    //      isn't there.  For example, if we tried to give a unique quest item and want to purge it
    //      from our inventory if the player already had one.  It may or may not be there.
    //	else
    //	{
    //	    ch->prog_error( QStringLiteral("Mppurge - Bad argument."));
    //            return ReturnValue::eFAILURE|ReturnValue::eINTERNAL_ERROR;
    //	}
    return ReturnValue::eSUCCESS;
  }

  if (victim->isPlayer())
  {
    ch->prog_error(QStringLiteral("Mppurge - Purging a PC."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  //    issame = (ch == victim);
  if (ch == victim)
  {
    // DC::getInstance()->logf(0, DC::LogChannel::LOG_BUG, "selfpurge on %s to %s", qPrintable(ch->name()), qPrintable(victim->name()));
    selfpurge = true;
    selfpurge.setOwner(ch, "do_mppurge");
  }
  extract_char(victim, true);

  //    if(issame)

  // we have to return both ch, and vict, since this could be in a greet or entry prog
  // or a number of other possibilities and we dunno who died.  Otherwise we continue
  // trying to move someone that is dead.

  return ReturnValue::eSUCCESS | ReturnValue::eCH_DIED | ReturnValue::eVICT_DIED;

  //    return ReturnValue::eSUCCESS;
}

/* lets the mobile goto any location it wishes that is not private */

command_return_t do_mpgoto(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  qint32 location = -1;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg);
  if (arg[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpgoto - No argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  // TODO - make this work with strings (goto chode) too

  CharacterPtr vict;
  if (!str_cmp(arg, "mob"))
  {
    one_argument(argument, arg);
    if (arg[0] == '\0' || !is_number(arg))
    {
      ch->prog_error(QStringLiteral("Mpgoto - Missing vnum after 'mob' argument."));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }
    vict = get_mob_vnum(atoi(arg));
    if (!vict)
      location = -1;
    else
      location = vict->in_room;
  }
  else if (!str_cmp(arg, "pc"))
  {
    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
      ch->prog_error(QStringLiteral("Mpgoto - Missing arg after 'pc' argument."));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }
    if (!(vict = get_pc(arg)))
      location = -1;
    else
      location = vict->in_room;
  }
  else
  {
    ObjectPtr obj;
    if ((vict = get_char_vis(ch, arg)))
    {
      location = vict->in_room;
    }
    else if ((obj = get_obj_vis(ch, arg)) && obj->in_room >= 0)
    {
      location = obj->in_room;
    }
    else
    {
      location = atoi(arg);
    }
  }
  if (location < 0)
  {
    ch->prog_error(QStringLiteral("Mpgoto - No such location."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  if (location == ch->in_room)
    return ReturnValue::eFAILURE; // zz
  if (location > DC::getInstance()->top_of_world || !DC::getInstance()->rooms.contains(location))
    location = {};

  if (ch->fighting != nullptr)
    stop_fighting(ch);

  char_from_room(ch);
  char_to_room(ch, location);

  return ReturnValue::eSUCCESS;
}

/* lets the mobile do a command at another location. Very useful */

command_return_t do_mpat(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  qint32 location;
  qint32 original;
  qint32 result;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpat - Bad argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  // TODO - make location take args
  CharacterPtr vict;
  if (!(vict = get_char_vis(ch, arg)))
  {
    location = atoi(arg);
  }
  else
  {
    location = vict->in_room;
  }

  if (location < 0)
  {
    ch->prog_error(QStringLiteral("do_mpat - No such location."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  if (location > DC::getInstance()->top_of_world || !DC::getInstance()->rooms.contains(location))
  {
    if (!DC::getInstance()->rooms.contains(1))
    {
      ch->send(QStringLiteral("mpat - Room %1 invalid. Tried room 1 but it's invalid too.\r\n").arg(location));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }
    else
    {
      location = 1;
    }
  }

  original = ch->in_room;
  char_from_room(ch, false); // Don't stop all fighting
  char_to_room(ch, location);
  result = ch->command_interpreter(QString(argument));

  if (isSet(result, ReturnValue::eCH_DIED))
    return result;

  char_from_room(ch);
  char_to_room(ch, original);

  return result;
}

// Reward the player with some XP
// Also works with -xp to penalize
command_return_t do_mpxpreward(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  QString buf;
  qint32 reward;
  CharacterPtr vict;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  half_chop(argument, arg, buf);

  if (arg[0] == '\0' || !(vict = get_pc_room_vis_exact(ch, QString(arg))))
  {
    ch->prog_error(QStringLiteral("Mpxpreward - Bad argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (!check_valid_and_convert(reward, buf))
  {
    ch->prog_error(QStringLiteral("Mpxpreward - Bad argument."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (reward < 0)
    if ((vict->exp + reward) < 0)
    {
      vict->send(QStringLiteral("You lose %d exps.\r\n").arg((-1 * reward)));
      vict->exp = {};
      return ReturnValue::eSUCCESS;
    }

  vict->send(QStringLiteral("You receive %1 exps.\r\n").arg(reward));
  gain_exp(vict, reward);
  return ReturnValue::eSUCCESS;
}

/* lets the mobile transfer people.  the all argument transfers
   everyone in the current room to the specified location */

command_return_t do_mptransfer(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  QString arg2;
  qint32 location;
  Connection *d;
  CharacterPtr victim;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if (arg1[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mptransfer - Bad syntax"));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (!str_cmp(arg1, "all"))
  {
    for (d = DC::getInstance()->connections_; d != nullptr; d = conn->next)
    {
      if (conn->connected == Connection::states::PLAYING && conn->character != ch && conn->character->in_room == ch->in_room && CAN_SEE(ch, conn->character))
      {
        QString buf;
        dc_sprintf(buf, "%s %s", qPrintable(conn->character->name()), arg2);
        do_mptransfer(ch, buf, cmd);
      }
    }
    return ReturnValue::eSUCCESS;
  }

  /*
   * Thanks to Grodyn for the optional location parameter.
   */
  if (arg2[0] == '\0')
  {
    location = ch->in_room;
  }
  else
  {
    // TODO - make location take args
    location = atoi(arg2);

    if (location < 0)
    {
      ch->prog_error(QStringLiteral("Mptransfer - No such location."));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }

    if (isSet(DC::getInstance()->world[location].room_flags, PRIVATE))
    {
      ch->prog_error(QStringLiteral("Mptransfer - Private room."));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }
  }

  if ((victim = get_char_vis(ch, arg1)) == nullptr)
  {
    ch->prog_error(QStringLiteral("Mptransfer - No such person."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (victim->in_room == DC::NOWHERE)
  {
    ch->prog_error(QStringLiteral("Mptransfer - Victim in Limbo."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  if (victim->fighting != nullptr)
    stop_fighting(victim);

  char_from_room(victim);
  char_to_room(victim, location);

  return ReturnValue::eSUCCESS;
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */

command_return_t do_mpforce(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpforce - Bad syntax"));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  // TODO - this is dangerous...need to rework it.  If the person we force kills the
  // next person we force, who was a mob, we can crash cause vch_next is no longer valid

  if (!str_cmp(arg, "all"))
  {
    for (auto tch : DC::getInstance()->world[ch->in_room].people_)
    {
      if (!tch->isImmortalPlayer() && CAN_SEE(ch, tch))
      {
        tch->command_interpreter(QString(argument));
      }
    }
  }
  else
  {
    auto victim = ch->get_char_room_vis(arg);
    if (victim)
    {
      ch->prog_error(QStringLiteral("Mpforce - No such victim."));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }

    if (victim == ch)
    {
      ch->prog_error(QStringLiteral("Mpforce - Forcing oneself"));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }

    if (CAN_SEE(ch, victim))
    {
      if (!victim->isImmortalPlayer())
        victim->command_interpreter(QString(argument));
      else
      {
        ch->prog_error(QStringLiteral("Tried to MOBProg force %1 to '%2'.").arg(victim->name()).arg(argument));
      }
    }
  }

  return ReturnValue::eSUCCESS;
}

// "Throw" a message to another mob.  Right now, it's only an qint32 specifying which
// 'catch' should handle it
// argument should be <mob> <catchnum> <delay>
command_return_t do_mpthrow(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 mob_num;
  qint32 catch_num;
  qint32 delay;

  QString first;
  QString second;
  QString third;
  QString fourth;
  QString fifth;
  // locate and validate argument to find target
  argument = one_argument(argument, first);
  argument = one_argument(argument, second);
  argument = one_argument(argument, third);
  argument = one_argument(argument, fourth);
  argument = one_argument(argument, fifth);

  if (isdigit(*first))
  {
    if (!check_valid_and_convert(mob_num, first) || (real_mobile(mob_num) < 0))
    {
      ch->prog_error(QStringLiteral("Mpthrow - Invalid mobnum."));
      return ReturnValue::eFAILURE;
    }
    *first = '\0';
  }
  else
  {
    if (strlen(first) >= MAX_THROW_NAME)
    {
      ch->prog_error(QStringLiteral("Mpthrow - Name too long."));
      return ReturnValue::eFAILURE;
    }
  }

  if (!check_range_valid_and_convert(catch_num, second, MPROG_CATCH_MIN, MPROG_CATCH_MAX))
  {
    ch->prog_error(QStringLiteral("Mpthrow - Invalid catch_num."));
    return ReturnValue::eFAILURE;
  }

  if (!check_range_valid_and_convert(delay, third, 0, 500000))
  {
    ch->prog_error(QStringLiteral("Mpthrow - Invalid delay."));
    return ReturnValue::eFAILURE;
  }

  qint32 opt = {};
  if (!check_range_valid_and_convert(opt, fifth, 0, 50000))
    opt = {};

  // create
  auto throwitem = new mprog_throw_type;
  throwitem->target_mob_num = mob_num;
  strcpy(throwitem->target_mob_name, first);
  throwitem->data_num = catch_num;
  throwitem->delay = delay;
  throwitem->mob = true; // This is, suprisingly, a mob
  throwitem->actor = activeActor;
  throwitem->obj = activeObj;
  throwitem->vo = activeVo;
  throwitem->rndm = {};

  if (fourth[0] != '\0')
    throwitem->var = (fourth);
  else
    throwitem->var = {};
  throwitem->opt = opt;
  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;

  return ReturnValue::eSUCCESS;
}

command_return_t do_mpteachskill(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  QString skill;

  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  half_chop(argument, arg, skill);

  if (arg[0] == '\0' || skill[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpteachskill - Bad syntax."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  CharacterPtr victim;

  if ((victim = get_char_room(arg, ch->in_room)) == nullptr)
  {
    ch->prog_error(QStringLiteral("Mpteachskill - No such victim."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  qint32 skillnum;
  if (!(check_range_valid_and_convert(skillnum, skill, 0, SKILL_SONG_MAX)))
  {
    ch->prog_error(QStringLiteral("Mpteachskill - No such skill"));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  QString skillname = get_skill_name(skillnum);

  if (victim->has_skill(skillnum))
  {
    victim->send(QStringLiteral("You already know the basics of %s!\r\n").arg(skillname.isEmpty() ? "Unknown" : qPrintable(skillname)));
    return ReturnValue::eFAILURE;
  }

  CharacterClassSkill *skilllist = victim->get_skill_list();
  if (!skilllist)
  {
    ch->prog_error(QStringLiteral("Mpteachskill - %1 had no skill list?").arg(victim->name()));
    return ReturnValue::eFAILURE; // no skills to train
  }

  qint32 index = search_skills2(skillnum, skilllist);
  if (victim->getLevel() < skilllist[index].levelavailable)
  {
    victim->send(QStringLiteral("You try to learn the basics of %1, but it is too advanced for you right now.\r\n").arg(skillname));
    return ReturnValue::eFAILURE;
  }

  if (!skillname.isEmpty())
    dc_snprintf(skill, sizeof(skill), "$BYou have learned the basics of %s.$R\r\n", qPrintable(skillname));
  else
  {
    victim->sendln("I just tried to teach you an invalid skill.  Tell a god.");
    ch->prog_error(QStringLiteral("Mpteachskill - invalid skill number"));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  victim->send(skill);

  victim->learn_skill(skillnum, 1, 1);

  prepare_character_for_sixty(ch);

  dc_snprintf(skill, sizeof(skill), "$N has learned the basics of %s.", qPrintable(skillname));
  act(skill, ch, 0, victim, TO_ROOM, NOTVICT);

  return ReturnValue::eSUCCESS;
}

qint32 determine_attack_type(QString attacktype)
{
  extern QStringList strs_damage_types;

  for (qint32 i = 10; *strs_damage_types[i] != '\n'; i++)
    if (!strcmp(strs_damage_types[i], attacktype))
      return (TYPE_HIT + i);

  if (!strcmp("undefined", attacktype))
    return TYPE_UNDEFINED;

  return 0;
}

QString Character::getTemp(QString name)
{
  tempvariable *eh;
  for (eh = tempVariable; eh; eh = eh->next)
    if (eh->name == name)
      return eh->data;
  return {};
}

command_return_t Character::do_mpsettemp(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim;
  QString arg = arguments.value(0);
  QString temp = arguments.value(1);
  QString arg2 = arguments.value(2);
  QString arg3 = arguments.value(3);
  if (arg.isEmpty() || temp.isEmpty() || arg2.isEmpty())
  {
    if (this->isNonPlayer())
    {
      qint32 num = DC::getInstance()->mob_index[this->mobdata->nr].vnum();

      DC::getInstance()->logentry(QStringLiteral("Mob %1 lacking argument for mpsettemp.").arg(num));
    }
    return ReturnValue::eFAILURE;
  }
  victim = get_char_room(arg, this->in_room);
  qint32 type = {};
  if (!victim)
  {
    if (arg == "allpc")
      type = 1;
    else if (arg == "allmob")
      type = 2;
    else if (arg == "all")
      type = 3;
  }

  if (!victim && type == 0)
    return ReturnValue::eFAILURE;
  if (!victim)
    victim = DC::getInstance()->world[this->in_room].people;

  for (; victim; victim = victim->next_in_room)
  {
    if (type == 1 && victim->isNonPlayer())
      continue;
    else if (type == 2 && victim->isPlayer())
      continue;

    tempvariable *eh;
    for (eh = victim->tempVariable; eh; eh = eh->next)
    {
      if (eh->name == temp)
        break;
    }
    if (eh)
    {
      eh->data = arg2;
    }
    else
    {
      eh = new tempvariable;

      eh->data = arg2;
      eh->name = temp;
      eh->next = victim->tempVariable;
      victim->tempVariable = eh;
      if (arg3 == "save")
        eh->save = 1;
      else
        eh->save = {};
    }
    if (type == 0)
      break;
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_mpsetalign(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString arg, align[MAX_INPUT_LENGTH];
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }
  if (!*argument || !ch->in_room)
    return ReturnValue::eFAILURE;
  half_chop(argument, arg, align);
  victim = get_char_room(arg, ch->in_room);
  if (!victim || (!is_number(align) && (!is_number(align + 1) || *align != '-')))
    return ReturnValue::eFAILURE;
  if (atoi(align) > 1000 || atoi(align) < -1000)
    return ReturnValue::eFAILURE;
  victim->alignment = atoi(align);
  return ReturnValue::eSUCCESS;
}

// List of people who have been recently damaged by mpdamage
// Used for connecting mpdamage with messaging
class damage_list
{
public:
  damage_list *next;
  QString name;
  qint32 damage; // Damage #
};

damage_list *dmg_list = {};

void free_dmg_list()
{
  damage_list *n;
  for (auto c = dmg_list; c; c = n)
  {
    n = c->next;
    c = {};
  }
  dmg_list = {};
}

void add_dmg(CharacterPtr ch, qint32 dmg)
{
  for (auto c = dmg_list; c; c = c->next)
  {
    if ((ch->isNonPlayer() && !str_cmp(c->name, qPrintable(ch->shortdesc_or_name()))) ||
        (ch->isPlayer() && !str_cmp(c->name, qPrintable(ch->name()))))
    {
      c->damage += dmg;
      return;
    }
  }

  auto c = new damage_list;
  if (ch->isNonPlayer())
    strcpy(c->name, qPrintable(ch->shortdesc_or_name()));
  else
    strcpy(c->name, qPrintable(ch->name()));
  c->damage = dmg;
  c->next = dmg_list;
  dmg_list = c;
}

command_return_t do_mpdamage(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  QString temp;
  QString damroll;
  QString attacktype;
  qint32 hitpoints = {};

  free_dmg_list();
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    ch->prog_error(QStringLiteral("Mpdamage - Bad syntax: Missing victim."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }

  CharacterPtr victim = {};

  // if it's 'all' leave victim nullptr and skip
  if (strcmp(arg, "all") && strcmp(arg, "allpc"))
  {
    victim = get_char_room(arg, ch->in_room);
    if (victim && (victim->getLevel() > MORTAL || victim->isNonPlayer())) // don't target immortals
      victim = {};

    if (!victim)
      return ReturnValue::eFAILURE; // not an error, just couldn't get valid vict
  }

  qint32 retval = ReturnValue::eSUCCESS;
  while (1)
  {
    argument = one_argument(argument, damroll);
    argument = one_argument(argument, attacktype);
    argument = one_argument(argument, temp);

    if (damroll[0] == '\0')
      break;
    qint32 numdice, sizedice;
    numdice = sizedice = {};
    bool perc = true;
    QChar t, l, o;
    qint32 plus = {};
    bool plusPerc = true;

    if (sscanf(damroll, "%dd%d%c%c%d%c", &numdice, &sizedice, &t, &l, &plus, &o) != 6 || l != '+' || t != '%' || o != '%')
    {
      plusPerc = false;
      if (sscanf(damroll, "%dd%d%c%c%d", &numdice, &sizedice, &t, &l, &plus) != 5 || l != '+' || t != '%')
        if (sscanf(damroll, "%dd%d%c", &numdice, &sizedice, &t) != 3 || t != '%')
        {
          perc = false;
          plusPerc = true;
          if (sscanf(damroll, "%dd%d%c%d%c", &numdice, &sizedice, &l, &plus, &o) != 5 || l != '+' || o != '%')
          {
            plusPerc = false;
            if (sscanf(damroll, "%dd%d%c%d", &numdice, &sizedice, &l, &plus) != 4 || l != '+')
              sscanf(damroll, "%dd%d", &numdice, &sizedice);
          }
        }
    }
    if (!numdice || !sizedice)
    {
      ch->prog_error(QStringLiteral("Mpdamage - Invalid damroll"));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }

    // figure out attack type
    qint32 damtype = determine_attack_type(attacktype);
    if (!damtype)
    {
      ch->prog_error(QStringLiteral("Mpdamage - Invalid damtype"));
      return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
    }
    qint32 dam = {};
    // do the damage
    // half of the casts are probably pointless, but whatever

    if (victim)
    {
      qint32 *data = {};
      if (!temp[0] || !str_cmp(temp, "hitpoints"))
      {
        hitpoints = ch->getHP();
        data = &hitpoints;
      }
      else if (!str_cmp(temp, "mana"))
      {
        data = &GET_MANA(victim);
      }
      else if (!str_cmp(temp, "ki"))
      {
        data = &GET_KI(victim);
      }
      else if (!str_cmp(temp, "move"))
      {
        data = victim->getMovePtr();
      }
      else
      {
        ch->prog_error(QStringLiteral("Mpdamage - Must damage either ki,mana,hitpoints or move"));
        return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
      }

      if (perc)
      {
        dam = (qint32)((double)(*data) / 100.0 * (double)dice(numdice, sizedice));
      }
      else
      {
        dam = dice(numdice, sizedice);
      }

      if (plusPerc)
      {
        dam += (qint32)((double)(*data) / 100.0 * (double)plus);
      }
      else
      {
        dam += plus;
      }

      add_dmg(victim, dam);
      if (!temp[0] || !str_cmp(temp, "hitpoints"))
      {
        retval = damage(ch, victim, dam, damtype, TYPE_UNDEFINED, 0, true);
        if (SOMEONE_DIED(retval))
          return retval;
      }
      else
      {
        *data -= dam;

        if (*data < 0)
        {
          *data = {};
        }
      }

      continue;
    }

    CharacterPtr next_vict;
    for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_vict)
    {
      next_vict = victim->next_in_room;
      if ((victim->isPlayer() && victim->getLevel() > MORTAL) || victim == ch)
      {
        continue;
      }

      if (arg == u"allpc"_s && victim->isNonPlayer())
      {
        continue;
      }
      qint32 *data = {};
      if (!temp[0] || !str_cmp(temp, "hitpoints"))
      {
        hitpoints = ch->getHP();
        data = &hitpoints;
      }
      else if (!str_cmp(temp, "mana"))
        data = &GET_MANA(victim);
      else if (!str_cmp(temp, "ki"))
        data = &GET_KI(victim);
      else if (!str_cmp(temp, "move"))
        data = victim->getMovePtr();
      else
      {
        ch->prog_error(QStringLiteral("Mpdamage - Must damage either ki,mana,hitpoints or move"));
        return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
      }
      if (perc)
        dam = (qint32)((double)(*data) / 100.0 * (double)dice(numdice, sizedice));
      else
        dam = dice(numdice, sizedice);
      if (plusPerc)
        dam += (qint32)((double)(*data) / 100.0 * (double)plus);
      else
        dam += plus;
      add_dmg(victim, dam);
      if (!temp[0] || !str_cmp(temp, "hitpoints"))
      {
        retval = damage(ch, victim, dam, damtype, TYPE_UNDEFINED, 0, true);
        if (isSet(retval, ReturnValue::eCH_DIED))
        {
          return retval;
        }
      }
      else
      {
        *data -= dam;
        if (*data < 0)
          *data = {};
      }

      if (SOMEONE_DIED(retval))
      {
        return retval;
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_mpothrow(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 mob_num;
  qint32 catch_num;
  qint32 delay;

  QString first;
  QString second;
  QString third;
  QString fourth;
  // locate and validate argument to find target
  argument = one_argument(argument, first);
  argument = one_argument(argument, second);
  argument = one_argument(argument, third);
  argument = one_argument(argument, fourth);

  if (isdigit(*first))
  {
    if (!check_valid_and_convert(mob_num, first) ||
        (real_object(mob_num) < 0))
    {
      ch->prog_error(QStringLiteral("Mpothrow - Invalid objnum."));
      return ReturnValue::eFAILURE;
    }
    *first = '\0';
  }
  else
  {
    if (strlen(first) >= MAX_THROW_NAME)
    {
      ch->prog_error(QStringLiteral("Mpthrow - Name too long."));
      return ReturnValue::eFAILURE;
    }
  }

  if (!check_range_valid_and_convert(catch_num, second, MPROG_CATCH_MIN,
                                     MPROG_CATCH_MAX))
  {
    ch->prog_error(QStringLiteral("Mpthrow - Invalid catch_num."));
    return ReturnValue::eFAILURE;
  }

  if (!check_range_valid_and_convert(delay, third, 0, 500))
  {
    ch->prog_error(QStringLiteral("Mpthrow - Invalid delay."));
    return ReturnValue::eFAILURE;
  }

  // create
  auto throwitem = new mprog_throw_type;
  throwitem->target_mob_num = mob_num;
  strcpy(throwitem->target_mob_name, first);
  throwitem->data_num = catch_num;
  throwitem->delay = delay;
  throwitem->mob = false;
  if (fourth[0] != '\0')
    throwitem->var = (fourth);
  else
    throwitem->var = {};

  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;

  return ReturnValue::eSUCCESS;
}

qint32 skill_aff[] =
    {
        4, 29, 18, 19, 20, 44,
        123, 36, 0, SPELL_EAS, 17, 145,
        33, 34, 84, 81, 86, 38,
        89, 0, 0, 0, 0, 0, SPELL_SOLIDITY, 72,
        SPELL_CANTQUIT, SPELL_KILLER, 56, 133, 74, 0,
        SPELL_SHADOWSLIP, SPELL_INSOMNIA, SPELL_FREEFLOAT,
        SPELL_FARSIGHT, SPELL_CAMOUFLAGE, SPELL_STABILITY,
        0, 0, SPELL_FOREST_MELD, SKILL_SONG_INSANE_CHANT,
        SKILL_SONG_GLITTER_DUST, SKILL_SONG_STICKY_LULL,
        0, SPELL_PROTECT_FROM_GOOD, SKILL_INNATE_POWERWIELD,
        SKILL_INNATE_REGENERATION, SKILL_INNATE_FOCUS,
        SPELL_KNOW_ALIGNMENT, 0, SPELL_WATER_BREATHING, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

command_return_t do_mpbestow(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg, arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
      arg3[MAX_INPUT_LENGTH];
  CharacterPtr victim, owner = {};
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }
  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);

  if (arg[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0')
    return ReturnValue::eFAILURE;
  if ((victim = get_char_room(arg, ch->in_room, true)) == nullptr && str_cmp(arg, "all") && str_cmp(arg, "allpc"))
  {
    ch->prog_error(QStringLiteral("Mpbestow - No such person."));
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  }
  qint32 i, o = {};
  if (!check_range_valid_and_convert(i, arg2, 0, 100))
    return ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR;
  check_range_valid_and_convert(o, arg3, 0, 10000);
  qint32 a = {};
  if (ch->beacon)
    owner = (CharacterPtr)ch->beacon;

  if (!victim)
    victim = DC::getInstance()->world[ch->in_room].people;
  qint32 z = {};
  for (; victim;)
  {
    for (; affected_bits[a][0] != '\n'; a++)
    {
      if (!str_cmp(affected_bits[a], arg1))
      {
        // debugpoint();
        affected_type af;
        af.type = z = skill_aff[a];
        if (victim->affected_by_spell(z + BASE_TIMERS))
        {
          //		victim->sendln("A s.");
          return ReturnValue::eFAILURE;
        }
        af.duration = i;
        af.bitvector = a + 1;
        af.location = {};
        af.modifier = 987; // Notifies that it's timered.
        affect_join(victim, &af, true, false);
        if (z && o) // Timer on it
        {
          af.type = BASE_TIMERS + z;
          af.duration = o;
          af.bitvector = -1;
          af.location = {};
          af.modifier = {};
          affect_join(victim, &af, true, false);
        }
      }
    }
    if (!str_cmp(arg, "all"))
      victim = victim->next_in_room;
    else if (!str_cmp(arg, "allpc"))
    {
      while ((victim = victim->next_in_room))
      {
        if (victim->isPlayer())
          break;
      }
    }
    else
      break;
  }
  if (z && o && owner) // Timer on it
  {
    affected_type af;
    af.type = BASE_TIMERS + z;
    af.duration = o;
    af.bitvector = -1;
    af.location = {};
    af.modifier = {};
    affect_join(owner, &af, true, false);
  }
  return ReturnValue::eSUCCESS;
}

// simulate a pause in proc execution
// stops prog, mpthrow a special kinda throw, picks it up again when delay is over
command_return_t do_mppause(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }
  qint32 delay;
  QString first;
  QString second;

  argument = one_argument(argument, first);
  mprog_throw_type *throwitem{};
  if (QString(first) == "all")
  {
    argument = one_argument(argument, second);

    if (!check_range_valid_and_convert(delay, second, 0, 65536))
    {
      ch->prog_error(QStringLiteral("mpppause all - Invalid delay."));
      return ReturnValue::eFAILURE;
    }
    throwitem = new mprog_throw_type;

    if (ch && ch->mobdata)
    {
      ch->mobdata->paused = true;
      throwitem->data_num = -1000;
    }
  }
  else
  {
    if (!check_range_valid_and_convert(delay, first, 0, 65536))
    {
      ch->prog_error(QStringLiteral("mppause - Invalid delay."));
      return ReturnValue::eFAILURE;
    }
    throwitem = new mprog_throw_type;
    throwitem->data_num = -999;
  }

  if (ch->isNonPlayer())
  {
    throwitem->target_mob_num = DC::getInstance()->mob_index[ch->mobdata->nr].vnum();
    throwitem->mob = true; // This is, suprisingly, a mob
  }
  else
  {
    throwitem->target_mob_num = {};
    throwitem->mob = false;
  }

  throwitem->target_mob_name[0] = '\0';
  throwitem->tMob = ch;
  throwitem->delay = delay;

  extern CharacterPtr activeActor;
  extern CharacterPtr activeRndm;
  extern ObjectPtr activeObj;
  extern void *activeVo;
  throwitem->actor = activeActor;
  throwitem->obj = activeObj;
  throwitem->vo = activeVo;
  throwitem->rndm = activeRndm;

  extern QString activeProg;
  extern QString activePos;
  extern QString activeProgTmpBuf;
  throwitem->orig = (activeProg);

  extern qint32 cIfs[256];
  throwitem->cPos = {};
  memcpy(&throwitem->ifchecks[0], &cIfs[0], sizeof(qint32) * 256);

  throwitem->startPos = activePos - activeProgTmpBuf;

  throwitem->var = {};
  throwitem->opt = {};
  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;
  return ReturnValue::eSUCCESS | ReturnValue::eDELAYED_EXEC;
}

command_return_t do_mpteleport(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString person, type[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  room_t to_room = {};

  if (ch->isPlayer() && ch->getLevel() < 110)
    return ReturnValue::eFAILURE;

  half_chop(argument, person, type);

  if (!*person)
  {
    strncpy(person, qPrintable(ch->name()), MAX_INPUT_LENGTH);
  }

  if (!(victim = get_char_vis(ch, person)))
  {
    if (person == u"area"_s)
    {
      victim = ch;
    }
    else
    {
      ch->sendln("No-one by that name around.");
      return ReturnValue::eFAILURE;
    }
  }

  if (isSet(DC::getInstance()->world[victim->in_room].room_flags, TELEPORT_BLOCK) ||
      IS_AFFECTED(victim, AFF_SOLIDITY))
  {
    ch->sendln("You find yourself unable to.");
    if (ch != victim)
    {
      dc_snprintf(buf, MAX_INPUT_LENGTH, "%s just tried to teleport you.\r\n", qPrintable(ch->shortdesc_or_name()));
      victim->send(buf);
    }
    return ReturnValue::eFAILURE;
  }

  qint32 i = DC::getInstance()->world[ch->in_room].zone,
         low = DC::getInstance()->zones.value(i).getRealBottom(),
         high = DC::getInstance()->zones.value(i).getRealTop(),
         attempts = {};

  do
  {
    if ((*type && type == u"area"_s) ||
        (victim == ch && *person && person == u"area"_s))
    {
      to_room = number(low, high);

      // Check to see if we're in an endless loop
      if (attempts++ > high - low)
      {
        return ReturnValue::eFAILURE;
      }
    }
    else
    {
      to_room = number<room_t>(1, DC::getInstance()->top_of_world);
    }
  } while (!DC::getInstance()->rooms.contains(to_room) ||
           isSet(DC::getInstance()->world[to_room].room_flags, PRIVATE) ||
           isSet(DC::getInstance()->world[to_room].room_flags, IMP_ONLY) ||
           isSet(DC::getInstance()->world[to_room].room_flags, NO_TELEPORT) ||
           isSet(DC::getInstance()->world[to_room].room_flags, ARENA) ||
           (DC::getInstance()->world[to_room].sector_type == SECT_UNDERWATER && victim->race != RACE_FISH) ||
           DC::getInstance()->zones.value(DC::getInstance()->world[to_room].zone).isNoTeleport() ||
           ((victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN)) ? (DC::getInstance()->zones.value(DC::getInstance()->world[to_room].zone).isTown()) : false) ||
           (IS_AFFECTED(victim, AFF_CHAMPION) && (isSet(DC::getInstance()->world[to_room].room_flags, CLAN_ROOM) ||
                                                  (to_room >= 1900 && to_room <= 1999))));

  act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
  move_char(victim, to_room);
  act("$n slowly fades into existence.", victim, 0, 0, TO_ROOM, 0);

  do_look(victim, "");
  return ReturnValue::eSUCCESS;
}

command_return_t do_mppeace(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eSUCCESS;
  }
  QString arg;
  argument = one_argument(argument, arg);

  CharacterPtr rch, vict = {};

  if (arg[0])
  {
    vict = get_char_room(arg, ch->in_room);
    if (!vict)
    {
      ch->prog_error(QStringLiteral("Mppeace - Vict not found."));
      return ReturnValue::eFAILURE;
    }
    if (vict->isNonPlayer() && vict->mobdata->hated != nullptr)
      remove_memory(vict, 'h');
    if (vict->fighting != nullptr)
      stop_fighting(vict);
    return ReturnValue::eSUCCESS;
  }
  for (rch = DC::getInstance()->world[ch->in_room].people; rch != nullptr; rch = rch->next_in_room)
  {
    if (rch->isNonPlayer() && rch->mobdata->hated != nullptr)
      remove_memory(rch, 'h');
    if (rch->fighting != nullptr)
      stop_fighting(rch);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_mpretval(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  const QStringList retvals =
      {
          "true",
          "false",
          "\n"};
  QString arg;
  one_argument(argument, arg);
  qint32 retval = ReturnValue::eSUCCESS;

  for (qint32 i = {}; retvals[i][0] != '\n'; i++)
    if (!str_cmp(arg, retvals[i]))
      SET_BIT(retval, 1 << (5 + i));

  return retval;
}

// Yes, I'm sure there's a better way to do this. Stop whining.
qint32 process_math(CharacterPtr ch, QString string)
{
  qint32 result = 0, curr = {};
  QChar lastsign = '\0';
  bool numproc = false;
  if (!string)
    return -9839;

  while (1)
  {
    if (*string == ' ')
    {
      string++;
      continue;
    }
    if (isdigit(*string))
    {
      numproc = true;
      curr *= 10;
      curr += (*string - '0');
      string++;
      continue;
    }
    else if (numproc)
    {
      if (lastsign == '\0')
        result = curr;
      else
      {
        switch (lastsign)
        {
        case '+':
          result += curr;
          break;
        case '-':
          result -= curr;
          break;
        case '/':
          result /= curr;
          break;
        case '*':
          result *= curr;
          break;
        }
      }
      curr = {};
    }
    if (*string == '\0' || *string == '\r' || *string == '\n')
      break;
    switch (*string)
    {
    case '+':
    case '-':
    case '/':
    case '*':
      curr = {};
      lastsign = *string;
      break;
    default:
      ch->prog_error(QStringLiteral("Mpsetmath - Unknown symbol: '%1'.").arg(*string));
      return result;
      break;
    };
    string++;
  }
  return result;
}

// Unreadable code > you
QString expand_data(CharacterPtr ch, QString orig)
{
  static QString buf;
  QString buf1;
  while (*orig == ' ')
    orig++;
  strcpy(buf1, orig);
  orig = &buf1[0];
  buf[0] = '\0';
  QString ptr;
  qint32 i = 0, l, r, z = 0, o;
  QChar c;
  while (1)
  {
    QString left;
    QString right;
    ptr = strchr(orig, '.');
    if (ptr == orig)
      break;
    if (!ptr)
      break;
    *ptr = '#';
    for (l = 1;; l++)
    {
      if ((ptr - l) == orig)
        break;
      if (!isalpha(*(ptr - l)) && (*(ptr - l) != ',') &&
          (*(ptr - l) != '_'))
      {
        // Failsafe to ensure no 'reserved' characters are being used, ie. # ~
        l--;
        break;
      }
    }
    *(ptr) = '\0';
    strcpy(left, (ptr - l));
    *(ptr) = '#';
    *(ptr - l) = '~';
    for (r = 1;; r++)
    {
      if ((ptr + r) == 0)
        break;
      if (!isalpha(*(ptr + r)) && (*(ptr + r) != ',') && (*(ptr - l) != '_'))
        break;
    }
    c = *(ptr + r);
    *(ptr + r) = '\0';
    strcpy(right, ptr + 1);
    *(ptr + r) = c;
    r--;
    if (c == '\0')
      *(ptr + r) = '~';

    qint16 *lvali = {};
    quint32 *lvalui = {};
    QString *lvalstr = {};
    QString lvalqstr;
    qint64 *lvali64 = {};
    quint64 *lvalui64 = {};
    qint8 *lvalb = {};
    translate_value(left, right, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, ch, activeActor, activeObj, activeVo, activeRndm, lvalqstr);

    if (!lvali && !lvalui && !lvalb)
    {
      ch->prog_error(QStringLiteral("Mpsetmath - Expand_data - Accessing unknown data."));
      return {};
    }

    for (o = z;; o++)
    {
      if (*(orig + o) == '~' || *(orig + o) == '\0')
        break;
      buf[i++] = *(orig + o);
    }
    buf[i] = '\0';
    QString tmp;
    if (lvali)
      dc_sprintf(tmp, "%d", *lvali);
    if (lvalui)
      dc_sprintf(tmp, "%u", *lvalui);
    if (lvalb)
      dc_sprintf(tmp, "%d", *lvalb);
    strcat(buf, tmp);
    i += strlen(tmp);
    z = (ptr - orig) + r + 1;
  }

  for (o = z;; o++)
  {
    if (*(orig + o) == '~' || *(orig + o) == '\0')
      break;
    buf[i++] = *(orig + o);
  }
  buf[i] = '\0';
  return &buf[0];
}

const QStringList allowedData = {
    "hitpoints", "mana", "move", nullptr};

command_return_t do_mpsetmath(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (ch->isPlayer())
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  CharacterPtr vict;
  //  if (activeActor) activeActor->send(QStringLiteral("{%1}\r\n").arg(arg));
  //  vict = get_pc("Urizen");
  //  if (vict) vict->send(QStringLiteral("{%1}\r\n").arg(arg));

  QString arg1;
  QString arg2;
  arg = one_argument(arg, arg1);
  QString r = {};

  if ((r = strchr(arg1, '.')) != nullptr)
  {
    *r = '\0';
    r++;
    one_argument(r, arg2);
  }
  if (!r || !*r || !*arg1)
  {
    ch->prog_error(QStringLiteral("Mpsetmath - Invalid primary data."));
    return ReturnValue::eFAILURE;
  }
  bool allowed = false;

  qint16 *lvali = {};
  quint32 *lvalui = {};
  QString *lvalstr = {};
  QString lvalqstr;
  qint64 *lvali64 = {};
  quint64 *lvalui64 = {};
  qint8 *lvalb = {};
  translate_value(arg1, r, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, ch, activeActor, activeObj, activeVo, activeRndm, lvalqstr);

  vict = activeTarget;
  if (!vict)
  {
    // ch->prog_error( QStringLiteral("Mpsetmath - No target."));
    // return ReturnValue::eFAILURE;
  }
  if (vict && vict->isNonPlayer())
    allowed = true;
  else if (vict)
    for (qint32 i = {}; allowedData[i]; i++)
      if (!str_cmp(allowedData[i], r))
        allowed = true;

  if (!allowed && vict)
  {
    ch->prog_error(QStringLiteral("Mpsetmath - Accessing unallowed data."));
    return ReturnValue::eFAILURE;
  }

  if (lvalstr)
  {
    QString nw;
    arg = one_argument(arg, nw);
    if (!nw[0])
    {
      ch->prog_error(QStringLiteral("Mpsetmath - Invalid string."));
      return ReturnValue::eFAILURE;
    }
    *lvalstr = (nw);
    return ReturnValue::eSUCCESS;
  }
  if (!lvalui && !lvali && !lvalb)
  {
    ch->prog_error(QStringLiteral("Mpsetmath - Accessing unknown data."));
    return ReturnValue::eFAILURE;
  }

  QString fixed = expand_data(ch, arg);

  qint32 i = process_math(ch, fixed);

  if (i == -9839)
  {
    ch->prog_error(QStringLiteral("Mpsetmath - Invalid math-string."));
    return ReturnValue::eFAILURE;
  }
  if (lvali)
  {
    *lvali = i;
    //  ch->prog_error( QStringLiteral("Mpsetmath - %1 set to %2."));
    //  r, i, DC::getInstance()->mob_index[ch->mobdata->nr].vnum() );
  }
  if (lvalb)
  {
    *lvalb = (qint8)i;
    //  ch->prog_error( QStringLiteral("Mpsetmath - %1 set to %2."));
    //  r, i, DC::getInstance()->mob_index[ch->mobdata->nr].vnum() );
  }
  if (lvalui)
  {
    *lvalui = (quint32)i;
    //  ch->prog_error( QStringLiteral("Mpsetmath - %1 set to %2."));
    //  r, i, DC::getInstance()->mob_index[ch->mobdata->nr].vnum() );
  }

  /*  vict->sendln(QStringLiteral("%d\r\n%d\r\n%d\r\n%d\r\n",
      process_math(ch, "1+1-3+5*10"),
      process_math(ch, "1-3+5*10/2"),
      process_math(ch, "1+101-34+5*10*2/8"),
      process_math(ch, "1+101-34+5*10*2/8")
      );
    */
  return ReturnValue::eSUCCESS;
}

void Character::prog_error(QString error_message)
{
  if (IS_OBJ(this))
  {
    logworld(QStringLiteral("Obj %1, com %2, line %3: %4").arg(dc_->obj_index[objdata->item_number].vnum()).arg(mprog_command_num).arg(mprog_line_num).arg(error_message));
  }
  else if (this->isNonPlayer())
  {
    logworld(QStringLiteral("Mob %1, com %2, line %3: %4").arg(dc_->mob_index[mobdata->nr].vnum()).arg(mprog_command_num).arg(mprog_line_num).arg(error_message));
  }
  else
  {
    logworld(QStringLiteral("Unknown prog: %1").arg(error_message));
  }
}
