/********************************
| Level 103 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/DC.h"
#include "DC/interp.h"
#include "DC/clan.h"
#include "DC/race.h"
#include "DC/const.h"

#include <fmt/format.h>

command_return_t do_boot(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr victim;
  QString name, type, buf[500];

  half_chop(arg, name, type);

  if (!(*name))
  {
    ch->sendln("Syntax: boot <victim> [boot]");
    ch->sendln("The boot option causes the victim to see a large ASCII boot.");
    return ReturnValue::eFAILURE;
  }

  victim = get_pc_vis(ch, name);

  if (victim)
  {
    if (victim->isPlayer() && (ch->getLevel() <= victim->getLevel()))
    {
      act_to_character("You cast a stream of fire at $N.", ch, 0, victim, 0);
      act_to_victim("$n casts a stream of fire at you.", ch, 0, victim, 0);
      act_to_room("$n casts a stream of fire at $N.", ch, 0, victim, NOTVICT);
      return ReturnValue::eFAILURE;
    }
    if (!victim->isNonPlayer() && victim->player->possesing)
    {
      ch->send("Oops! They ain't linkdead! Just possessing.");
      return ReturnValue::eFAILURE;
    }
    if (IS_AFFECTED(victim, AFF_CANTQUIT))
    {
      if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
      {
        act_to_character("$N is a thief.  Don't boot $M.", ch, 0, victim, 0);
        return ReturnValue::eFAILURE;
      }
      act_to_character("$N is a pkiller.  Don't boot $M.", ch, 0, victim, 0);
      return ReturnValue::eFAILURE;
    }

    /* Still here? Ok, the boot continues */
    victim->sendln("You have been disconnected.");
    ch->sendln("Ok.");
    if (victim->isPlayer())
    {
      dc_sprintf(buf, "A stream of fire arcs down from the heavens, striking "
                      "you between the eyes.\r\nYou have been removed from the "
                      "world by %s.\r\n",
                 qPrintable(ch->shortdesc_or_name()));
      victim->send(buf);
    }

    act("A stream of fire arcs down from the heavens, striking "
        "$n between the eyes.\r\n$n has been removed from the world "
        "by $N.",
        victim, 0, ch, TO_ROOM, INVIS_NULL);

    dc_sprintf(name, "%s has booted %s.", qPrintable(ch->name()), qPrintable(victim->name()));
    dc_->logentry(name, ch->getLevel(), DC::LogChannel::LOG_GOD);

    if (type == u"boot"_s)
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
    if (type == u"kitty"_s)
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

  return ReturnValue::eSUCCESS;
}

command_return_t do_disconnect(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  QString buf;
  ConnectionPtr d;
  quint32 sdesc;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  one_argument(argument, arg);
  sdesc = atoi(arg);
  if (arg == 0)
  {
    ch->sendln("Illegal descriptor number.");
    ch->sendln("Usage: release <#>");
    return ReturnValue::eFAILURE;
  }
  for (auto &d : dc_->connections_)
  {
    if (conn->descriptor == sdesc)
    {
      if (conn->character && (conn->character->getLevel() > ch->getLevel()))
      {
        dc_sprintf(buf, "Heh, %s tried to disconnect you. He has paid.\r\n", qPrintable(ch->name()));
        conn->character->send(buf);
        ch->sendln("You dummy, can't do that to your elders!");
        close_socket(ch->desc);
        return ReturnValue::eFAILURE;
      }
      else
      {
        close_socket(d);
        dc_sprintf(buf, "Closing socket to descriptor #%d\r\n", sdesc);
        ch->send(buf);
        return ReturnValue::eFAILURE;
      }
    }
  }
  ch->sendln("Descriptor not found!");
  return ReturnValue::eSUCCESS;
}

