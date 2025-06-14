/************************************************************************
| $Id: who.cpp,v 1.60 2014/07/04 22:00:04 jhhudso Exp $
| who.C
| Commands for who, maybe? :P
*/
#include <cstring>

#include "DC/connect.h"
#include "DC/utility.h"
#include "DC/character.h"
#include "DC/mobile.h"
#include "DC/terminal.h"
#include "DC/player.h"
#include "DC/clan.h"
#include "DC/room.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/returnvals.h"
#include "DC/const.h"

// TODO - Figure out the weird bug for why when I do "who <class>" a random player
//        from another class will pop up who name is DC::NOWHERE near matching.

clan_data *get_clan(Character *);

// #define GWHOBUFFERSIZE   (MAX_STRING_LENGTH*2)
// char gWhoBuffer[GWHOBUFFERSIZE];

// We now use a allocated pointer for the who buffer stuff.  It stays allocated, so
// we're not repeatedly allocing it, and it grows as needed to fit all the data (like a CString)
// That way we never have to worry about having a bunch of players on, and overflowing it.
// -pir 2/20/01
char *gWhoBuffer = nullptr;
int32_t gWhoBufferCurSize = 0;
int32_t gWhoBufferMaxSize = 0;

void add_to_who(char *strAdd)
{
  int32_t strLength = 0;

  if (!strAdd)
    return;
  if (!(strLength = strlen(strAdd)))
    return;

  if ((strLength + gWhoBufferCurSize) >= gWhoBufferMaxSize)
  {                                         // expand the buffer
    gWhoBufferMaxSize += (strLength + 500); // expand by the size + 500

    gWhoBuffer = (char *)dc_realloc(gWhoBuffer, gWhoBufferMaxSize);
  }

  // guaranteed to work, since we just allocated enough for it + 500
  strcat(gWhoBuffer, strAdd);
  gWhoBufferCurSize += strLength; // update current data size
}

void clear_who_buffer()
{
  if (gWhoBuffer)
    *gWhoBuffer = '\0';  // kill the std::string
  gWhoBufferCurSize = 0; // update the size
}

int do_whogroup(Character *ch, char *argument, int cmd)
{

  Connection *d;
  Character *k, *i;
  follow_type *f;
  char target[MAX_INPUT_LENGTH];
  char tempbuffer[800];
  int foundtarget = 0;
  int foundgroup = 0;
  int hasholylight;

  one_argument(argument, target);

  hasholylight = IS_NPC(ch) ? 0 : ch->player->holyLite;

  send_to_char(
      "$B$7($4:$7)=======================================================================($4:$7)\n\r"
      "$7|$5/$7|                     $5Current Grouped Adventurers                       $7|$5/$7|\n\r"
      "$7($4:$7)=======================================================================($4:$7)$R\n\r",
      ch);

  if (*target)
  {
    sprintf(gWhoBuffer, "Searching for '$B%s$R'...\r\n", target);
    ch->send(gWhoBuffer);
  }

  clear_who_buffer();

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    foundtarget = 0;

    if ((d->connected) || (!CAN_SEE(ch, d->character)))
      continue;

    //  What the hell is this line supposed to be checking? -pir
    //  If this occurs, we got alot bigger problems than 'who_group'
    //      if (ch->desc->character != ch)
    //         continue;

    i = d->character;

    // If I'm the leader of my group, process it
    if ((!i->master) && (IS_AFFECTED(i, AFF_GROUP)))
    {
      foundgroup = 1; // we found someone!
      k = i;
      sprintf(tempbuffer, "\n\r"
                          "   $B$7[$4: $5%s $4:$7]$R\n\r"
                          "   Player kills: %-3d  Average level of victim: %d  Total kills: %-3d\n\r",
              k->group_name,
              IS_NPC(k) ? 0 : k->player->group_pkills,
              IS_NPC(k) ? 0 : (k->player->group_pkills ? (k->player->grpplvl / k->player->group_pkills) : 0),
              IS_NPC(k) ? 0 : k->player->group_kills);
      add_to_who(tempbuffer);

      // If we're searching, see if this is the target
      if (is_abbrev(target, GET_NAME(i)))
        foundtarget = 1;

      // First, if they're not anonymous
      if ((!IS_NPC(ch) && hasholylight) || (!IS_ANONYMOUS(k) || (k->clan == ch->clan && ch->clan)))
      {
        sprintf(tempbuffer,
                "   $B%-18s %-10s %-14s   Level %2lld      $1($7Leader$1)$R \n\r",
                GET_NAME(k), races[(int)GET_RACE(k)].singular_name,
                pc_clss_types[(int)GET_CLASS(k)], k->getLevel());
      }
      else
      {
        sprintf(tempbuffer,
                "   $B%-18s %-10s Anonymous                      $1($7Leader$1)$R \n\r",
                GET_NAME(k), races[(int)GET_RACE(k)].singular_name);
      }
      add_to_who(tempbuffer);

      // loop through my followers and process them
      for (f = k->followers; f; f = f->next)
      {
        if (IS_PC(f->follower))
          if (IS_AFFECTED(f->follower, AFF_GROUP))
          {
            // If we're searching, see if this is the target
            if (is_abbrev(target, GET_NAME(f->follower)))
              foundtarget = 1;
            // First if they're not anonymous
            if (!IS_ANONYMOUS(f->follower) || (f->follower->clan == ch->clan && ch->clan))
              sprintf(tempbuffer, "   %-18s %-10s %-14s   Level %2lld\n\r",
                      GET_NAME(f->follower), races[(int)GET_RACE(f->follower)].singular_name,
                      pc_clss_types[(int)GET_CLASS(f->follower)], f->follower->getLevel());
            else
              sprintf(tempbuffer,
                      "   %-18s %-10s Anonymous            \n\r",
                      GET_NAME(f->follower), races[(int)GET_RACE(f->follower)].singular_name);
            add_to_who(tempbuffer);
          }
      } // for f = k->followers
    } //  ((!i->master) && (IS_AFFECTED(i, AFF_GROUP)) )

    // if we're searching (target exists) and we didn't find it, clear out
    // the buffer cause we only want the target's group.
    // If we found it, send it out, clear the buffer, and keep going in case someone else
    // matches the same target pattern.   ('whog a' gets Anarchy and Alpha's groups)
    // -pir
    if (*target && !foundtarget)
    {
      clear_who_buffer();
      foundgroup = 0;
    }
    else if (*target && foundtarget)
    {
      ch->send(gWhoBuffer);
      clear_who_buffer();
    }
  } // End for(d).

  if (0 == foundgroup)
    add_to_who("\n\rNo groups found.\r\n");

  // page it to the player.  the 1 tells page_string to make it's own copy of the data
  page_string(ch->desc, gWhoBuffer, 1);
  return eSUCCESS;
}

