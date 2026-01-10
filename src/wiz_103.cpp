/********************************
| Level 103 wizard commands
| 11/20/95 -- Azrack
**********************/

#include <fmt/format.h>

#include "DC/wizard.h"
#include "DC/utility.h"
#include "DC/mobile.h"
#include "DC/player.h"
#include "DC/db.h"
#include "DC/connect.h"
#include "DC/interp.h"
#include "DC/room.h"
#include "DC/handler.h"
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/clan.h"
#include "DC/race.h"
#include "DC/const.h"

int do_boot(Character *ch, char *arg, cmd_t cmd)
{
  Character *victim;
  char name[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], buf[500];

  half_chop(arg, name, type);

  if (!(*name))
  {
    ch->sendln("Syntax: boot <victim> [boot]");
    ch->sendln("The boot option causes the victim to see a large ASCII boot.");
    return eFAILURE;
  }

  victim = get_pc_vis(ch, name);

  if (victim)
  {
    if (IS_PC(victim) && (ch->getLevel() <= victim->getLevel()))
    {
      act("You cast a stream of fire at $N.", ch, 0, victim, TO_CHAR, 0);
      act("$n casts a stream of fire at you.", ch, 0, victim, TO_VICT, 0);
      act("$n casts a stream of fire at $N.", ch, 0, victim, TO_ROOM, NOTVICT);
      return eFAILURE;
    }
    if (victim->isPlayer() && victim->player->possesing)
    {
      ch->send("Oops! They ain't linkdead! Just possessing.");
      return eFAILURE;
    }
    if (IS_AFFECTED(victim, AFF_CANTQUIT))
    {
      if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
      {
        act("$N is a thief.  Don't boot $M.", ch, 0, victim, TO_CHAR, 0);
        return eFAILURE;
      }
      act("$N is a pkiller.  Don't boot $M.", ch, 0, victim, TO_CHAR, 0);
      return eFAILURE;
    }

    /* Still here? Ok, the boot continues */
    victim->sendln("You have been disconnected.");
    ch->sendln("Ok.");
    if (IS_PC(victim))
    {
      sprintf(buf, "A stream of fire arcs down from the heavens, striking "
                   "you between the eyes.\r\nYou have been removed from the "
                   "world by %s.\r\n",
              GET_SHORT(ch));
      victim->send(buf);
    }

    act("A stream of fire arcs down from the heavens, striking "
        "$n between the eyes.\r\n$n has been removed from the world "
        "by $N.",
        victim, 0, ch, TO_ROOM, INVIS_NULL);

    sprintf(name, "%s has booted %s.", GET_NAME(ch), qPrintable(victim->getName()));
    DC::getInstance()->logentry(name, ch->getLevel(), DC::LogChannel::LOG_GOD);

    if (!strcmp(type, "boot"))
    {
      send_to_char(
          "\r\n"
          "                       $1/               /\r\n"
          "                      $1/               /\r\n"
          "                     $1/               /\r\n"
          "                    $1/               /$R\r\n"
          "                   $5/\\_             $1/$R\r\n"
          "      $5____        /   \\_          $1/$R\r\n"
          "     $5/    \\      /\\     \\_       $1/$R\r\n"
          "    $5/      \\    /\\        \\_    $1/$R\r\n"
          "   $5/        \\  /\\           \\_ $1/$R\r\n"
          "  $5/          \\/\\              /\r\n"
          "  |\\                         /\r\n"
          "   \\\\                       /\r\n"
          "    \\\\                     /\r\n"
          "     \\\\                   /\r\n"
          "      \\\\                 /\r\n"
          "       \\\\               /\r\n"
          "        \\\\             /\r\n"
          "         \\\\           /\r\n"
          "          \\\\         /\r\n"
          "           \\\\       /\r\n"
          "            \\\\     /\r\n"
          "             \\\\   /\r\n"
          "              \\__/$R\r\n",
          victim);
    }
    if (!strcmp(type, "kitty"))
    {
      send_to_char("\r\n"
                   "                __                             $6$B$I___$R            $6$B$I_yygL$R\r\n"
                   "$B$6i wuv u!$R       #####gy_,                    $6$B$Iy#######g   __g########g$R\r\n"
                   "    \\        g#F   `M##bg.                $6$B$Ig#\"'    ###g####~'    9##L$R\r\n"
                   "             ##F       `###g____yyyyy_____$6$B$Ij#\"        ###          ##E$R\r\n"
                   "            a#F           3##\"#~~~~~~~###$6$B$I##\"          ##g          ##$R\r\n"
                   "           j#F                           $6$B$I5#      ____ _##y__       ##1$R\r\n"
                   "           a#                           $6$B$Iy##    _g##~####\"#M##g     ##1$R\r\n"
                   "           #E                           $6$B$IJ#L    ##$R  $6$B$Ig#\"'     `#########g_$R\r\n"
                   "          o#1                           $6$B$I##     ##$R $6$B$Iy#E         ##L     9#,$R\r\n"
                   "           #g                           $6$B$I##      ####F         3#g      ##$R\r\n"
                   "          a#F                           $6$B$I3#L       ##L         ##M#.    ##!$R\r\n"
                   "         g#F                             $6$B$I##_     _##g       _g#F$R $6$B$I#g   y##$R\r\n"
                   "        _#F                               $6$B$I~###g###~M##g_   y###yg#'  y##'$R\r\n"
                   "        ##                                           $6$B$I`?\"M###        g##F$R\r\n"
                   "       ##'                                                $6$B$I?#,      ###L$R\r\n"
                   "       #E                                                  $6$B$I##g___g#$R\"###\r\n"
                   "      J#F                                                    $6$B$I`M##'$R   ##L\r\n"
                   "      ##                                                              ##\r\n"
                   "      ##                                                              ##\r\n"
                   "      ##                                                              ##L\r\n"
                   "   ___##y_.      a#o                                                __##1\r\n"
                   "##\"\"F~5#F        ###L                                 __          #M#M###M##\r\n"
                   "      J#K        ###L                                g##g             ##\r\n"
                   "     _y##ga       ~           _amog                  ####            ##F\r\n"
                   " a###~'\"#1                   d#   \"#                 \"##          #wy##L.\r\n"
                   "        3#,                   #g__g\"                                ##\"\"5##g\r\n"
                   "         ##g#                    ''                                a##    '~\r\n"
                   "    __y#\"FH#_                                                  y_ g##\r\n"
                   "   ##\"'     ##g                                                 \"###g_\r\n"
                   "   ~         `9#g_                                            _g##'\"9##gg,\r\n"
                   "                 ?##gy_.                                   _y##\"'      `##\r\n"
                   "                     ~\"####ggy_____                  ___g###F'\r\n"
                   "                             \"~~~~~##################~~~\"\r\n",
                   victim);
    }
    move_char(victim, real_room(START_ROOM));
    do_quit(victim, "", cmd_t::SAVE_SILENTLY);
  }

  else
    ch->sendln("Boot Who?");

  return eSUCCESS;
}

