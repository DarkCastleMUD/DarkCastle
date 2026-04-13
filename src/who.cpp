/************************************************************************
| $Id: who.cpp,v 1.60 2014/07/04 22:00:04 jhhudso Exp $
| who.C
| Commands for who, maybe? :P
*/
#include <cstring>
#include "DC/levels.h"

#include "DC/DC.h"
#include "DC/clan.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/returnvals.h"
#include "DC/const.h"

ClanPtr get_clan(CharacterPtr);

command_return_t do_whogroup(CharacterPtr ch, QString argument, cmd_t cmd)
{
  auto arguments = QString(argument).split(' ');
  Connection *d{};
  CharacterPtr k{}, i{};
  follow_type *f{};
  qint32 foundtarget{};
  qint32 foundgroup{};
  auto target = arguments.value(0);

  auto hasholylight = ch->isNonPlayer() ? false : ch->player->holyLite;

  send_to_char("$B$7($4:$7)=======================================================================($4:$7)\r\n"
               "$7|$5/$7|                     $5Current Grouped Adventurers                       $7|$5/$7|\r\n"
               "$7($4:$7)=======================================================================($4:$7)$R\r\n",
               ch);

  if (!target.isEmpty())
  {
    ch->sendln(u"Searching for '$B%1$R'..."_s.arg(target));
  }

  QString tempbuffer;
  for (auto &d : dc_->connections_)
  {
    foundtarget = {};

    if ((conn->connected) || (!CAN_SEE(ch, conn->character)))
      continue;

    //  What the hell is this line supposed to be checking? -pir
    //  If this occurs, we got alot bigger problems than 'who_group'
    //      if (ch->desc->character != ch)
    //         continue;

    i = conn->character;

    // If I'm the leader of my group, process it
    if ((!i->master) && (IS_AFFECTED(i, AFF_GROUP)))
    {
      foundgroup = 1; // we found someone!
      k = i;
      tempbuffer += u"\r\n"_s
                                   "   $B$7[$4: $5%1 $4:$7]$R\r\n"
                                   "   Player kills: %2  Average level of victim: %3  Total kills: %4\r\n")
                        .arg(k->group_name)
                        .arg(k->isNonPlayer() ? 0 : k->player->group_pkills, -3)
                        .arg(k->isNonPlayer() ? 0 : (k->player->group_pkills ? (k->player->grpplvl / k->player->group_pkills) : 0))
                        .arg(k->isNonPlayer() ? 0 : k->player->group_kills, -3);
      // If we're searching, see if this is the target
      if (is_abbrev(target, qPrintable(i->name())))
        foundtarget = 1;

      // First, if they're not anonymous
      if ((!ch->isNonPlayer() && hasholylight) || (!IS_ANONYMOUS(k) || (k->clan == ch->clan && ch->clan)))
      {
        tempbuffer += u"   $B%1 %2 %3   Level %4      $1($7Leader$1)$R \r\n"_s
                          .arg(k->name(), -18)
                          .arg(races[(qint32)k->race].singular_name, -10)
                          .arg(pc_clss_types[(qint32)GET_CLASS(k)], -14)
                          .arg(k->getLevel(), 2);
      }
      else
      {
        tempbuffer += u"   $B%1 %2 Anonymous                      $1($7Leader$1)$R \r\n"_s.arg(k->name(), -18).arg(races[(qint32)k->race].singular_name, -10);
      }

      // loop through my followers and process them
      for (f = k->followers; f; f = f->next)
      {
        if (f->follower->isPlayer())
          if (IS_AFFECTED(f->follower, AFF_GROUP))
          {
            // If we're searching, see if this is the target
            if (is_abbrev(target, qPrintable(f->follower->name())))
              foundtarget = 1;
            // First if they're not anonymous
            if (!IS_ANONYMOUS(f->follower) || (f->follower->clan == ch->clan && ch->clan))
              tempbuffer += u"   %1 %2 %3   Level %4\r\n"_s
                                .arg(f->follower->name(), -18)
                                .arg(races[(qint32)f->follower->race].singular_name, -10)
                                .arg(pc_clss_types[(qint32)GET_CLASS(f->follower)], -14)
                                .arg(f->follower->getLevel(), 2);
            else
              tempbuffer += u"   %1 %2 Anonymous            \r\n"_s
                                .arg(f->follower->name(), -18)
                                .arg(races[(qint32)f->follower->race].singular_name, -10);
          }
      } // for f = k->followers
    } //  ((!i->master) && (IS_AFFECTED(i, AFF_GROUP)) )

    // if we're searching (target exists) and we didn't find it, clear out
    // the buffer cause we only want the target's group.
    // If we found it, send it out, clear the buffer, and keep going in case someone else
    // matches the same target pattern.   ('whog a' gets Anarchy and Alpha's groups)
    // -pir
    if (!target.isEmpty() && !foundtarget)
    {
      foundgroup = {};
    }
    else if (!target.isEmpty() && foundtarget)
    {
      ch->send(tempbuffer);
    }
  } // End for(d).

  if (!foundgroup)
    ch->sendln("\r\nNo groups found.");

  // page it to the player.  the 1 tells page_string to make it's own copy of the data
  page_string(ch->desc, qPrintable(tempbuffer), 1);
  return ReturnValue::eSUCCESS;
}