int do_whosolo(Character *ch, char *argument, int cmd)
{
  Connection *d;
  Character *i;
  char tempbuffer[800];
  char buf[MAX_INPUT_LENGTH + 1];
  bool foundtarget;

  one_argument(argument, buf);

  send_to_char(
      "$B$7($4:$7)=======================================================================($4:$7)\n\r"
      "$7|$5/$7|                      $5Current SOLO Adventurers                         $7|$5/$7|\n\r"
      "$7($4:$7)=======================================================================($4:$7)$R\n\r"
      "   $BName            Race      Class        Level  PKs Deaths Avg-vict-level$R\n\r",
      ch);

  clear_who_buffer();

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    foundtarget = false;

    if ((d->connected) || !(i = d->character) || (!CAN_SEE(ch, i)))
      continue;

    if (is_abbrev(buf, GET_NAME(i)))
      foundtarget = true;

    if (*buf && !foundtarget)
      continue;

    if (i->getLevel() <= MORTAL)
      if (!IS_AFFECTED(i, AFF_GROUP))
      {
        if (!IS_ANONYMOUS(i) || (i->clan && i->clan == ch->clan))
          sprintf(tempbuffer,
                  "   %-15s %-9s %-13s %2lld     %-4d%-7d%d\n\r",
                  i->getNameC(),
                  races[(int)GET_RACE(i)].singular_name,
                  pc_clss_types[(int)GET_CLASS(i)], i->getLevel(),
                  IS_NPC(i) ? 0 : i->player->totalpkills,
                  IS_NPC(i) ? 0 : i->player->pdeathslogin,
                  IS_NPC(i) ? 0 : (i->player->totalpkills ? (i->player->totalpkillslv / i->player->totalpkills) : 0));
        else
          sprintf(tempbuffer,
                  "   %-15s %-9s Anonymous            %-4d%-7d%d\n\r",
                  i->getNameC(),
                  races[(int)GET_RACE(i)].singular_name,
                  IS_NPC(i) ? 0 : i->player->totalpkills,
                  IS_NPC(i) ? 0 : i->player->pdeathslogin,
                  IS_NPC(i) ? 0 : (i->player->totalpkills ? (i->player->totalpkillslv / i->player->totalpkills) : 0));
        add_to_who(tempbuffer);
      } // if is affected by group
  } // End For Loop.

  // page it to the player.  the 1 tells page_string to make it's own copy of the data
  page_string(ch->desc, gWhoBuffer, 1);
  return eSUCCESS;
}