int do_disconnect(Character *ch, char *argument, cmd_t cmd)
{
  char arg[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  class Connection *d;
  unsigned sdesc;

  if (IS_NPC(ch))
    return eFAILURE;

  one_argument(argument, arg);
  sdesc = atoi(arg);
  if (arg == 0)
  {
    ch->sendln("Illegal descriptor number.");
    ch->sendln("Usage: release <#>");
    return eFAILURE;
  }
  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    if (d->descriptor == sdesc)
    {
      if (d->character && (d->character->getLevel() > ch->getLevel()))
      {
        sprintf(buf, "Heh, %s tried to disconnect you. He has paid.\r\n", GET_NAME(ch));
        d->character->send(buf);
        ch->sendln("You dummy, can't do that to your elders!");
        close_socket(ch->desc);
        return eFAILURE;
      }
      else
      {
        close_socket(d);
        sprintf(buf, "Closing socket to descriptor #%d\r\n", sdesc);
        ch->send(buf);
        return eFAILURE;
      }
    }
  }
  ch->sendln("Descriptor not found!");
  return eSUCCESS;
}

int do_fsave(Character *ch, std::string argument, cmd_t cmd)
{
  Character *vict = {};
  std::string name = {}, buf = {};

  if (IS_NPC(ch))
  {
    return eFAILURE;
  }

  std::tie(name, argument) = half_chop(argument);
  if (name.empty())
  {
    ch->sendln("Who do you wish to force to save?");
    return eFAILURE;
  }

  if (!(vict = get_char_vis(ch, name)))
  {
    ch->sendln("No-one by that name here..");
    return eFAILURE;
  }

  if (ch->player->stealth == false)
  {
    buf = "$n has forced you to 'save'.";
    act(buf, ch, 0, vict, TO_VICT, 0);
    buf = {};
    ch->sendln("Ok.");
  }
  vict->save();

  DC::getInstance()->logentry(QStringLiteral("%1 just forced %2 to save.").arg(GET_NAME(ch)).arg(GET_NAME(vict)), ch->getLevel(), DC::LogChannel::LOG_GOD);

  return eSUCCESS;
}