command_return_t do_whosolo(CharacterPtr ch, QString argument, cmd_t cmd)
{
  auto arguments = QString(argument).split(' ');
  auto target = arguments.value(0);
  send_to_char("$B$7($4:$7)=======================================================================($4:$7)\r\n"
               "$7|$5/$7|                      $5Current SOLO Adventurers                         $7|$5/$7|\r\n"
               "$7($4:$7)=======================================================================($4:$7)$R\r\n"
               "   $BName            Race      Class        Level  PKs Deaths Avg-vict-level$R\r\n",
               ch);

  bool foundtarget{};
  CharacterPtr i{};
  QString tempbuffer;
  for (auto d = dc_->connections_; d; d = conn->next)
  {
    foundtarget = false;

    if ((conn->connected) || !(i = conn->character) || (!CAN_SEE(ch, i)))
      continue;

    if (is_abbrev(target, qPrintable(i->name())))
      foundtarget = true;

    if (!target.isEmpty() && !foundtarget)
      continue;

    if (i->getLevel() <= MORTAL)
      if (!IS_AFFECTED(i, AFF_GROUP))
      {
        if (!IS_ANONYMOUS(i) || (i->clan && i->clan == ch->clan))
          tempbuffer += u"   %1 %2 %3 %4     %5%6%7\r\n"_s
                            .arg(i->name(), -15)
                            .arg(races[(qint32)i->race].singular_name, -9)
                            .arg(pc_clss_types[(qint32)GET_CLASS(i)], -13)
                            .arg(i->getLevel(), 2)
                            .arg(i->isNonPlayer() ? 0 : i->player->totalpkills, -4)
                            .arg(i->isNonPlayer() ? 0 : i->player->pdeathslogin, -7)
                            .arg(i->isNonPlayer() ? 0 : (i->player->totalpkills ? (i->player->totalpkillslv / i->player->totalpkills) : 0));
        else
          tempbuffer += u"   %1 %2 Anonymous            %3%4%5\r\n"_s
                            .arg(i->name(), -15)
                            .arg(races[(qint32)i->race].singular_name, -9)
                            .arg(i->isNonPlayer() ? 0 : i->player->totalpkills, -4)
                            .arg(i->isNonPlayer() ? 0 : i->player->pdeathslogin, -7)
                            .arg(i->isNonPlayer() ? 0 : (i->player->totalpkills ? (i->player->totalpkillslv / i->player->totalpkills) : 0));
      } // if is affected by group
  } // End For Loop.

  // page it to the player.  the 1 tells page_string to make it's own copy of the data
  page_string(ch->desc, qPrintable(tempbuffer), 1);
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_who(QStringList arguments, cmd_t cmd)
{
  const QStringList immortFields = {
      "   Immortal  ",
      "  Architect  ",
      "    Deity    ",
      "   Overseer  ",
      "   Divinity  ",
      "   --------  ",
      " Coordinator ",
      "   --------  ",
      " Implementer "};

  quint64 lowlevel{}, highlevel{UINT64_MAX}, numPC{}, numImmort = {};
  bool anoncheck{}, sexcheck{}, guidecheck{}, lfgcheck{}, charcheck{}, nomatch{}, charmatchistrue{}, addimmbuf = {};
  Character::sex_t sextype = {};
  QString charname, class_found, race_found;
  for (const auto &oneword : arguments)
  {
    bool ok = false;
    auto levelarg = oneword.toULongLong(&ok);
    if (ok)
    {
      if (!lowlevel)
      {
        lowlevel = levelarg;
      }
      else if (lowlevel > levelarg)
      {
        highlevel = lowlevel;
        lowlevel = levelarg;
      }
      else
      {
        highlevel = levelarg;
      }
      continue;
    }

    // note that for all these, we don't 'continue' cause we want
    // to check for a name match at the end in case some annoying mortal
    // named himself "Anonymous" or "Penis", etc.
    if (is_abbrev(oneword, "anonymous"))
    {
      anoncheck = true;
    }
    else if (is_abbrev(oneword, "penis") || is_abbrev(oneword, "male"))
    {
      sexcheck = true;
      sextype = Character::sex_t::MALE;
    }
    else if (is_abbrev(oneword, "guide"))
    {
      guidecheck = true;
    }
    else if (is_abbrev(oneword, "vagina") || is_abbrev(oneword, "female"))
    {
      sexcheck = true;
      sextype = Character::sex_t::FEMALE;
    }
    else if (is_abbrev(oneword, "other") || is_abbrev(oneword, "neutral"))
    {
      sexcheck = true;
      sextype = Character::sex_t::NEUTRAL;
    }
    else if (is_abbrev(oneword, "lfg"))
    {
      lfgcheck = true;
    }
    else
    {
      auto is_abbreviation = [&](auto fullname)
      { return is_abbrev(oneword, fullname); };

      auto it = std::std::find_if(std::begin(class_names), std::end(class_names), is_abbreviation);
      if (it != std::end(class_names))
      {
        class_found = *it;
      }
      else
      {
        it = std::std::find_if(std::begin(race_names), std::end(race_names), is_abbreviation);
        if (it != std::end(race_names))
        {
          race_found = *it;
        }
        else
        {
          charname = oneword;
          charcheck = true;
        }
      }
    }
  } // end of for loop

  // Display the actual stuff
  send("[$4:$R]===================================[$4:$R]\r\n"
       "|$5/$R|      $BDenizens of Dark Castle$R      |$5/$R|\r\n"
       "[$4:$R]===================================[$4:$R]\r\n\r\n");

  QString buf;
  QString immbuf;
  bool hasholylight = this->isNonPlayer() ? false : player->holyLite;
  for (auto d = dc_->connections_; d; d = conn->next)
  {
    QString infoBuf;
    QString extraBuf;
    QString tailBuf;
    QString preBuf;

    // we have an invalid match arg, so nothing is going to match
    if (nomatch)
    {
      break;
    }

    // Don't show any connection that's not playing or editing on who list
    if (conn->connected != Connection::states::PLAYING && !conn->isEditing())
    {
      continue;
    }

    CharacterPtr i = {};
    if (conn->original)
    {
      i = conn->original;
    }
    else
    {
      i = conn->character;
    }

    if (i->isNonPlayer())
    {
      continue;
    }
    if (!CAN_SEE(this, i))
    {
      continue;
    }
    // Level checks.  These happen no matter what
    if (i->getLevel() < lowlevel)
    {
      continue;
    }

    if (i->getLevel() > highlevel)
    {
      continue;
    }

    if (!class_found.isEmpty() && !hasholylight && (!i->clan || i->clan != this->clan) && IS_ANONYMOUS(i) && i->getLevel() < MIN_GOD)
    {
      continue;
    }
    if (lowlevel > 0 && IS_ANONYMOUS(i) && !hasholylight)
    {
      continue;
    }

    // Skip QString based checks if our name matches
    if (!charcheck || !is_abbrev(charname, qPrintable(i->name())))
    {
      if (!class_found.isEmpty() && i->getClassName() != class_found && !charmatchistrue)
      {
        continue;
      }
      if (anoncheck && !IS_ANONYMOUS(i) && !charmatchistrue)
      {
        continue;
      }
      if (sexcheck && (GET_SEX(i) != sextype || (IS_ANONYMOUS(i) && !hasholylight)) && !charmatchistrue)
      {
        continue;
      }
      if (lfgcheck && !isSet(i->player->toggles, Player::PLR_LFG))
      {
        continue;
      }
      if (guidecheck && !isSet(i->player->toggles, Player::PLR_GUIDE_TOG))
      {
        continue;
      }
      if (!race_found.isEmpty() && i->getRaceName() != race_found && !charmatchistrue)
      {
        continue;
      }
    }

    if (charcheck)
    {
      if (!is_abbrev(charname, qPrintable(i->name())))
      {
        continue;
      }
    }

    addimmbuf = false;
    if (i->getLevel() > MORTAL)
    {
      /* Immortals can't be anonymous */
      if (!str_cmp(qPrintable(i->name()), "Urizen"))
      {
        infoBuf = "   Meatball  ";
      }
      else if (!str_cmp(qPrintable(i->name()), "Julian"))
      {
        infoBuf = "    $B$7S$4a$7l$4m$7o$4n$R   ";
      }
      else if (!dc_strcmp(qPrintable(i->name()), "Apocalypse"))
      {
        infoBuf = "    $5Moose$R    ";
      }
      else if (!dc_strcmp(qPrintable(i->name()), "Pirahna"))
      {
        infoBuf = "   $B$4>$5<$1($2($1($5:$4>$R   ";
      }
      else if (!dc_strcmp(qPrintable(i->name()), "Petra"))
      {
        infoBuf = "    $B$1R$2o$1a$2d$1i$2e$R   ";
      }
      else
      {
        infoBuf = immortFields.value(i->getLevel() - IMMORTAL);
      }

      if (level_ >= IMMORTAL && !i->isNonPlayer() && i->player->wizinvis > 0)
      {
        if (!i->isNonPlayer() && i->player->incognito == true)
        {
          extraBuf = u" (Incognito / WizInvis %1)"_s.arg(i->player->wizinvis);
        }
        else
        {
          extraBuf = u" (WizInvis %1)"_s.arg(i->player->wizinvis);
        }
      }
      numImmort++;
      addimmbuf = true;
    }
    else
    {
      if (!IS_ANONYMOUS(i) || (clan && clan == i->clan) || hasholylight)
      {
        infoBuf = u" $B$5%1$7-$1%2  $2%3$R$7 "_s.arg(i->getLevel(), 2).arg(pc_clss_abbrev[(qint32)GET_CLASS(i)]).arg(race_abbrev[(qint32)i->race]);
      }
      else
      {
        infoBuf = u"  $6-==-   $B$2%1$R "_s.arg(race_abbrev[(qint32)i->race]);
      }
      numPC++;
    }

    if (conn->isEditing())
    {
      tailBuf = "$1$B(writing) ";
    }

    if (isSet(i->player->toggles, Player::PLR_GUIDE_TOG))
    {
      preBuf = "$7$B(Guide)$R ";
    }

    if (isSet(i->player->toggles, Player::PLR_LFG))
    {
      tailBuf += "$3(LFG) ";
    }

    if (IS_AFFECTED(i, AFF_CHAMPION))
    {
      tailBuf += "$B$4(Champion)$R";
    }

    auto clanPtr = get_clan(i);
    if (i->clan && clanPtr && i->getLevel() < OVERSEER)
    {
      buf = u"[%1] %2$3%3 %4 "_s.arg(infoBuf).arg(preBuf).arg(qPrintable(i->shortdesc_or_name())).arg(i->title);
      buf += u"%5 $2[%6$R$2] %7$R\r\n"_s.arg(extraBuf).arg(clanPtr->name()).arg(tailBuf);
    }
    else
    {
      buf = u"[%1] %2$3%3 %4 "_s.arg(infoBuf).arg(preBuf).arg(qPrintable(i->shortdesc_or_name())).arg(i->title);
      buf += u"%5 %6$R\r\n"_s.arg(extraBuf).arg(tailBuf);
    }

    if (addimmbuf)
    {
      immbuf += buf;
    }
    else
    {
      send(buf);
    }
  }

  if (numPC && numImmort)
  {
    send("\r\n");
  }

  if (numImmort)
  {
    send(immbuf);
  }

  send(QString("\r\n"
               "    Visible Players Connected:   %1\r\n"
               "    Visible Immortals Connected: %2\r\n"
               "    (Max this boot is %3)\r\n")
           .arg(numPC)
           .arg(numImmort)
           .arg(max_who));

  return ReturnValue::eSUCCESS;
}

command_return_t do_whoarena(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 count = {};
  ClanPtr clan;

  ch->sendln("\r\nPlayers in the Arena:\r\n--------------------------");

  if (ch->getLevel() <= MORTAL)
  {
    const auto &character_list = dc_->character_list;
    for (const auto &tmp : character_list)
    {
      if (CAN_SEE(ch, tmp))
      {
        if (tmp->room().isArena() && !isSet(dc_->world[tmp->in_room].room_flags, NO_WHERE))
        {
          if ((tmp->clan) && (clan = get_clan(tmp)) && tmp->isMortalPlayer())
            ch->send(u"%-20s - [%s$R]\r\n"_s.arg(qPrintable(tmp->name())).arg(qPrintable(clan->name())));
          else
            ch->send(u"%-20s\r\n"_s.arg(qPrintable(tmp->name())));
          count++;
        }
      }
    }

    if (count == 0)
      ch->sendln("\r\nThere are no visible players in the arena.");

    return ReturnValue::eSUCCESS;
  }

  // If they're here that means they're a god
  const auto &character_list = dc_->character_list;
  for (const auto &tmp : character_list)
  {
    if (CAN_SEE(ch, tmp))
    {
      if (tmp->room().isArena())
      {
        if ((tmp->clan) && (clan = get_clan(tmp)) && tmp->isMortalPlayer())
          ch->send(u"%-20s  Level: %-3d  Hit: %-5d  Room: %-5d - [%s$R]\r\n"_s.arg(qPrintable(tmp->name()), tmp->getLevel()).arg(tmp->getHP()).arg(tmp->in_room).arg(qPrintable(clan->name())));
        else
          ch->send(u"%-20s  Level: %-3d  Hit: %-5d  Room: %-5d\r\n"_s.arg(qPrintable(tmp->name())).arg(tmp->getLevel()).arg(tmp->getHP()).arg(tmp->in_room));
        count++;
      }
    }
  }

  if (count == 0)
    ch->sendln("\r\nThere are no visible players in the arena.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_where(CharacterPtr ch, QString argument, cmd_t cmd)
{
  class Connection *d;
  qint32 zonenumber;
  QString buf;

  one_argument(argument, buf);

  if (ch->isImmortalPlayer() && *buf && buf == u"all"_s)
  { //  immortal noly, shows all
    ch->sendln("All Players:\r\n--------");
    for (auto &d : dc_->connections_)
    {
      if (conn->character && (conn->connected == Connection::states::PLAYING) && (CAN_SEE(ch, conn->character)) && (conn->character->in_room != DC::NOWHERE))
      {
        if (conn->original)
        { // If switched
          ch->send(u"%-20s - %s$R [%d] In body of %s\r\n"_s.arg(qPrintable(conn->original->name())).arg(dc_->world[conn->character->in_room].name).arg(dc_->world[conn->character->in_room].number).arg(qPrintable(fname(conn->character->name()))));
        }
        else
        {
          ch->send(u"%-20s - %s$R [%d]\r\n"_s.arg(qPrintable(conn->character->name())).arg(dc_->world[conn->character->in_room].name).arg(dc_->world[conn->character->in_room].number));
        }
      }
    } // for
  }
  else if (ch->isImmortalPlayer() && *buf)
  { // immortal only, shows ONE person
    ch->sendln("Search of Players:\r\n--------");
    for (auto &d : dc_->connections_)
    {
      if (conn->character && (conn->connected == Connection::states::PLAYING) && (CAN_SEE(ch, conn->character)) && (conn->character->in_room != DC::NOWHERE))
      {
        if (conn->original)
        { // If switched
          if (is_abbrev(buf, conn->original->name()))
          {
            ch->send(u"%-20s - %s$R [%d] In body of %s\r\n"_s.arg(qPrintable(conn->original->name())).arg(dc_->world[conn->character->in_room].name).arg(dc_->world[conn->character->in_room].number).arg(qPrintable(fname(conn->character->name()))));
          }
        }
        else
        {
          if (is_abbrev(buf, qPrintable(conn->character->name())))
          {
            ch->send(u"%-20s - %s$R [%d]\r\n"_s.arg(qPrintable(conn->character->name())).arg(dc_->world[conn->character->in_room].name).arg(dc_->world[conn->character->in_room].number));
          }
        }
      }
    } // for
  }
  else
  { // normal, mortal where
    zonenumber = dc_->world[ch->in_room].zone;
    ch->sendln("Players in your vicinity:\r\n-------------------------");
    if (isSet(dc_->world[ch->in_room].room_flags, NO_WHERE))
      return ReturnValue::eFAILURE;
    for (auto &d : dc_->connections_)
    {
      /*Don't show snooped mobs*/
      if (conn->character &&
          (conn->connected == Connection::states::PLAYING) &&
          (conn->character->in_room != DC::NOWHERE) &&
          !isSet(dc_->world[conn->character->in_room].room_flags, NO_WHERE) &&
          CAN_SEE(ch, conn->character) &&
          !conn->character->isNonPlayer())
      {
        if (dc_->world[conn->character->in_room].zone == zonenumber)
          ch->send(u"%-20s - %s$R\r\n"_s.arg(qPrintable(conn->character->name())).arg(dc_->world[conn->character->in_room].name));
      }
    }
  }

  return ReturnValue::eSUCCESS;
}
