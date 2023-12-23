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
#include <cstdarg>

#include "fileinfo.h"
#include "act.h"
#include "player.h"
#include "levels.h"
#include "room.h"
#include "structs.h"
#include "fight.h"
#include "spells.h"
#include "utility.h"
#include "connect.h"
#include "interp.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "returnvals.h"
#include "innate.h"
#include "arena.h"
#include "race.h"
#include "const.h"
#include "guild.h"

// external vars

extern struct index_data *mob_index;

extern bool MOBtrigger;
extern struct mprog_throw_type *g_mprog_throw_list;

extern Character *activeActor;
extern Character *activeRndm;
extern Character *activeTarget;
extern Object *activeObj;
extern void *activeVo;

extern room_t top_of_world;
extern struct index_data *obj_index;
extern int mprog_line_num;    // From mob_prog.cpp
extern int mprog_command_num; // From mob_prog.cpp

/*
 * Local functions.
 */

char *mprog_type_to_name(int type);

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. It allows the words to show up in mpstat to
 *  make it just a hair bit easier to see what a mob should be doing.
 */

char *mprog_type_to_name(int type)
{
  switch (type)
  {
  case IN_FILE_PROG:
    return "in_file_prog";
  case ACT_PROG:
    return "act_prog";
  case SPEECH_PROG:
    return "speech_prog";
  case RAND_PROG:
    return "rand_prog";
  case ARAND_PROG:
    return "arand_prog";
  case FIGHT_PROG:
    return "fight_prog";
  case HITPRCNT_PROG:
    return "hitprcnt_prog";
  case DEATH_PROG:
    return "death_prog";
  case ENTRY_PROG:
    return "entry_prog";
  case GREET_PROG:
    return "greet_prog";
  case ALL_GREET_PROG:
    return "all_greet_prog";
  case GIVE_PROG:
    return "give_prog";
  case BRIBE_PROG:
    return "bribe_prog";
  case CATCH_PROG:
    return "catch_prog";
  case ATTACK_PROG:
    return "attack_prog";
  case LOAD_PROG:
    return "load_prog";
  case CAN_SEE_PROG:
    return "can_see_prog";
  case DAMAGE_PROG:
    return "damage_prog";
  case COMMAND_PROG:
    return "command_prog";
  default:
    return "ERROR_PROG";
  }
}

/* A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MOBprograms which are set.
 */

void mpstat(Character *ch, Character *victim)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  mob_prog_data *mprg;
  int i;

  sprintf(buf, "$3Name$R: %s  $3Vnum$R: %d.\r\n",
          victim->getNameC(), mob_index[victim->mobdata->nr].virt);
  ch->send(buf);

  sprintf(buf, "$3Short description$R: %s\n\r$3Long  description$R: %s\r\n",
          victim->short_desc,
          victim->long_desc ? victim->long_desc : "(nullptr)");
  ch->send(buf);

  if (!(mob_index[victim->mobdata->nr].progtypes))
  {
    ch->sendln("That mob has no programs set.");
    return;
  }

  for (mprg = mob_index[victim->mobdata->nr].mobprogs, i = 1; mprg != nullptr;
       i++, mprg = mprg->next)
  {
    sprintf(buf, "$3%d$R>$3$B", i);
    ch->send(buf);
    send_to_char(mprog_type_to_name(mprg->type), ch);
    ch->send("$R ");
    sprintf(buf, "$B$5%s$R\n\r", mprg->arglist);
    ch->send(buf);
    ch->sendRaw(std::string(mprg->comlist) + "\r\n");
  }
}

/* prints the argument to all the rooms aroud the mobile */