int do_fighting(Character *ch, char *argument, cmd_t cmd)
{
  const int CLANTAG_LEN = MAX_CLAN_LEN + 3; // "[Foobar]"
  Character *i;
  bool arenaONLY = false;
  int countFighters = 0;
  char buf[80];
  char arg[MAX_STRING_LENGTH];
  clan_data *ch_clan = 0;
  char ch_clan_name[CLANTAG_LEN];
  ch_clan_name[0] = 0;
  clan_data *victim_clan = 0;
  char victim_clan_name[CLANTAG_LEN];
  victim_clan_name[0] = 0;

  one_argument(argument, arg);
  if (!strcmp(arg, "arena"))
    arenaONLY = true;

  for (i = combat_list; i; i = i->next_fighting)
  {
    // Don't show mobs fighting or people not in the arena when arena
    // keyword was specified.
    if (IS_NPC(i) || (arenaONLY && !i->room().isArena()))
    {
      continue;
    }
    else
    {
      countFighters++;
    }

    // If they're in a clan
    ch_clan_name[0] = '\0';
    if ((ch_clan = get_clan(i)))
      snprintf(ch_clan_name, CLANTAG_LEN, "[%s]", ch_clan->name);

    if ((victim_clan = get_clan(i->fighting)))
      snprintf(victim_clan_name, CLANTAG_LEN, "[%s]", victim_clan->name);

    snprintf(buf, 80, "%s %s fighting %s %s (%d)\r\n",
             GET_NAME(i), ch_clan_name,
             GET_NAME(i->fighting), victim_clan_name,
             DC::getInstance()->world[i->in_room].number);
    ch->send(buf);
  }

  if (countFighters == 0)
  {
    if (arenaONLY)
      ch->sendln("No fighting characters found in the arena.");
    else
      ch->sendln("No fighting characters found.");
  }
  return eSUCCESS;
}

int do_peace(Character *ch, char *argument, cmd_t cmd)
{
  Character *rch;

  for (rch = DC::getInstance()->world[ch->in_room].people; rch != nullptr; rch = rch->next_in_room)
  {
    if (IS_NPC(rch) && rch->mobdata->hated != nullptr)
      remove_memory(rch, 'h');
    if (rch->fighting != nullptr)
      stop_fighting(rch);
  }
  act("$n makes a gesture and all fighting stops.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You stop all fighting in this room.");
  return eSUCCESS;
}

int do_matrixinfo(Character *ch, char *argument, cmd_t cmd)
{
  char buf[MAX_STRING_LENGTH];
  int i = 0;
  buf[0] = '\0';
  for (; i < MAX_RACE; i++)
  {
    char immbuf[MAX_STRING_LENGTH], resbuf[MAX_STRING_LENGTH], susbuf[MAX_STRING_LENGTH];
    immbuf[0] = resbuf[0] = susbuf[0] = '\0';
    sprintbit(races[i].immune, isr_bits, immbuf);
    sprintbit(races[i].resist, isr_bits, resbuf);
    sprintbit(races[i].suscept, isr_bits, susbuf);

    char hatbuf[MAX_STRING_LENGTH], fribuf[MAX_STRING_LENGTH];
    hatbuf[0] = fribuf[0] = '\0';
    sprintbit(races[i].hate_fear << 1, race_abbrev, hatbuf);
    sprintbit(races[i].friendly << 1, race_abbrev, fribuf);

    sprintf(buf, "%s %s - Imm: %s Res: %s Sus: %s\r\n    Hates: %s Friend: %s\r\n",
            buf, races[i].plural_name, immbuf, resbuf, susbuf, hatbuf, fribuf);
  }
  ch->send(buf);
  return eSUCCESS;
}