command_return_t Character::do_who(QStringList arguments, int cmd)
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

  quint64 lowlevel{}, highlevel{UINT64_MAX}, numPC{}, numImmort{};
  bool anoncheck{}, sexcheck{}, guidecheck{}, lfgcheck{}, charcheck{}, nomatch{}, charmatchistrue{}, addimmbuf{};
  Character::sex_t sextype{};
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

      auto it = std::find_if(std::begin(class_names), std::end(class_names), is_abbreviation);
      if (it != std::end(class_names))
      {
        class_found = *it;
      }
      else
      {
        it = std::find_if(std::begin(race_names), std::end(race_names), is_abbreviation);
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
  send("[$4:$R]===================================[$4:$R]\n\r"
       "|$5/$R|      $BDenizens of Dark Castle$R      |$5/$R|\n\r"
       "[$4:$R]===================================[$4:$R]\n\r\n\r");

  QString buf;
  QString immbuf;
  bool hasholylight = IS_NPC(this) ? false : player->holyLite;
  for (auto d = DC::getInstance()->descriptor_list; d; d = d->next)
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
    if (d->connected != Connection::states::PLAYING && !d->isEditing())
    {
      continue;
    }

    Character *i{};
    if (d->original)
    {
      i = d->original;
    }
    else
    {
      i = d->character;
    }

    if (IS_NPC(i))
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

    // Skip std::string based checks if our name matches
    if (!charcheck || !is_abbrev(charname, GET_NAME(i)))
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
      if (!is_abbrev(charname, GET_NAME(i)))
      {
        continue;
      }
    }

    addimmbuf = false;
    if (i->getLevel() > MORTAL)
    {
      /* Immortals can't be anonymous */
      if (!str_cmp(GET_NAME(i), "Urizen"))
      {
        infoBuf = "   Meatball  ";
      }
      else if (!str_cmp(GET_NAME(i), "Julian"))
      {
        infoBuf = "    $B$7S$4a$7l$4m$7o$4n$R   ";
      }
      else if (!strcmp(GET_NAME(i), "Apocalypse"))
      {
        infoBuf = "    $5Moose$R    ";
      }
      else if (!strcmp(GET_NAME(i), "Pirahna"))
      {
        infoBuf = "   $B$4>$5<$1($2($1($5:$4>$R   ";
      }
      else if (!strcmp(GET_NAME(i), "Petra"))
      {
        infoBuf = "    $B$1R$2o$1a$2d$1i$2e$R   ";
      }
      else
      {
        infoBuf = immortFields.value(i->getLevel() - IMMORTAL);
      }

      if (level_ >= IMMORTAL && !IS_NPC(i) && i->player->wizinvis > 0)
      {
        if (!IS_NPC(i) && i->player->incognito == true)
        {
          extraBuf = QStringLiteral(" (Incognito / WizInvis %1)").arg(i->player->wizinvis);
        }
        else
        {
          extraBuf = QStringLiteral(" (WizInvis %1)").arg(i->player->wizinvis);
        }
      }
      numImmort++;
      addimmbuf = true;
    }
    else
    {
      if (!IS_ANONYMOUS(i) || (clan && clan == i->clan) || hasholylight)
      {
        infoBuf = QStringLiteral(" $B$5%1$7-$1%2  $2%3$R$7 ").arg(i->getLevel(), 2).arg(pc_clss_abbrev[(int)GET_CLASS(i)]).arg(race_abbrev[(int)GET_RACE(i)]);
      }
      else
      {
        infoBuf = QStringLiteral("  $6-==-   $B$2%1$R ").arg(race_abbrev[(int)GET_RACE(i)]);
      }
      numPC++;
    }

    if (d->isEditing())
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
      buf = QStringLiteral("[%1] %2$3%3 %4 %5 $2[%6$R$2] %7$R\n\r").arg(infoBuf).arg(preBuf).arg(GET_SHORT(i)).arg(QString(i->title)).arg(extraBuf).arg(clanPtr->name).arg(tailBuf);
    }
    else
    {
      buf = QStringLiteral("[%1] %2$3%3 %4 %5 %6$R\n\r").arg(infoBuf).arg(preBuf).arg(GET_SHORT(i)).arg(QString(i->title)).arg(extraBuf).arg(tailBuf);
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
    send("\n\r");
  }

  if (numImmort)
  {
    send(immbuf);
  }

  send(QString("\n\r"
               "    Visible Players Connected:   %1\n\r"
               "    Visible Immortals Connected: %2\n\r"
               "    (Max this boot is %3)\n\r")
           .arg(numPC)
           .arg(numImmort)
           .arg(max_who));

  return eSUCCESS;
}