int do_mpasound(Character *ch, char *argument, int cmd)
{

  int32_t was_in_room;
  int door;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  if (argument[0] == '\0')
  {
    prog_error(ch, "Mpasound - No argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  was_in_room = ch->in_room;
  for (door = 0; door <= 5; door++)
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
  return eSUCCESS;
}

/* lets the mobile kill any player or mobile without murder*/

int do_mpkill(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  Character *victim;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    prog_error(ch, "MpKill - no argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if ((victim = ch->get_char_room_vis(arg)) == nullptr)
  {
    prog_error(ch, "MpKill - Victim not in room.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (victim == ch)
  {
    prog_error(ch, "MpKill - Bad victim to attack.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
  {
    prog_error(ch, "MpKill - Charmed mob attacking master.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (ch->isFighting())
  {
    prog_error(ch, "MpKill - Already fighting");
    return eFAILURE | eINTERNAL_ERROR;
  }

  return attack(ch, victim, TYPE_UNDEFINED);
}

int do_mphit(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  Character *victim;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    prog_error(ch, "MpHit - no argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if ((victim = ch->get_char_room_vis(arg)) == nullptr)
  {
    prog_error(ch, "MpHit - Victim not in room.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (GET_POS(victim) == position_t::DEAD)
  {
    prog_error(ch, "MpHit - Victim already dead.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (victim == ch)
  {
    prog_error(ch, "MpHit - Bad victim to attack.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
  {
    prog_error(ch, "MpHit - Charmed mob attacking master.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  return one_hit(ch, victim, TYPE_UNDEFINED, FIRST);
}

int do_mpaddlag(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  Character *victim;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);

  if (arg[0] == '\0')
  {
    prog_error(ch, "MpAddlag - no argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if ((victim = ch->get_char_room_vis(arg)) == nullptr)
  {
    prog_error(ch, "MpAddlag - Victim not in room.");
    return eFAILURE | eINTERNAL_ERROR;
  }
  if (!arg1[0] || !is_number(arg1))
  {
    prog_error(ch, "MpAddlag - Invalid duration.");
    return eFAILURE | eINTERNAL_ERROR;
  }
  WAIT_STATE(victim, atoi(arg1));
  return eSUCCESS;
}

/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy
   just plain everything  */

int do_mpjunk(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  Object *obj;
  int location;
  bool dot = false;
  char dotbuf[MAX_INPUT_LENGTH];
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }
  dotbuf[0] = '\0';
  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    prog_error(ch, "Mpjunk - No argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (str_cmp(arg, "all") && !sscanf(arg, "all.%s", dotbuf))
  {
    if ((obj = ch->get_object_in_equip_vis(arg, ch->equipment, &location, false)))
    {
      extract_obj(unequip_char(ch, location));
      return eSUCCESS;
    }
    if ((obj = get_obj_in_list(arg, ch->carrying)))
    {
      extract_obj(obj);
      return eSUCCESS;
    }
  }
  else
  {
    if (dotbuf[0] != '\0')
      dot = true;

    for (int l = 0; l < MAX_WEAR; l++)
      if (ch->equipment[l])
        if (!dot || isexact(dotbuf, ch->equipment[l]->name))
          extract_obj(unequip_char(ch, l));

    Object *x, *v;
    for (x = ch->carrying; x; x = v)
    {
      v = x->next_content;
      if (!dot || isexact(dotbuf, x->name))
        extract_obj(x);
    }
    return eSUCCESS;
  }
  return eFAILURE;
}

/* prints the message to everyone in the room other than the mob and victim */

int do_mpechoaround(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  Character *victim;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    prog_error(ch, "Mpechoaround - No argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (!(victim = get_char_room(arg, ch->in_room, true)))
  {
    prog_error(ch, "Mpechoaround - victim does not exist.");
    return eFAILURE | eINTERNAL_ERROR;
  }
  if (CAN_SEE(ch, victim))
    act(argument + 1, ch, nullptr, victim, TO_ROOM, NOTVICT);
  return eSUCCESS;
}

int do_mpechoaroundnotbad(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
  Character *victim, *victim2;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);
  if (arg[0] == '\0' || arg1[0] == '\0')
  {
    prog_error(ch, "Mpechoaroundnotbad - No argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (!(victim = get_char_room(arg, ch->in_room)))
  {
    prog_error(ch, "Mpechoaroundnotbad - victim does not exist.");
    return eFAILURE | eINTERNAL_ERROR;
  }
  if (!(victim2 = get_char_room(arg1, ch->in_room)))
  {
    prog_error(ch, "Mpechoaroundnotbad - victim does not exist.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  //     if (CAN_SEE(ch,victim))
  act(argument + 1, victim, nullptr, victim2, TO_ROOM, NOTVICT);
  return eSUCCESS;
}

/* prints the message to only the victim */

int do_mpechoat(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  Character *victim;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0')
  {
    prog_error(ch, "Mpechoat - No argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (!(victim = get_char_room(arg, ch->in_room, true)))
  {
    prog_error(ch, "Mpechoat - victim does not exist.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  act(argument + 1, ch, nullptr, victim, TO_VICT, 0);
  return eSUCCESS;
}

/* prints the message to the room at large */

int do_mpecho(Character *ch, char *argument, int cmd)
{
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  if (argument[0] == '\0')
  {
    prog_error(ch, "Mpecho - called w/o argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  act(argument + 1, ch, nullptr, nullptr, TO_ROOM, 0);
  return eSUCCESS;
}

/* lets the mobile load an item or mobile.  All items
are loaded into inventory.  you can specify a level with
the load object portion as well. */

int do_mpmload(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  int realnum;
  Character *victim;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0' || !is_number(arg))
  {
    prog_error(ch, "Mpmload - Bad vnum as arg.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if ((realnum = real_mobile(atoi(arg))) < 0)
  {
    prog_error(ch, "Mpmload - Bad mob vnum.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  victim = clone_mobile(realnum);
  victim->hometown = DC::getInstance()->world[ch->in_room].number;
  char_to_room(victim, ch->in_room);
  mprog_load_trigger(victim); // victim not used after, no selfpurge checks, leave the selfpurge of the mobprog that is causing this load intact as whatever it is

  return eSUCCESS;
}

int do_mpoload(Character *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH] = {0};
  char arg2[MAX_INPUT_LENGTH] = {0};
  Object *obj;
  int realnum;
  extern struct index_data *obj_index;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg1);
  one_argument(argument, arg2);

  if (arg1[0] == '\0' || !is_number(arg1))
  {
    prog_error(ch, "Mpoload - Bad syntax.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if ((realnum = real_object(atoi(arg1))) < 0)
  {
    prog_error(ch, "Mpoload - Bad vnum arg.");
    return eFAILURE | eINTERNAL_ERROR;
  }
  obj = clone_object(realnum);

  if (obj_index[obj->item_number].virt == 393 && isSet(DC::getInstance()->world[ch->in_room].room_flags, ARENA) && arena.type == POTATO && ArenaIsOpen())
  {
    return eFAILURE;
  }

  if (!strcasecmp(arg2, "random"))
  {
    randomize_object(obj);
  }

  if (CAN_WEAR(obj, ITEM_TAKE))
  {
    obj_to_char(obj, ch);
  }
  else
  {
    obj_to_room(obj, ch->in_room);
  }

  return eSUCCESS;
}

/* lets the mobile purge all objects and other npcs in the room,
   or purge a specified object or mob in the room.  It can purge
   itself, but this had best be the last command in the MOBprogram
   otherwise ugly stuff will happen */

int do_mppurge(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  Character *victim;
  Object *obj;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    /* 'purge' */
    Character *vnext;
    Object *obj_next;

    for (victim = DC::getInstance()->world[ch->in_room].people; victim != nullptr; victim = vnext)
    {
      vnext = victim->next_in_room;
      if (IS_NPC(victim) && victim != ch)
      {
        extract_char(victim, true);
      }
    }

    for (obj = DC::getInstance()->world[ch->in_room].contents; obj != nullptr; obj = obj_next)
    {
      obj_next = obj->next_content;
      extract_obj(obj);
    }

    return eSUCCESS;
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
    //	    prog_error(ch, "Mppurge - Bad argument.");
    //            return eFAILURE|eINTERNAL_ERROR;
    //	}
    return eSUCCESS;
  }

  if (IS_PC(victim))
  {
    prog_error(ch, "Mppurge - Purging a PC.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  //    issame = (ch == victim);
  if (ch == victim)
  {
    // logf(0, LogChannels::LOG_BUG, "selfpurge on %s to %s", GET_NAME(ch), victim->getNameC());
    selfpurge = true;
    selfpurge.setOwner(ch, "do_mppurge");
  }
  extract_char(victim, true);

  //    if(issame)

  // we have to return both ch, and vict, since this could be in a greet or entry prog
  // or a number of other possibilities and we dunno who died.  Otherwise we continue
  // trying to move someone that is dead.

  return eSUCCESS | eCH_DIED | eVICT_DIED;

  //    return eSUCCESS;
}

/* lets the mobile goto any location it wishes that is not private */

int do_mpgoto(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  int32_t location = -1;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg);
  if (arg[0] == '\0')
  {
    prog_error(ch, "Mpgoto - No argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  // TODO - make this work with strings (goto chode) too

  Character *vict;
  if (!str_cmp(arg, "mob"))
  {
    one_argument(argument, arg);
    if (arg[0] == '\0' || !is_number(arg))
    {
      prog_error(ch, "Mpgoto - Missing vnum after 'mob' argument.");
      return eFAILURE | eINTERNAL_ERROR;
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
      prog_error(ch, "Mpgoto - Missing arg after 'pc' argument.");
      return eFAILURE | eINTERNAL_ERROR;
    }
    if (!(vict = get_pc(arg)))
      location = -1;
    else
      location = vict->in_room;
  }
  else
  {
    Object *obj;
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
    prog_error(ch, "Mpgoto - No such location.");
    return eFAILURE | eINTERNAL_ERROR;
  }
  if (location == ch->in_room)
    return eFAILURE; // zz
  extern room_t top_of_world;
  if (location > top_of_world || !DC::getInstance()->rooms.contains(location))
    location = 0;

  if (ch->fighting != nullptr)
    stop_fighting(ch);

  char_from_room(ch);
  char_to_room(ch, location);

  return eSUCCESS;
}

/* lets the mobile do a command at another location. Very useful */

int do_mpat(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  int32_t location;
  int32_t original;
  int result;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0')
  {
    prog_error(ch, "Mpat - Bad argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  // TODO - make location take args
  Character *vict;
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
    prog_error(ch, "do_mpat - No such location.");
    return eFAILURE | eINTERNAL_ERROR;
  }
  extern room_t top_of_world;
  if (location > top_of_world || !DC::getInstance()->rooms.contains(location))
  {
    if (!DC::getInstance()->rooms.contains(1))
    {
      ch->send(QString("mpat - Room %1 invalid. Tried room 1 but it's invalid too.\r\n").arg(location));
      return eFAILURE | eINTERNAL_ERROR;
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

  if (isSet(result, eCH_DIED))
    return result;

  char_from_room(ch);
  char_to_room(ch, original);

  return result;
}

// Reward the player with some XP
// Also works with -xp to penalize
int do_mpxpreward(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  int reward;
  Character *vict;

  Character *get_pc_room_vis_exact(Character * ch, const char *name);

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  half_chop(argument, arg, buf);

  if (arg[0] == '\0' || !(vict = get_pc_room_vis_exact(ch, arg)))
  {
    prog_error(ch, "Mpxpreward - Bad argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (!check_valid_and_convert(reward, buf))
  {
    prog_error(ch, "Mpxpreward - Bad argument.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (reward < 0)
    if ((GET_EXP(vict) + reward) < 0)
    {
      csendf(vict, "You lose %d exps.\r\n", (-1 * reward));
      GET_EXP(vict) = 0;
      return eSUCCESS;
    }

  vict->send(QString("You receive %1 exps.\r\n").arg(reward));
  gain_exp(vict, reward);
  return eSUCCESS;
}

/* lets the mobile transfer people.  the all argument transfers
   everyone in the current room to the specified location */

int do_mptransfer(Character *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int32_t location;
  Connection *d;
  Character *victim;

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if (arg1[0] == '\0')
  {
    prog_error(ch, "Mptransfer - Bad syntax");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (!str_cmp(arg1, "all"))
  {
    for (d = DC::getInstance()->descriptor_list; d != nullptr; d = d->next)
    {
      if (d->connected == Connection::states::PLAYING && d->character != ch && d->character->in_room == ch->in_room && CAN_SEE(ch, d->character))
      {
        char buf[MAX_STRING_LENGTH];
        sprintf(buf, "%s %s", d->character->getNameC(), arg2);
        do_mptransfer(ch, buf, cmd);
      }
    }
    return eSUCCESS;
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
      prog_error(ch, "Mptransfer - No such location.");
      return eFAILURE | eINTERNAL_ERROR;
    }

    if (isSet(DC::getInstance()->world[location].room_flags, PRIVATE))
    {
      prog_error(ch, "Mptransfer - Private room.");
      return eFAILURE | eINTERNAL_ERROR;
    }
  }

  if ((victim = get_char_vis(ch, arg1)) == nullptr)
  {
    prog_error(ch, "Mptransfer - No such person.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (victim->in_room == DC::NOWHERE)
  {
    prog_error(ch, "Mptransfer - Victim in Limbo.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  if (victim->fighting != nullptr)
    stop_fighting(victim);

  char_from_room(victim);
  char_to_room(victim, location);

  return eSUCCESS;
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */

int do_mpforce(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0')
  {
    prog_error(ch, "Mpforce - Bad syntax");
    return eFAILURE | eINTERNAL_ERROR;
  }

  // TODO - this is dangerous...need to rework it.  If the person we force kills the
  // next person we force, who was a mob, we can crash cause vch_next is no longer valid

  if (!str_cmp(arg, "all"))
  {
    Character *vch;
    Character *vch_next;

    for (vch = DC::getInstance()->world[ch->in_room].people; vch != nullptr; vch = vch_next)
    {
      vch_next = vch->next_in_room;

      if (vch->getLevel() < IMMORTAL && CAN_SEE(ch, vch))
      {
        vch->command_interpreter(QString(argument));
      }
    }
  }
  else
  {
    Character *victim;

    if ((victim = ch->get_char_room_vis(arg)) == nullptr)
    {
      prog_error(ch, "Mpforce - No such victim.");
      return eFAILURE | eINTERNAL_ERROR;
    }

    if (victim == ch)
    {
      prog_error(ch, "Mpforce - Forcing oneself");
      return eFAILURE | eINTERNAL_ERROR;
    }

    if (CAN_SEE(ch, victim))
    {
      if (victim->getLevel() < IMMORTAL)
        victim->command_interpreter(QString(argument));
      else
      {
        prog_error(ch, "Tried to MOBProg force %s to '%s'.",
                   victim->getNameC(), argument);
      }
    }
  }

  return eSUCCESS;
}

// "Throw" a message to another mob.  Right now, it's only an int specifying which
// 'catch' should handle it
// argument should be <mob> <catchnum> <delay>
int do_mpthrow(Character *ch, char *argument, int cmd)
{
  struct mprog_throw_type *throwitem = nullptr;
  int mob_num;
  int catch_num;
  int delay;

  char first[MAX_INPUT_LENGTH];
  char second[MAX_INPUT_LENGTH];
  char third[MAX_INPUT_LENGTH];
  char fourth[MAX_INPUT_LENGTH];
  char fifth[MAX_INPUT_LENGTH];
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
      prog_error(ch, "Mpthrow - Invalid mobnum.");
      return eFAILURE;
    }
    *first = '\0';
  }
  else
  {
    if (strlen(first) >= MAX_THROW_NAME)
    {
      prog_error(ch, "Mpthrow - Name too long.");
      return eFAILURE;
    }
  }

  if (!check_range_valid_and_convert(catch_num, second, MPROG_CATCH_MIN, MPROG_CATCH_MAX))
  {
    prog_error(ch, "Mpthrow - Invalid catch_num.");
    return eFAILURE;
  }

  if (!check_range_valid_and_convert(delay, third, 0, 500000))
  {
    prog_error(ch, "Mpthrow - Invalid delay.");
    return eFAILURE;
  }

  int opt = 0;
  if (!check_range_valid_and_convert(opt, fifth, 0, 50000))
    opt = 0;

  // create struct
  throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct mprog_throw_type));
  throwitem->target_mob_num = mob_num;
  strcpy(throwitem->target_mob_name, first);
  throwitem->data_num = catch_num;
  throwitem->delay = delay;
  throwitem->mob = true; // This is, suprisingly, a mob
  throwitem->actor = activeActor;
  throwitem->obj = activeObj;
  throwitem->vo = activeVo;
  throwitem->rndm = nullptr;

  if (fourth[0] != '\0')
    throwitem->var = str_dup(fourth);
  else
    throwitem->var = nullptr;
  throwitem->opt = opt;
  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;

  return eSUCCESS;
}

int do_mpteachskill(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  char skill[MAX_INPUT_LENGTH];

  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  half_chop(argument, arg, skill);

  if (arg[0] == '\0' || skill[0] == '\0')
  {
    prog_error(ch, "Mpteachskill - Bad syntax.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  Character *victim;

  if ((victim = get_char_room(arg, ch->in_room)) == nullptr)
  {
    prog_error(ch, "Mpteachskill - No such victim.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  int skillnum;
  if (!(check_range_valid_and_convert(skillnum, skill, 0, SKILL_SONG_MAX)))
  {
    prog_error(ch, "Mpteachskill - No such skill");
    return eFAILURE | eINTERNAL_ERROR;
  }

  const char *skillname = get_skill_name(skillnum);

  if (victim->has_skill(skillnum))
  {
    csendf(victim, "You already know the basics of %s!\r\n", skillname ? skillname : "Unknown");
    return eFAILURE;
  }

  class_skill_defines *skilllist = victim->get_skill_list();
  if (!skilllist)
  {
    prog_error(ch, "Mpteachskill - %s had no skill list?", victim->getNameC());
    return eFAILURE; // no skills to train
  }

  int index = search_skills2(skillnum, skilllist);
  if (victim->getLevel() < skilllist[index].levelavailable)
  {
    victim->send(QString("You try to learn the basics of %1, but it is too advanced for you right now.\r\n").arg(skillname));
    return eFAILURE;
  }

  if (skillname)
    sprintf(skill, "$BYou have learned the basics of %s.$R\n\r", skillname);
  else
  {
    victim->sendln("I just tried to teach you an invalid skill.  Tell a god.");
    prog_error(ch, "Mpteachskill - invalid skill number");
    return eFAILURE | eINTERNAL_ERROR;
  }

  victim->send(skill);

  victim->learn_skill(skillnum, 1, 1);

  prepare_character_for_sixty(ch);

  sprintf(skill, "$N has learned the basics of %s.", skillname);
  act(skill, ch, 0, victim, TO_ROOM, NOTVICT);

  return eSUCCESS;
}

int determine_attack_type(char *attacktype)
{
  extern char *strs_damage_types[];

  for (int i = 10; *strs_damage_types[i] != '\n'; i++)
    if (!strcmp(strs_damage_types[i], attacktype))
      return (TYPE_HIT + i);

  if (!strcmp("undefined", attacktype))
    return TYPE_UNDEFINED;

  return 0;
}

QString Character::getTemp(QString name)
{
  struct tempvariable *eh;
  for (eh = tempVariable; eh; eh = eh->next)
    if (eh->name == name)
      return eh->data;
  return nullptr;
}

command_return_t Character::do_mpsettemp(QStringList arguments, int cmd)
{
  Character *victim;
  if (IS_PC(this) && cmd != CMD_OTHER)
  {
    this->sendln("Huh?");
    return eFAILURE;
  }

  QString arg = arguments.value(0);
  QString temp = arguments.value(1);
  QString arg2 = arguments.value(2);
  QString arg3 = arguments.value(3);
  if (arg[0] == '\0' || temp[0] == '\0' || arg2[0] == '\0')
  {
    if (IS_NPC(this))
    {
      int num = mob_index[this->mobdata->nr].virt;

      logentry(QString("Mob %1 lacking argument for mpsettemp.").arg(num));
    }
    return eFAILURE;
  }
  victim = get_char_room(arg, this->in_room);
  int type = 0;
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
    return eFAILURE;
  if (!victim)
    victim = DC::getInstance()->world[this->in_room].people;

  for (; victim; victim = victim->next_in_room)
  {
    if (type == 1 && IS_NPC(victim))
      continue;
    else if (type == 2 && IS_PC(victim))
      continue;

    struct tempvariable *eh;
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
        eh->save = 0;
    }
    if (type == 0)
      break;
  }
  return eSUCCESS;
}

int do_mpsetalign(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char arg[MAX_INPUT_LENGTH], align[MAX_INPUT_LENGTH];
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }
  if (!*argument || !ch->in_room)
    return eFAILURE;
  half_chop(argument, arg, align);
  victim = get_char_room(arg, ch->in_room);
  if (!victim || (!is_number(align) && (!is_number(align + 1) || *align != '-')))
    return eFAILURE;
  if (atoi(align) > 1000 || atoi(align) < -1000)
    return eFAILURE;
  victim->alignment = atoi(align);
  return eSUCCESS;
}

// List of people who have been recently damaged by mpdamage
// Used for connecting mpdamage with messaging
struct damage_list
{
  struct damage_list *next;
  char name[512];
  int damage; // Damage #
};

struct damage_list *dmg_list = nullptr;

void free_dmg_list()
{
  struct damage_list *c, *n;
  for (c = dmg_list; c; c = n)
  {
    n = c->next;
    dc_free(c);
  }
  dmg_list = nullptr;
}

void add_dmg(Character *ch, int dmg)
{
  struct damage_list *c;

  for (c = dmg_list; c; c = c->next)
  {
    if ((IS_NPC(ch) && !str_cmp(c->name, GET_SHORT(ch))) ||
        (IS_PC(ch) && !str_cmp(c->name, GET_NAME(ch))))
    {
      c->damage += dmg;
      return;
    }
  }

#ifdef LEAK_CHECK
  c = (struct damage_list *)calloc(1, sizeof(struct damage_list));
#else
  c = (struct damage_list *)dc_alloc(1, sizeof(struct damage_list));
#endif
  if (IS_NPC(ch))
    strcpy(c->name, GET_SHORT(ch));
  else
    strcpy(c->name, GET_NAME(ch));
  c->damage = dmg;
  c->next = dmg_list;
  dmg_list = c;
}

int do_mpdamage(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  char temp[MAX_INPUT_LENGTH];
  char damroll[MAX_INPUT_LENGTH];
  char attacktype[MAX_INPUT_LENGTH];
  int32_t hitpoints = 0;

  free_dmg_list();
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    prog_error(ch, "Mpdamage - Bad syntax: Missing victim.");
    return eFAILURE | eINTERNAL_ERROR;
  }

  Character *victim = nullptr;

  // if it's 'all' leave victim nullptr and skip
  if (strcmp(arg, "all") && strcmp(arg, "allpc"))
  {
    victim = get_char_room(arg, ch->in_room);
    if (victim && (victim->getLevel() > MORTAL || IS_MOB(victim))) // don't target immortals
      victim = nullptr;

    if (!victim)
      return eFAILURE; // not an error, just couldn't get valid vict
  }

  int retval = eSUCCESS;
  while (1)
  {
    argument = one_argument(argument, damroll);
    argument = one_argument(argument, attacktype);
    argument = one_argument(argument, temp);

    if (damroll[0] == '\0')
      break;
    int numdice, sizedice;
    numdice = sizedice = 0;
    bool perc = true;
    char t, l, o;
    int plus = 0;
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
      prog_error(ch, "Mpdamage - Invalid damroll");
      return eFAILURE | eINTERNAL_ERROR;
    }

    // figure out attack type
    int damtype = determine_attack_type(attacktype);
    if (!damtype)
    {
      prog_error(ch, "Mpdamage - Invalid damtype");
      return eFAILURE | eINTERNAL_ERROR;
    }
    int dam = 0;
    // do the damage
    // half of the casts are probably pointless, but whatever

    if (victim)
    {
      int *data = nullptr;
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
        prog_error(ch, "Mpdamage - Must damage either ki,mana,hitpoints or move");
        return eFAILURE | eINTERNAL_ERROR;
      }

      if (perc)
      {
        dam = (int)((double)(*data) / 100.0 * (double)dice(numdice, sizedice));
      }
      else
      {
        dam = dice(numdice, sizedice);
      }

      if (plusPerc)
      {
        dam += (int)((double)(*data) / 100.0 * (double)plus);
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
          *data = 0;
        }
      }

      continue;
    }

    Character *next_vict;
    for (victim = DC::getInstance()->world[ch->in_room].people; victim; victim = next_vict)
    {
      next_vict = victim->next_in_room;
      if ((IS_PC(victim) && victim->getLevel() > MORTAL) || victim == ch)
      {
        continue;
      }

      if (!strcmp(arg, "allpc") && IS_NPC(victim))
      {
        continue;
      }
      int *data = nullptr;
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
        prog_error(ch, "Mpdamage - Must damage either ki,mana,hitpoints or move");
        return eFAILURE | eINTERNAL_ERROR;
      }
      if (perc)
        dam = (int)((double)(*data) / 100.0 * (double)dice(numdice, sizedice));
      else
        dam = dice(numdice, sizedice);
      if (plusPerc)
        dam += (int)((double)(*data) / 100.0 * (double)plus);
      else
        dam += plus;
      add_dmg(victim, dam);
      if (!temp[0] || !str_cmp(temp, "hitpoints"))
      {
        retval = damage(ch, victim, dam, damtype, TYPE_UNDEFINED, 0, true);
        if (isSet(retval, eCH_DIED))
        {
          return retval;
        }
      }
      else
      {
        *data -= dam;
        if (*data < 0)
          *data = 0;
      }

      if (SOMEONE_DIED(retval))
      {
        return retval;
      }
    }
  }
  return eSUCCESS;
}

int do_mpothrow(Character *ch, char *argument, int cmd)
{
  struct mprog_throw_type *throwitem = nullptr;
  int mob_num;
  int catch_num;
  int delay;

  char first[MAX_INPUT_LENGTH];
  char second[MAX_INPUT_LENGTH];
  char third[MAX_INPUT_LENGTH];
  char fourth[MAX_INPUT_LENGTH];
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
      prog_error(ch, "Mpothrow - Invalid objnum.");
      return eFAILURE;
    }
    *first = '\0';
  }
  else
  {
    if (strlen(first) >= MAX_THROW_NAME)
    {
      prog_error(ch, "Mpthrow - Name too long.");
      return eFAILURE;
    }
  }

  if (!check_range_valid_and_convert(catch_num, second, MPROG_CATCH_MIN,
                                     MPROG_CATCH_MAX))
  {
    prog_error(ch, "Mpthrow - Invalid catch_num.");
    return eFAILURE;
  }

  if (!check_range_valid_and_convert(delay, third, 0, 500))
  {
    prog_error(ch, "Mpthrow - Invalid delay.");
    return eFAILURE;
  }

  // create struct
  throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct
                                                            mprog_throw_type));
  throwitem->target_mob_num = mob_num;
  strcpy(throwitem->target_mob_name, first);
  throwitem->data_num = catch_num;
  throwitem->delay = delay;
  throwitem->mob = false;
  if (fourth[0] != '\0')
    throwitem->var = str_dup(fourth);
  else
    throwitem->var = nullptr;

  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;

  return eSUCCESS;
}

int skill_aff[] =
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

int do_mpbestow(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
      arg3[MAX_INPUT_LENGTH];
  Character *victim, *owner = nullptr;
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }
  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);

  if (arg[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0')
    return eFAILURE;
  if ((victim = get_char_room(arg, ch->in_room, true)) == nullptr && str_cmp(arg, "all") && str_cmp(arg, "allpc"))
  {
    prog_error(ch, "Mpbestow - No such person.");
    return eFAILURE | eINTERNAL_ERROR;
  }
  int i, o = 0;
  if (!check_range_valid_and_convert(i, arg2, 0, 100))
    return eFAILURE | eINTERNAL_ERROR;
  check_range_valid_and_convert(o, arg3, 0, 10000);
  int a = 0;
  if (ch->beacon)
    owner = (Character *)ch->beacon;

  if (!victim)
    victim = DC::getInstance()->world[ch->in_room].people;
  int z = 0;
  for (; victim;)
  {
    for (; affected_bits[a][0] != '\n'; a++)
    {
      if (!str_cmp(affected_bits[a], arg1))
      {
        // debugpoint();
        struct affected_type af;
        af.type = z = skill_aff[a];
        if (victim->affected_by_spell(z + BASE_TIMERS))
        {
          //		victim->sendln("A s.");
          return eFAILURE;
        }
        af.duration = i;
        af.bitvector = a + 1;
        af.location = 0;
        af.modifier = 987; // Notifies that it's timered.
        affect_join(victim, &af, true, false);
        if (z && o) // Timer on it
        {
          af.type = BASE_TIMERS + z;
          af.duration = o;
          af.bitvector = -1;
          af.location = 0;
          af.modifier = 0;
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
        if (IS_PC(victim))
          break;
      }
    }
    else
      break;
  }
  if (z && o && owner) // Timer on it
  {
    struct affected_type af;
    af.type = BASE_TIMERS + z;
    af.duration = o;
    af.bitvector = -1;
    af.location = 0;
    af.modifier = 0;
    affect_join(owner, &af, true, false);
  }
  return eSUCCESS;
}

// simulate a pause in proc execution
// stops prog, mpthrow a special kinda throw, picks it up again when delay is over
int do_mppause(Character *ch, char *argument, int cmd)
{
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  struct mprog_throw_type *throwitem = nullptr;
  int delay;

  char first[MAX_INPUT_LENGTH];
  char second[MAX_INPUT_LENGTH];

  argument = one_argument(argument, first);
  if (std::string(first) == "all")
  {
    argument = one_argument(argument, second);

    if (!check_range_valid_and_convert(delay, second, 0, 65536))
    {
      prog_error(ch, "mpppause all - Invalid delay.");
      return eFAILURE;
    }
    throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct mprog_throw_type));

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
      prog_error(ch, "mppause - Invalid delay.");
      return eFAILURE;
    }
    throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct mprog_throw_type));
    throwitem->data_num = -999;
  }

  if (IS_MOB(ch))
  {
    throwitem->target_mob_num = mob_index[ch->mobdata->nr].virt;
    throwitem->mob = true; // This is, suprisingly, a mob
  }
  else
  {
    throwitem->target_mob_num = 0;
    throwitem->mob = false;
  }

  throwitem->target_mob_name[0] = '\0';
  throwitem->tMob = ch;
  throwitem->delay = delay;

  extern Character *activeActor;
  extern Character *activeRndm;
  extern Object *activeObj;
  extern void *activeVo;
  throwitem->actor = activeActor;
  throwitem->obj = activeObj;
  throwitem->vo = activeVo;
  throwitem->rndm = activeRndm;

  extern char *activeProg;
  extern char *activePos;
  extern char *activeProgTmpBuf;
  throwitem->orig = str_dup(activeProg);

  extern int cIfs[256];
  throwitem->cPos = 0;
  memcpy(&throwitem->ifchecks[0], &cIfs[0], sizeof(int) * 256);

  throwitem->startPos = activePos - activeProgTmpBuf;

  throwitem->var = nullptr;
  throwitem->opt = 0;
  // add to delay list
  throwitem->next = g_mprog_throw_list;
  g_mprog_throw_list = throwitem;
  return eSUCCESS | eDELAYED_EXEC;
}

int do_mpteleport(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char person[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  room_t to_room = 0;

  if (IS_PC(ch) && ch->getLevel() < 110)
    return eFAILURE;

  half_chop(argument, person, type);

  if (!*person)
  {
    strncpy(person, GET_NAME(ch), MAX_INPUT_LENGTH);
  }

  if (!(victim = get_char_vis(ch, person)))
  {
    if (!strcmp(person, "area"))
    {
      victim = ch;
    }
    else
    {
      ch->sendln("No-one by that name around.");
      return eFAILURE;
    }
  }

  if (isSet(DC::getInstance()->world[victim->in_room].room_flags, TELEPORT_BLOCK) ||
      IS_AFFECTED(victim, AFF_SOLIDITY))
  {
    ch->sendln("You find yourself unable to.");
    if (ch != victim)
    {
      snprintf(buf, MAX_INPUT_LENGTH, "%s just tried to teleport you.\r\n", GET_SHORT(ch));
      victim->send(buf);
    }
    return eFAILURE;
  }

  int i = DC::getInstance()->world[ch->in_room].zone,
      low = DC::getInstance()->zones.value(i).getRealBottom(),
      high = DC::getInstance()->zones.value(i).getRealTop(),
      attempts = 0;

  do
  {
    if ((*type && !strcmp(type, "area")) ||
        (victim == ch && *person && !strcmp(person, "area")))
    {
      to_room = number(low, high);

      // Check to see if we're in an endless loop
      if (attempts++ > high - low)
      {
        return eFAILURE;
      }
    }
    else
    {
      to_room = number<room_t>(1, top_of_world);
    }
  } while (!DC::getInstance()->rooms.contains(to_room) ||
           isSet(DC::getInstance()->world[to_room].room_flags, PRIVATE) ||
           isSet(DC::getInstance()->world[to_room].room_flags, IMP_ONLY) ||
           isSet(DC::getInstance()->world[to_room].room_flags, NO_TELEPORT) ||
           isSet(DC::getInstance()->world[to_room].room_flags, ARENA) ||
           (DC::getInstance()->world[to_room].sector_type == SECT_UNDERWATER && GET_RACE(victim) != RACE_FISH) ||
           DC::getInstance()->zones.value(DC::getInstance()->world[to_room].zone).isNoTeleport() ||
           ((IS_NPC(victim) && ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN)) ? (DC::getInstance()->zones.value(DC::getInstance()->world[to_room].zone).isTown()) : false) ||
           (IS_AFFECTED(victim, AFF_CHAMPION) && (isSet(DC::getInstance()->world[to_room].room_flags, CLAN_ROOM) ||
                                                  (to_room >= 1900 && to_room <= 1999))));

  act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
  move_char(victim, to_room);
  act("$n slowly fades into existence.", victim, 0, 0, TO_ROOM, 0);

  do_look(victim, "", 0);
  return eSUCCESS;
}

int do_mppeace(Character *ch, char *argument, int cmd)
{
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eSUCCESS;
  }
  char arg[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg);

  Character *rch, *vict = nullptr;

  if (arg[0])
  {
    vict = get_char_room(arg, ch->in_room);
    if (!vict)
    {
      prog_error(ch, "Mppeace - Vict not found.");
      return eFAILURE;
    }
    if (IS_MOB(vict) && vict->mobdata->hated != nullptr)
      remove_memory(vict, 'h');
    if (vict->fighting != nullptr)
      stop_fighting(vict);
    return eSUCCESS;
  }
  for (rch = DC::getInstance()->world[ch->in_room].people; rch != nullptr; rch = rch->next_in_room)
  {
    if (IS_MOB(rch) && rch->mobdata->hated != nullptr)
      remove_memory(rch, 'h');
    if (rch->fighting != nullptr)
      stop_fighting(rch);
  }
  return eSUCCESS;
}

int do_mpretval(Character *ch, char *argument, int cmd)
{
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  char *retvals[] =
      {
          "true",
          "false",
          "\n"};
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);
  int retval = eSUCCESS;

  for (int i = 0; retvals[i][0] != '\n'; i++)
    if (!str_cmp(arg, retvals[i]))
      SET_BIT(retval, 1 << (5 + i));

  return retval;
}

// Yes, I'm sure there's a better way to do this. Stop whining.
int process_math(Character *ch, char *string)
{
  int result = 0, curr = 0;
  char lastsign = '\0';
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
      curr = 0;
    }
    if (*string == '\0' || *string == '\r' || *string == '\n')
      break;
    switch (*string)
    {
    case '+':
    case '-':
    case '/':
    case '*':
      curr = 0;
      lastsign = *string;
      break;
    default:
      prog_error(ch, "Mpsetmath - Unknown symbol: '%c'.", *string);
      return result;
      break;
    };
    string++;
  }
  return result;
}

// Unreadable code > you
char *expand_data(Character *ch, char *orig)
{
  static char buf[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH];
  while (*orig == ' ')
    orig++;
  strcpy(buf1, orig);
  orig = &buf1[0];
  buf[0] = '\0';
  char *ptr;
  int i = 0, l, r, z = 0, o;
  char c;
  while (1)
  {
    char left[MAX_INPUT_LENGTH];
    char right[MAX_INPUT_LENGTH];
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
    if (!c == '\0')
      *(ptr + r) = '~';

    int16_t *lvali = nullptr;
    uint32_t *lvalui = nullptr;
    char **lvalstr = nullptr;
    QString lvalqstr;
    int64_t *lvali64 = nullptr;
    uint64_t *lvalui64 = nullptr;
    int8_t *lvalb = nullptr;
    translate_value(left, right, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, ch, activeActor, activeObj, activeVo, activeRndm, lvalqstr);

    if (!lvali && !lvalui && !lvalb)
    {
      prog_error(ch, "Mpsetmath - Expand_data - Accessing unknown data.");
      return nullptr;
    }

    for (o = z;; o++)
    {
      if (*(orig + o) == '~' || *(orig + o) == '\0')
        break;
      buf[i++] = *(orig + o);
    }
    buf[i] = '\0';
    char tmp[MAX_INPUT_LENGTH];
    if (lvali)
      sprintf(tmp, "%d", *lvali);
    if (lvalui)
      sprintf(tmp, "%u", *lvalui);
    if (lvalb)
      sprintf(tmp, "%d", *lvalb);
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

char *allowedData[] = {
    "hitpoints", "mana", "move", nullptr};

int do_mpsetmath(Character *ch, char *arg, int cmd)
{
  if (IS_PC(ch))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  Character *vict;
  //  if (activeActor) activeActor->send(QString("{%1}\r\n").arg(arg));
  //  vict = get_pc("Urizen");
  //  if (vict) vict->send(QString("{%1}\r\n").arg(arg));

  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  arg = one_argument(arg, arg1);
  char *r = nullptr;

  if ((r = strchr(arg1, '.')) != nullptr)
  {
    *r = '\0';
    r++;
    one_argument(r, arg2);
  }
  if (!r || !*r || !*arg1)
  {
    prog_error(ch, "Mpsetmath - Invalid primary data.");
    return eFAILURE;
  }
  bool allowed = false;

  int16_t *lvali = nullptr;
  uint32_t *lvalui = nullptr;
  char **lvalstr = nullptr;
  QString lvalqstr;
  int64_t *lvali64 = nullptr;
  uint64_t *lvalui64 = nullptr;
  int8_t *lvalb = nullptr;
  translate_value(arg1, r, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, ch, activeActor, activeObj, activeVo, activeRndm, lvalqstr);

  vict = activeTarget;
  if (!vict)
  {
    //	    prog_error(ch, "Mpsetmath - No target.");
    //	    return eFAILURE;
  }
  if (vict && IS_NPC(vict))
    allowed = true;
  else if (vict)
    for (int i = 0; allowedData[i]; i++)
      if (!str_cmp(allowedData[i], r))
        allowed = true;

  if (!allowed && vict)
  {
    prog_error(ch, "Mpsetmath - Accessing unallowed data.");
    return eFAILURE;
  }

  if (lvalstr)
  {
    char nw[MAX_INPUT_LENGTH];
    arg = one_argument_long(arg, nw);
    if (!nw[0])
    {
      prog_error(ch, "Mpsetmath - Invalid string.");
      return eFAILURE;
    }
    *lvalstr = str_dup(nw);
    return eSUCCESS;
  }
  if (!lvalui && !lvali && !lvalb)
  {
    prog_error(ch, "Mpsetmath - Accessing unknown data.");
    return eFAILURE;
  }

  char *fixed = expand_data(ch, arg);

  int i = process_math(ch, fixed);

  if (i == -9839)
  {
    prog_error(ch, "Mpsetmath - Invalid math-string.");
    return eFAILURE;
  }
  if (lvali)
  {
    *lvali = i;
    //    prog_error(ch, "Mpsetmath - %s set to %d.");
    //  	r, i, mob_index[ch->mobdata->nr].virt );
  }
  if (lvalb)
  {
    *lvalb = (int8_t)i;
    //    prog_error(ch, "Mpsetmath - %s set to %d.");
    //  	r, i, mob_index[ch->mobdata->nr].virt );
  }
  if (lvalui)
  {
    *lvalui = (unsigned int)i;
    //    prog_error(ch, "Mpsetmath - %s set to %d.");
    //  	r, i, mob_index[ch->mobdata->nr].virt );
  }

  /*  csendf(vict, "%d\r\n%d\r\n%d\r\n%d\r\n",
      process_math(ch, "1+1-3+5*10"),
      process_math(ch, "1-3+5*10/2"),
      process_math(ch, "1+101-34+5*10*2/8"),
      process_math(ch, "1+101-34+5*10*2/8")
      );
    */
  return eSUCCESS;
}

void prog_error(Character *ch, char *format, ...)
{
  va_list ap;
  char buffer[MAX_STRING_LENGTH];

  va_start(ap, format);
  vsnprintf(buffer, MAX_STRING_LENGTH, format, ap);
  va_end(ap);

  if (ch && IS_OBJ(ch))
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Obj %d, com %d, line %d: %s",
         obj_index[ch->objdata->item_number].virt, mprog_command_num,
         mprog_line_num, buffer);
  }
  else if (ch && IS_MOB(ch))
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Mob %d, com %d, line %d: %s",
         mob_index[ch->mobdata->nr].virt, mprog_command_num, mprog_line_num,
         buffer);
  }
  else
  {
    logf(IMMORTAL, LogChannels::LOG_WORLD, "Unknown prog: %s", buffer);
  }
}