int lookupClass(Character *ch, char *str)
{
  int c_class;

  if (str != 0)
  {
    str[0] = toupper(str[0]);
    for (c_class = 1; c_class <= CLASS_MAX; c_class++)
    {
      if (is_abbrev(str, pc_clss_types[c_class]))
      {
        return c_class;
      }
    }
  }

  if (ch != 0)
  {
    ch->sendln("Invalid class.\r\n");
    ch->sendln("Valid classes:");
    for (c_class = 1; c_class <= CLASS_MAX; c_class++)
    {
      csendf(ch, "%s\r\n", pc_clss_types[c_class]);
    }
  }

  return -1;
}

int lookupRoom(Character *ch, char *str)
{
  if (str == 0)
    return -1;

  int room = atoi(str);

  if (room == DC::NOWHERE || room > DC::getInstance()->top_of_world || !DC::getInstance()->rooms.contains(room))
  {
    if (ch)
    {
      ch->sendln("No such room exists.");
    }

    return -1;
  }

  return room;
}

int do_guild(Character *ch, char *argument, cmd_t cmd)
{
  int c_class = 0, room = 0, old_room = 0;
  char arg1[MAX_STRING_LENGTH] = {0};
  char arg2[MAX_STRING_LENGTH] = {0};

  if (IS_NPC(ch))
    return eFAILURE;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  // No arguments
  if (arg1[0] == 0)
  {
    ch->sendln("Syntax:");
    ch->sendln("guild <room #>           - List all classes allowed in room");
    ch->sendln("guild <class>            - List all rooms that allow that class");
    ch->sendln("guild <class> <room #>   - Toggle allow/deny class in room\r\n");
    return eFAILURE;
  }

  // guild <room #> or guild <class>
  if (arg2[0] == 0)
  {
    if (is_number(arg1))
    {
      // guild <room #>
      room = lookupRoom(ch, arg1);
      if (room == DC::NOWHERE)
      {
        return eFAILURE;
      }

      ch->send(QStringLiteral("Allow list for room #%1: ").arg(room));
      bool found = false;
      for (c_class = 1; c_class < CLASS_MAX; c_class++)
      {
        if (DC::getInstance()->rooms.contains(room) && DC::getInstance()->rooms[room].allow_class[c_class] == true)
        {
          found = true;
          csendf(ch, "%s ", pc_clss_types[c_class]);
        }
      }

      if (found)
      {
        ch->sendln("");
      }
      else
      {
        ch->sendln("All");
      }

      return eSUCCESS;
    }
    else
    {
      // guild <class>
      c_class = lookupClass(ch, arg1);
      if (c_class == -1)
      {
        return eFAILURE;
      }

      int count = 0;
      csendf(ch, "%s only rooms:\r\n", pc_clss_types[c_class]);

      int cols = 0;
      for (int r = 0; r < DC::getInstance()->top_of_world; r++)
      {
        if (DC::getInstance()->rooms.contains(r) && DC::getInstance()->rooms[r].allow_class[c_class] == true)
        {
          ch->send(QStringLiteral("%1 ").arg(r, 5));

          count++;
          cols++;
          if (cols == 11)
          {
            cols = 0;
            ch->sendln("");
          }
        }
      }

      if (count == 0)
      {
        ch->sendln("None found.");
      }
      else
      {
        ch->sendln("");
      }

      return eSUCCESS;
    }
  }

  // guild <class> <room #>
  c_class = lookupClass(ch, arg1);
  if (c_class == -1)
  {
    return eFAILURE;
  }

  room = lookupRoom(ch, arg2);
  if (room == DC::NOWHERE)
  {
    return eFAILURE;
  }

  if (!can_modify_room(ch, room))
  {
    ch->sendln("You are unable to work creation outside of your range.");
    return eFAILURE;
  }

  if (!DC::getInstance()->rooms.contains(room))
  {
    ch->send(QStringLiteral("Room %1 does not exist.\r\n").arg(room));
    return eFAILURE;
  }

  if (DC::getInstance()->rooms[room].allow_class[c_class] == true)
  {
    csendf(ch, "Removed %s class from room #%d's allow list.\r\n", pc_clss_types[c_class], room);
    DC::getInstance()->rooms[room].allow_class[c_class] = false;
  }
  else
  {
    csendf(ch, "Added %s class to room #%d's allow list.\r\n", pc_clss_types[c_class], room);
    DC::getInstance()->rooms[room].allow_class[c_class] = true;
  }

  DC::getInstance()->set_zone_modified_world(room);

  old_room = ch->in_room;
  ch->in_room = room;
  do_rsave(ch, "");
  ch->in_room = old_room;

  return eSUCCESS;
}