int do_whoarena(Character *ch, char *argument, int cmd)
{
  int count = 0;
  clan_data *clan;

  ch->sendln("\n\rPlayers in the Arena:\n\r--------------------------");

  if (ch->getLevel() <= MORTAL)
  {
    const auto &character_list = DC::getInstance()->character_list;
    for (const auto &tmp : character_list)
    {
      if (CAN_SEE(ch, tmp))
      {
        if (tmp->room().isArena() && !isSet(DC::getInstance()->world[tmp->in_room].room_flags, NO_WHERE))
        {
          if ((tmp->clan) && (clan = get_clan(tmp)) && tmp->isMortalPlayer())
            csendf(ch, "%-20s - [%s$R]\n\r", GET_NAME(tmp), clan->name);
          else
            csendf(ch, "%-20s\n\r", GET_NAME(tmp));
          count++;
        }
      }
    }

    if (count == 0)
      ch->sendln("\n\rThere are no visible players in the arena.");

    return eSUCCESS;
  }

  // If they're here that means they're a god
  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &tmp : character_list)
  {
    if (CAN_SEE(ch, tmp))
    {
      if (tmp->room().isArena())
      {
        if ((tmp->clan) && (clan = get_clan(tmp)) && tmp->isMortalPlayer())
          csendf(ch, "%-20s  Level: %-3d  Hit: %-5d  Room: %-5d - [%s$R]\n\r",
                 GET_NAME(tmp),
                 tmp->getLevel(), tmp->getHP(), tmp->in_room, clan->name);
        else
          csendf(ch, "%-20s  Level: %-3d  Hit: %-5d  Room: %-5d\n\r",
                 GET_NAME(tmp),
                 tmp->getLevel(), tmp->getHP(), tmp->in_room);
        count++;
      }
    }
  }

  if (count == 0)
    ch->sendln("\n\rThere are no visible players in the arena.");
  return eSUCCESS;
}

int do_where(Character *ch, char *argument, int cmd)
{
  class Connection *d;
  int zonenumber;
  char buf[MAX_INPUT_LENGTH];

  one_argument(argument, buf);

  if (ch->isImmortalPlayer() && *buf && !strcmp(buf, "all"))
  { //  immortal noly, shows all
    ch->sendln("All Players:\n\r--------");
    for (d = DC::getInstance()->descriptor_list; d; d = d->next)
    {
      if (d->character && (d->connected == Connection::states::PLAYING) && (CAN_SEE(ch, d->character)) && (d->character->in_room != DC::NOWHERE))
      {
        if (d->original)
        { // If switched
          csendf(ch, "%-20s - %s$R [%lu] In body of %s\n\r", d->original->getNameC(), DC::getInstance()->world[d->character->in_room].name,
                 DC::getInstance()->world[d->character->in_room].number, fname(d->character->getNameC()).toStdString().c_str());
        }
        else
        {
          csendf(ch, "%-20s - %s$R [%lu]\n\r",
                 d->character->getNameC(), DC::getInstance()->world[d->character->in_room].name, DC::getInstance()->world[d->character->in_room].number);
        }
      }
    } // for
  }
  else if (ch->isImmortalPlayer() && *buf)
  { // immortal only, shows ONE person
    ch->sendln("Search of Players:\n\r--------");
    for (d = DC::getInstance()->descriptor_list; d; d = d->next)
    {
      if (d->character && (d->connected == Connection::states::PLAYING) && (CAN_SEE(ch, d->character)) && (d->character->in_room != DC::NOWHERE))
      {
        if (d->original)
        { // If switched
          if (is_abbrev(buf, d->original->getName()))
          {
            csendf(ch, "%-20s - %s$R [%lu] In body of %s\n\r", d->original->getNameC(), DC::getInstance()->world[d->character->in_room].name,
                   DC::getInstance()->world[d->character->in_room].number, fname(d->character->getName()).toStdString().c_str());
          }
        }
        else
        {
          if (is_abbrev(buf, d->character->getNameC()))
          {
            csendf(ch, "%-20s - %s$R [%lu]\n\r",
                   d->character->getNameC(), DC::getInstance()->world[d->character->in_room].name, DC::getInstance()->world[d->character->in_room].number);
          }
        }
      }
    } // for
  }
  else
  { // normal, mortal where
    zonenumber = DC::getInstance()->world[ch->in_room].zone;
    ch->sendln("Players in your vicinity:\n\r-------------------------");
    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_WHERE))
      return eFAILURE;
    for (d = DC::getInstance()->descriptor_list; d; d = d->next)
    {
      if (d->character && (d->connected == Connection::states::PLAYING) && (d->character->in_room != DC::NOWHERE) &&
          !isSet(DC::getInstance()->world[d->character->in_room].room_flags, NO_WHERE) &&
          CAN_SEE(ch, d->character) && !IS_NPC(d->character) /*Don't show snooped mobs*/)
      {
        if (DC::getInstance()->world[d->character->in_room].zone == zonenumber)
          csendf(ch, "%-20s - %s$R\n\r", d->character->getNameC(),
                 DC::getInstance()->world[d->character->in_room].name);
      }
    }
  }

  return eSUCCESS;
}