command_return_t do_fsave(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict = {};
  QString name = {}, buf = {};

  if (ch->isNonPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  std::tie(name, argument) = half_chop(argument);
  if (name.isEmpty())
  {
    ch->sendln("Who do you wish to force to save?");
    return ReturnValue::eFAILURE;
  }

  if (!(vict = get_char_vis(ch, name)))
  {
    ch->sendln("No-one by that name here..");
    return ReturnValue::eFAILURE;
  }

  if (ch->player->stealth == false)
  {
    buf = "$n has forced you to 'save'.";
    act_to_victim(buf, ch, 0, vict, 0);
    buf = {};
    ch->sendln("Ok.");
  }
  vict->save();

  dc_->logentry(u"%1 just forced %2 to save."_s.arg(qPrintable(ch->name())).arg(qPrintable(vict->name())), ch->getLevel(), DC::LogChannel::LOG_GOD);

  return ReturnValue::eSUCCESS;
}

command_return_t do_fighting(CharacterPtr ch, QString argument, cmd_t cmd)
{
  const qint32 CLANTAG_LEN = MAX_CLAN_LEN + 3; // "[Foobar]"
  CharacterPtr i;
  bool arenaONLY = false;
  qint32 countFighters = {};
  QString buf;
  QString arg;
  ClanPtr ch_clan = {};
  QString ch_clan_name;
  ch_clan_name[0] = {};
  ClanPtr victim_clan = {};
  QString victim_clan_name;
  victim_clan_name[0] = {};

  one_argument(argument, arg);
  if (arg == u"arena"_s)
    arenaONLY = true;

  for (i = combat_list; i; i = i->next_fighting)
  {
    // Don't show mobs fighting or people not in the arena when arena
    // keyword was specified.
    if (i->isNonPlayer() || (arenaONLY && !i->room().isArena()))
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
      dc_snprintf(ch_clan_name, CLANTAG_LEN, "[%s]", ch_clan->name);

    if ((victim_clan = get_clan(i->fighting)))
      dc_snprintf(victim_clan_name, CLANTAG_LEN, "[%s]", victim_clan->name);

    dc_snprintf(buf, 80, "%s %s fighting %s %s (%d)\r\n",
                qPrintable(i->name()), ch_clan_name,
                qPrintable(i->fighting->name()), victim_clan_name,
                dc_->world[i->in_room].number);
    ch->send(buf);
  }

  if (countFighters == 0)
  {
    if (arenaONLY)
      ch->sendln("No fighting characters found in the arena.");
    else
      ch->sendln("No fighting characters found.");
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_peace(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr rch;

  for (rch = dc_->world[ch->in_room].people_; rch != nullptr; rch = rch->next_in_room)
  {
    if (rch->isNonPlayer() && rch->mobdata->hated != nullptr)
      remove_memory(rch, 'h');
    if (rch->fighting != nullptr)
      stop_fighting(rch);
  }
  act_to_room("$n makes a gesture and all fighting stops.", ch, 0, 0, 0);
  ch->sendln("You stop all fighting in this room.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_matrixinfo(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  qint32 i = {};
  buf[0] = '\0';
  for (; i < MAX_RACE; i++)
  {
    QString immbuf, resbuf, susbuf;
    immbuf[0] = resbuf[0] = susbuf[0] = '\0';
    sprintbit(races[i].immune, isr_bits, immbuf);
    sprintbit(races[i].resist, isr_bits, resbuf);
    sprintbit(races[i].suscept, isr_bits, susbuf);

    QString hatbuf, fribuf;
    hatbuf[0] = fribuf[0] = '\0';
    sprintbit(races[i].hate_fear << 1, race_abbrev, hatbuf);
    sprintbit(races[i].friendly << 1, race_abbrev, fribuf);

    dc_sprintf(buf, "%s %s - Imm: %s Res: %s Sus: %s\r\n    Hates: %s Friend: %s\r\n",
               buf, races[i].plural_name, immbuf, resbuf, susbuf, hatbuf, fribuf);
  }
  ch->send(buf);
  return ReturnValue::eSUCCESS;
}

qint32 lookupClass(CharacterPtr ch, QString str)
{
  qint32 c_class;

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
      ch->send(u"%s\r\n"_s.arg(pc_clss_types[c_class]));
    }
  }

  return -1;
}

qint32 lookupRoom(CharacterPtr ch, QString str)
{
  if (str == 0)
    return -1;

  qint32 room = atoi(str);

  if (room == DC::NOWHERE || room > dc_->top_of_world || !dc_->rooms.contains(room))
  {
    if (ch)
    {
      ch->sendln("No such room exists.");
    }

    return -1;
  }

  return room;
}

command_return_t do_guild(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 c_class = 0, room = 0, old_room = {};
  QString arg1;
  QString arg2;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  // No arguments
  if (arg1[0] == 0)
  {
    ch->sendln("Syntax:");
    ch->sendln("guild <room #>           - List all classes allowed in room");
    ch->sendln("guild <class>            - List all rooms that allow that class");
    ch->sendln("guild <class> <room #>   - Toggle allow/deny class in room\r\n");
    return ReturnValue::eFAILURE;
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
        return ReturnValue::eFAILURE;
      }

      ch->send(u"Allow list for room #%1: "_s.arg(room));
      bool found = false;
      for (c_class = 1; c_class < CLASS_MAX; c_class++)
      {
        if (dc_->rooms.contains(room) && dc_->rooms[room].allow_class[c_class] == true)
        {
          found = true;
          ch->send(u"%s "_s.arg(pc_clss_types[c_class]));
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

      return ReturnValue::eSUCCESS;
    }
    else
    {
      // guild <class>
      c_class = lookupClass(ch, arg1);
      if (c_class == -1)
      {
        return ReturnValue::eFAILURE;
      }

      qint32 count = {};
      ch->send(u"%s only rooms:\r\n"_s.arg(pc_clss_types[c_class]));

      qint32 cols = {};
      for (qint32 r = {}; r < dc_->top_of_world; r++)
      {
        if (dc_->rooms.contains(r) && dc_->rooms[r].allow_class[c_class] == true)
        {
          ch->send(u"%1 "_s.arg(r, 5));

          count++;
          cols++;
          if (cols == 11)
          {
            cols = {};
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

      return ReturnValue::eSUCCESS;
    }
  }

  // guild <class> <room #>
  c_class = lookupClass(ch, arg1);
  if (c_class == -1)
  {
    return ReturnValue::eFAILURE;
  }

  room = lookupRoom(ch, arg2);
  if (room == DC::NOWHERE)
  {
    return ReturnValue::eFAILURE;
  }

  if (!can_modify_room(ch, room))
  {
    ch->sendln("You are unable to work creation outside of your range.");
    return ReturnValue::eFAILURE;
  }

  if (!dc_->rooms.contains(room))
  {
    ch->send(u"Room %1 does not exist.\r\n"_s.arg(room));
    return ReturnValue::eFAILURE;
  }

  if (dc_->rooms[room].allow_class[c_class] == true)
  {
    ch->send(u"Removed %s class from room #%d's allow list.\r\n"_s.arg(pc_clss_types[c_class]).arg(room));
    dc_->rooms[room].allow_class[c_class] = false;
  }
  else
  {
    ch->send(u"Added %s class to room #%d's allow list.\r\n"_s.arg(pc_clss_types[c_class]).arg(room));
    dc_->rooms[room].allow_class[c_class] = true;
  }

  dc_->set_zone_modified_world(room);

  old_room = ch->in_room;
  ch->in_room = room;
  do_rsave(ch, "");
  ch->in_room = old_room;

  return ReturnValue::eSUCCESS;
}
