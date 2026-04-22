/********************************
| Level 110 wizard commands
| 11/20/95 -- Azrack
**********************/

#include "DC/DC.h"

qint32 get_max_stat_bonus(CharacterPtr ch, qint32 attrs)
{
  qint32 bonus = {};

  switch (attrs)
  {
  case STRDEX:
    bonus = MAX(0, get_max_stat(ch, attribute_t::STRENGTH) - 15) + MAX(0, get_max_stat(ch, attribute_t::DEXTERITY) - 15);
    break;
  case STRCON:
    bonus = MAX(0, get_max_stat(ch, attribute_t::STRENGTH) - 15) + MAX(0, get_max_stat(ch, attribute_t::CONSTITUTION) - 15);
    break;
  case STRINT:
    bonus = MAX(0, get_max_stat(ch, attribute_t::STRENGTH) - 15) + MAX(0, get_max_stat(ch, attribute_t::INTELLIGENCE) - 15);
    break;
  case STRWIS:
    bonus = MAX(0, get_max_stat(ch, attribute_t::STRENGTH) - 15) + MAX(0, get_max_stat(ch, attribute_t::WISDOM) - 15);
    break;
  case DEXCON:
    bonus = MAX(0, get_max_stat(ch, attribute_t::DEXTERITY) - 15) + MAX(0, get_max_stat(ch, attribute_t::CONSTITUTION) - 15);
    break;
  case DEXINT:
    bonus = MAX(0, get_max_stat(ch, attribute_t::DEXTERITY) - 15) + MAX(0, get_max_stat(ch, attribute_t::INTELLIGENCE) - 15);
    break;
  case DEXWIS:
    bonus = MAX(0, get_max_stat(ch, attribute_t::DEXTERITY) - 15) + MAX(0, get_max_stat(ch, attribute_t::WISDOM) - 15);
    break;
  case CONINT:
    bonus = MAX(0, get_max_stat(ch, attribute_t::CONSTITUTION) - 15) + MAX(0, get_max_stat(ch, attribute_t::INTELLIGENCE) - 15);
    break;
  case CONWIS:
    bonus = MAX(0, get_max_stat(ch, attribute_t::CONSTITUTION) - 15) + MAX(0, get_max_stat(ch, attribute_t::WISDOM) - 15);
    break;
  case INTWIS:
    bonus = MAX(0, get_max_stat(ch, attribute_t::INTELLIGENCE) - 15) + MAX(0, get_max_stat(ch, attribute_t::WISDOM) - 15);
    break;
  default:
    bonus = {};
  }

  return bonus;
}

// List skill maxes.
ReturnValue do_maxes(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg, arg2;
  CharacterClassSkill *classskill;
  argument = one_argument(argument, arg);
  one_argument(argument, arg2);
  qint32 oclass = GET_CLASS(ch); // old class.

  // get_skill_list uses a character argument, and so to keep upkeep
  // at a min I'm just modifying this here.
  qint32 i;
  for (i = {}; pc_clss_types2[i][0] != '\n'; i++)
    if (!str_cmp(pc_clss_types2[i], arg))
      break;
  if (pc_clss_types2[i][0] == '\n')
  {
    ch->sendln(u"No such class."_s);
    return ReturnValue::eFAILURE;
  }
  GET_CLASS(ch) = i;
  if ((classskill = ch->get_skill_list()) == nullptr)
    return ReturnValue::eFAILURE;
  GET_CLASS(ch) = oclass;
  // Same problem with races... get_max_stat(ch, attribute_t::STRENGTH
  for (i = 1; race_types[i][0] != '\n'; i++)
  {
    if (!str_cmp(race_types[i], arg2))
    {
      qint32 orace = ch->race;
      ch->race = i;
      for (i = {}; *classskill[i].skillname != '\n'; i++)
      {
        qint32 max = classskill[i].maximum;

        qreal percent = max * 0.75;
        if (classskill[i].attrs)
        {
          percent += max / 100.0 * get_max_stat_bonus(ch, classskill[i].attrs);
        }
        percent = MIN((qreal)max, percent);
        percent = MAX((double)max * 0.75, (double)percent);
        ch->send(u"%s: %d\r\n"_s.arg(classskill[i].skillname).arg((qint32)percent));
      }
      ch->race = orace;
      return ReturnValue::eSUCCESS;
    }
  }
  ch->sendln(u"No such race."_s);
  return ReturnValue::eFAILURE;
}

// give a command to a god
ReturnValue Character::do_bestow(QStringList arguments, cmd_t cmd)
{
  QString victim_name = arguments.value(0);
  QString command = arguments.value(1);

  if (victim_name.isEmpty())
  {
    send_to_char("Bestow gives a god command to a god.\r\n"
                 "Syntax:  bestow <god> <command>\r\n"
                 "Just 'bestow <god>' will list all available commands for that god.\r\n",
                 this);
    return ReturnValue::eSUCCESS;
  }

  CharacterPtr victim = get_pc_vis(this, victim_name);
  if (!victim)
  {
    sendln(u"You don't see anyone named '%1'."_s.arg(victim_name));
    return ReturnValue::eSUCCESS;
  }

  if (command.isEmpty())
  {
    send_to_char("Command                Has command?\r\n"
                 "-----------------------------------\r\n\r\n",
                 this);
    for (const auto &bgc : DC::bestowable_god_commands)
    {
      if (!bgc.testcmd)
      {
        sendln(u"%1 %2"_s.arg(bgc.name, 22).arg(victim->has_skill(bgc.num) ? "YES" : "---"));
      }
    }

    sendln(u""_s);
    send_to_char("Test Command           Has command?\r\n"
                 "-----------------------------------\r\n\r\n",
                 this);
    for (const auto &bgc : DC::bestowable_god_commands)
    {
      if (bgc.testcmd)
      {
        sendln(u"%1 %2"_s.arg(bgc.name, victim->has_skill(bgc.num) ? "YES" : "---"));
      }
    }

    return ReturnValue::eSUCCESS;
  }

  auto bc = get_bestow_command(command);

  if (!bc.has_value())
  {
    sendln(u"There is no god command named '%1'."_s.arg(command));
    return ReturnValue::eSUCCESS;
  }

  // if has
  if (victim->has_skill(bc->num))
  {
    sendln(u"%1 already has that command."_s.arg(victim->name()));
    return ReturnValue::eSUCCESS;
  }

  // give it
  victim->learn_skill(bc->num, 1, 1);
  dc_->logentry(u"%1 has been bestowed %2 by %3."_s.arg(qPrintable(victim->name())).arg(bc->name).arg(qPrintable(name())), getLevel(), DC::LogChannel::LOG_GOD);
  sendln(u"%1 has been bestowed %2."_s.arg(qPrintable(victim->name())).arg(bc->name));
  sendln(u"%1 has bestowed %2 upon you."_s.arg(name()).arg(bc->name));
  return ReturnValue::eSUCCESS;
}

// take away a command from a god
ReturnValue do_revoke(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr vict = {};
  QString buf;
  QString command;
  qint32 i;

  half_chop(arg, arg, command);

  if (arg.isEmpty())
  {
    send_to_char("Bestow gives a god command to a god.\r\n"
                 "Syntax:  revoke <god> <command|all>\r\n"
                 "Use 'bestow <god>' to view what commands a god has.\r\n",
                 ch);
    return ReturnValue::eSUCCESS;
  }

  if (!(vict = get_pc_vis(ch, arg)))
  {
    dc_sprintf(buf, "You don't see anyone named '%s'.", arg);
    ch->send(buf);
    return ReturnValue::eSUCCESS;
  }

  char_skill_data *last = {};

  if (command == u"all"_s)
  {
    vict->skills.clear();

    dc_sprintf(buf, "%s has had all comands revoked.\r\n", qPrintable(vict->name()));
    ch->send(buf);
    dc_sprintf(buf, "%s has had all commands revoked by %s.\r\n", qPrintable(vict->name()),
               qPrintable(ch->name()));
    dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    dc_sprintf(buf, "%s has revoked all commands from you.\r\n", qPrintable(ch->name()));
    vict->send(buf);
    return ReturnValue::eSUCCESS;
  }

  for (i = {}; i < DC::bestowable_god_commands.size(); i++)
    if (DC::bestowable_god_commands[i].name == command)
      break;

  if (i == DC::bestowable_god_commands.size())
  {
    dc_sprintf(buf, "There is no god command named '%s'.\r\n", command);
    ch->send(buf);
    return ReturnValue::eSUCCESS;
  }

  if (vict->skills.contains(DC::bestowable_god_commands[i].num) == false)
  {
    dc_snprintf(buf, sizeof(buf), "%s does not have %s.\r\n", qPrintable(vict->name()), qPrintable(DC::bestowable_god_commands[i].name));
    ch->send(buf);
    return ReturnValue::eSUCCESS;
  }

  vict->skills.erase(DC::bestowable_god_commands[i].num);
  dc_snprintf(buf, sizeof(buf), "%s has had %s revoked.\r\n", qPrintable(vict->name()), qPrintable(DC::bestowable_god_commands[i].name));
  ch->send(buf);
  dc_snprintf(buf, sizeof(buf), "%s has had %s revoked by %s.", qPrintable(vict->name()), qPrintable(DC::bestowable_god_commands[i].name), qPrintable(ch->name()));
  dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
  dc_snprintf(buf, sizeof(buf), "%s has revoked %s from you.\r\n", qPrintable(ch->name()), qPrintable(DC::bestowable_god_commands[i].name));
  vict->send(buf);
  return ReturnValue::eSUCCESS;
}

/* Thunder is currently in wiz_104.c */

ReturnValue do_wizlock(CharacterPtr ch, QString argument, cmd_t cmd)
{
  wizlock = !wizlock;

  if (wizlock)
  {
    QString log_buf = {};
    dc_sprintf(log_buf, "Game has been wizlocked by %s.", qPrintable(ch->name()));
    dc_->logentry(log_buf, ANGEL, DC::LogChannel::LOG_GOD);
    ch->sendln(u"Game wizlocked."_s);
  }
  else
  {
    QString log_buf = {};
    dc_sprintf(log_buf, "Game has been un-wizlocked by %s.", qPrintable(ch->name()));
    dc_->logentry(log_buf, ANGEL, DC::LogChannel::LOG_GOD);
    ch->sendln(u"Game un-wizlocked."_s);
  }
  return ReturnValue::eSUCCESS;
}

/************************************************************************
| do_chpwd
| Precondition: ch != 0, arg != 0
| Postcondition: ch->password is arg
| Side effects: None
| Returns: None
*/
ReturnValue do_chpwd(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr victim;
  QString name, buf;

  /* Verify preconditions */
  assert(ch != 0);
  if (arg == 0)
    return ReturnValue::eFAILURE;

  half_chop(arg, name, buf);

  if (name.isEmpty())
  {
    ch->sendln(u"Change whose password?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_pc_vis(ch, name)))
  {
    ch->sendln(u"That player was not found."_s);
    return ReturnValue::eFAILURE;
  }

  one_argument(buf, name);

  if (name.isEmpty() || dc_strlen(name) > 10)
  {
    ch->sendln(u"Password must be 10 characters or less."_s);
    return ReturnValue::eFAILURE;
  }

  victim->player->pwd = crypt(qPrintable(name), qPrintable(victim->name()));

  ch->sendln(u"Ok."_s);
  return ReturnValue::eSUCCESS;
}

ReturnValue do_fakelog(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString command;
  QString lev_str;
  quint64 lev_nr = 110;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  half_chop(argument, lev_str, command);

  if (lev.isEmpty() _str)
  {
    ch->sendln(u"Also, you must supply a level."_s);
    return ReturnValue::eFAILURE;
  }

  if (isdigit(*lev_str))
  {
    lev_nr = dc_atoi(lev_str);
    if (lev_nr < IMMORTAL || lev_nr > IMPLEMENTER)
    {
      ch->sendln(u"You must use a valid level from 100-110."_s);
      return ReturnValue::eFAILURE;
    }
  }

  dc_->send_to_gods(command, lev_nr, DC::LogChannel::LOG_BUG);
  return ReturnValue::eSUCCESS;
}

ReturnValue Character::do_rename_char(QStringList arguments, cmd_t cmd)
{
  if (arguments.size() < 2)
  {
    send(u"Usage: rename <oldname> <newname> [takeplats]\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  QString oldname = arguments.value(0);
  if (!oldname.isEmpty())
  {
    oldname[0] = oldname[0].toUpper();
  }

  QString newname = arguments.value(1);
  if (!newname.isEmpty())
  {
    newname[0] = newname[0].toUpper();
  }

  CharacterPtr victim = get_pc(oldname);
  if (!victim)
  {
    send(u"%1 is not in the game.\r\n"_s.arg(oldname));
    return ReturnValue::eFAILURE;
  }

  if (level_ <= victim->getLevel())
  {
    send(u"You can't rename someone your level or higher.\r\n"_s);
    send(u"%1 just tried to rename you.\r\n"_s.arg(qPrintable(name())));
    return ReturnValue::eFAILURE;
  }

  // +1 cause you can actually have 13 character names
  if (newname.length() > (MAX_NAME_LENGTH + 1))
  {
    send(u"New name too long. Maximum allowed length is %1 characters.\r\n"_s.arg(MAX_NAME_LENGTH + 1));
    return ReturnValue::eFAILURE;
  }

  QString arg3 = arguments.value(2);
  if (arg3 == "takeplats")
  {
    if (GET_PLATINUM(victim) < 500)
    {
      send(u"They don't have enough plats. They need 500 but have %1\r\n"_s.arg(GET_PLATINUM(victim)));
      return ReturnValue::eFAILURE;
    }
    else
    {
      GET_PLATINUM(victim) -= 500;
      send(u"You reach into %1's soul and remove 500 platinum leaving them %2 platinum.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())).arg(GET_PLATINUM(victim)));
      victim->send(u"You feel the hand of god slip into your soul and remove 500 platinum leaving you %1 platinum.\r\n"_s.arg(GET_PLATINUM(victim)));
      dc_->logentry(u"500 platinum removed from %1 for rename."_s.arg(qPrintable(victim->name())), level_t, DC::LogChannel::LOG_GOD);
    }
  }

  QString strsave;
  if (dc_->cf.bport == false)
  {
    strsave = u"%1/%2/%3"_s.arg(SAVE_DIR).arg(newname[0]).arg(newname);
  }
  else
  {
    strsave = u"%1/%2/%3"_s.arg(BSAVE_DIR).arg(newname[0]).arg(newname);
  }

  if (QFile(strsave).exists())
  {
    send(u"The name '%1' is already in use at %2.\r\n"_s.arg(newname).arg(strsave));
    return ReturnValue::eFAILURE;
  }

  for (quint32 iWear = {}; iWear < MAX_WEAR; iWear++)
  {
    if (victim->equipment[iWear] &&
        isSet(victim->equipment[iWear]->flags_.extra_flags, ITEM_SPECIAL))
    {
      QString tmp = victim->equipment[iWear]->name();
      qsizetype x = tmp.length() - dc_strlen(qPrintable(victim->name())) - 1;
      if (x >= 0 && x < tmp.length())
      {
        tmp[x] = '\0';
      }

      tmp = u"%1 %2"_s.arg(tmp).arg(newname);
      victim->equipment[iWear]->name(tmp);
    }
    if (victim->equipment[iWear] && victim->equipment[iWear]->flags_.type_flag == ITEM_CONTAINER)
    {
      for (ObjectPtr obj = victim->equipment[iWear]->contains; obj; obj = obj->next_content)
      {
        if (isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
        {
          QString tmp = obj->name();
          qsizetype x = tmp.length() - dc_strlen(qPrintable(victim->name())) - 1;
          if (x >= 0 && x < tmp.length())
          {
            tmp[x] = '\0';
          }
          tmp = u"%1 %2"_s.arg(tmp).arg(newname);
          obj->name(tmp);
        }
      }
    }
  }

  ObjectPtr obj = victim->carrying;
  while (obj)
  {
    if (isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
    {
      QString tmp = u"%1"_s.arg(obj->name());
      qsizetype x = tmp.length() - dc_strlen(qPrintable(victim->name())) - 1;
      if (x >= 0 && x < tmp.length())
      {
        tmp[x] = '\0';
      }
      tmp = u"%1 %2"_s.arg(tmp).arg(newname);
      obj->name(tmp);
    }
    if (GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
    {
      ObjectPtr obj2;
      for (obj2 = obj->contains; obj2; obj2 = obj2->next_content)
      {
        if (isSet(obj2->flags_.extra_flags, ITEM_SPECIAL))
        {
          QString tmp = u"%1"_s.arg(obj2->name());
          qsizetype x = tmp.length() - dc_strlen(qPrintable(victim->name())) - 1;
          if (x >= 0 && x < tmp.length())
          {
            tmp[x] = '\0';
          }
          tmp = u"%1 %2"_s.arg(tmp).arg(newname);
          obj2->name(tmp);
        }
      }
    }
    obj = obj->next_content;
  }

  auto clan = GET_CLAN(victim);
  auto rights = plr_rights(victim);
  victim->do_outcast(victim->name().split(' '));

  do_fsave(this, qPrintable(victim->name()));

  // Copy the pfile
  QString buffer;
  if (dc_->cf.bport == false)
  {
    buffer = u"cp %1/%2/%3 %4/%5/%6"_s.arg(SAVE_DIR).arg(victim->name()[0]).arg(qPrintable(victim->name())).arg(SAVE_DIR).arg(newname[0]).arg(newname);
  }
  else
  {
    buffer = u"cp %1/%2/%3 %4/%5/%6"_s.arg(BSAVE_DIR).arg(victim->name()[0]).arg(qPrintable(victim->name())).arg(BSAVE_DIR).arg(newname[0]).arg(newname);
  }

  system(qPrintable(buffer));

  struct stat buf = {};

  // Only copy golems if they exist
  for (quint32 i = {}; i < MAX_GOLEMS; i++)
  {
    QString src_filename = u"%s/%c/%s.%d"_s.arg(FAMILIAR_DIR).arg(qPrintable(victim->name())[0]).arg(qPrintable(victim->name())).arg(i);
    if (0 == stat(qPrintable(src_filename), &buf))
    {
      // Make backup
      QString dst_filename = u"%1/%2/%3.%4.old"_s.arg(FAMILIAR_DIR).arg(qPrintable(victim->name())[0]).arg(qPrintable(victim->name())).arg(i);
      QString command = u"cp -f %1 %2"_s.arg(src_filename).arg(dst_filename);
      system(qPrintable(command));

      // Rename
      dst_filename = u"%1/%2/%3.%4"_s.arg(FAMILIAR_DIR).arg(newname[0]).arg(newname).arg(i);
      command = u"mv -f %1 %2"_s.arg(src_filename).arg(dst_filename);
      system(qPrintable(command));
    }
  }

  buffer = u"%1 renamed to %2."_s.arg(qPrintable(victim->name())).arg(newname);
  dc_->logentry(buffer, level_t, DC::LogChannel::LOG_GOD);

  // handle the renames
  dc_->TheAuctionHouse.HandleRename(this, qPrintable(victim->name()), newname);

  // Get rid of the existing one
  do_zap(victim->name().split(' '), cmd_t::TRACK);

  // load the new guy
  do_linkload(newname.split(' '), cmd_t::DEFAULT);

  if (!(victim = get_pc(newname)))
  {
    sendln(u"Major problem...coudn't find target after pfile copied.  Notify Urizen immediatly."_s);
    return ReturnValue::eFAILURE;
  }
  do_name(victim, " %");

  ClanMemberPtr pmember = {};

  if (clan)
  {
    ClanPtr tc = get_clan(clan);
    victim->clan = clan;
    add_clan_member(tc, victim);
    if ((pmember = get_member(victim->name(), clan)))
      pmember->rights(rights);
    add_totem_stats(victim);
  }
  rename_vault_owner(oldname, newname);
  leaderboard.rename(oldname, newname);

  return ReturnValue::eSUCCESS;
}
ReturnValue do_install(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString buf, type, arg1, err, arg2;
  qint32 range = 0, type_ok = 0, numrooms = {};
  qint32 ret;

  /*  if(!ch->has_skill( COMMAND_INSTALL)) {
          ch->sendln(u"Huh?"_s);
          return ReturnValue::eFAILURE;
    }
  */
  half_chop(arg, arg1, buf);
  half_chop(buf, arg2, type);

  if (arg.isEmpty() 1 || type.isEmpty() || arg.isEmpty() 2)
  {
    dc_sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\r\n"
                    "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n");
    ch->send(err);
    return ReturnValue::eFAILURE;
  }

  if (!(range = dc_atoi(arg1)))
  {
    dc_sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\r\n"
                    "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n");
    ch->send(err);
    return ReturnValue::eFAILURE;
  }

  if (range <= 0)
  {
    ch->sendln(u"Range number must be greater than 0"_s);
    return ReturnValue::eFAILURE;
  }

  if (!(numrooms = dc_atoi(arg2)))
  {
    dc_sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\r\n"
                    "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n");
    ch->send(err);
    return ReturnValue::eFAILURE;
  }

  if (numrooms <= 0)
  {
    ch->sendln(u"Number of rooms must be greater than 0."_s);
    return ReturnValue::eFAILURE;
  }

  switch (*type)
  {
  case 'W':
  case 'w':
  case 'O':
  case 'o':
  case 'M':
  case 'm':
  case 'z':
  case 'Z':
  case 'a':
  case 'A':
    type_ok = 1;
    break;
  default:
    type_ok = {};
    break;
  }

  if (type_ok != 1)
  {
    dc_sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\r\n"
                    "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n");
    ch->send(err);
    return ReturnValue::eFAILURE;
  }

  dc_sprintf(buf, "./new_zone %d %d %c true %s", range, numrooms, *type, dc_->cf.bport == true ? "b" : "n");
  ret = system(buf);
  // ret = bits, but I didn't use bits because I'm lazy and it only returns 2 values I gives a flyging fuck about!
  // if you change the script, you gotta change this too. - Rahz

  if (ret == 0)
  {
    dc_sprintf(err, "Range %d Installed!  These changes will not take effect until the next reboot!\r\n", range);
  }
  else if (ret == 256)
  {
    dc_sprintf(err, "That range would overlap another range!\r\n");
  }
  else
  {
    dc_sprintf(err, "Error Code: %d\r\n"
                    "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\r\n"
                    "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n",
               ret);
  }
  ch->send(err);
  return ReturnValue::eSUCCESS;
}

ReturnValue do_range(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (!ch->has_skill(COMMAND_RANGE))
  {
    ch->sendln(u"Huh?"_s);
    return ReturnValue::eFAILURE;
  }

  auto arguments = QString(arg).split(' ');
  QString name = arguments.value(0);
  QString kind = arguments.value(1);
  QString buf = arguments.value(2);
  QString trail = arguments.value(3);
  if (name.isEmpty() || buf.isEmpty() || kind.isEmpty())
  {
    ch->sendln(u"Syntax: range <god> <low vnum> <high vnum>"_s);
    ch->sendln(u"Syntax: range <god> <r/m/o> <low vnum> <high vnum>"_s);
    return ReturnValue::eFAILURE;
  }

  auto victim = get_pc_vis(ch, name);
  if (!victim)
  {
    ch->sendln(u"Set whose range?!"_s);
    return ReturnValue::eFAILURE;
  }

  room_t low, high;
  if (!trail.isEmpty())
  {
    if (!buf[0].isDigit() || !trail[0].isDigit())
    {
      ch->sendln(u"Specify valid numbers. To remove, set the ranges to 0 low and 0 high."_s);
      return ReturnValue::eFAILURE;
    }
    low = buf.toULong();
    high = trail.toULong();
  }
  else
  {
    if (!buf[0].isDigit() || !kind[0].isDigit())
    {
      ch->sendln(u"Specify valid numbers. To remove, set the ranges to 0 low and 0 high."_s);
      return ReturnValue::eFAILURE;
    }
    low = kind.toULong();
    high = buf.toULong();
  }

  if (!trail.isEmpty())
  {
    switch (kind[0].toLower().toLatin1())
    {
    case 'm':
      victim->player->buildMLowVnum = low;
      victim->player->buildMHighVnum = high;
      ch->sendln(u"%1 M range set to %2-%3."_s.arg(victim->name()).arg(low).arg(high));
      victim->sendln(u"Your M range has been set to %1-%2."_s.arg(low).arg(high));
      return ReturnValue::eSUCCESS;
    case 'o':
      victim->player->buildOLowVnum = low;
      victim->player->buildOHighVnum = high;
      ch->sendln(u"%1 O range set to %2-%3."_s.arg(victim->name()).arg(low).arg(high));
      victim->sendln(u"Your O range has been set to %1-%2."_s.arg(low).arg(high));
      return ReturnValue::eSUCCESS;
    case 'r':
      victim->player->buildLowVnum = low;
      victim->player->buildHighVnum = high;
      ch->sendln(u"%1 R range set to %2-%3."_s.arg(victim->name()).arg(low).arg(high));
      victim->sendln(u"Your R range has been set to %1-%2."_s.arg(low).arg(high));
      return ReturnValue::eSUCCESS;
    default:
      ch->sendln(u"Invalid type. Valid ones are r/o/m."_s);
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    victim->player->buildLowVnum = victim->player->buildOLowVnum = victim->player->buildMLowVnum = low;
    victim->player->buildHighVnum = victim->player->buildOHighVnum = victim->player->buildMHighVnum = high;
    ch->sendln(u"%1 range set to %2-%3."_s.arg(victim->name()).arg(low).arg(high));
    victim->send(u"Your range has been set to %1-%2."_s.arg(low).arg(high));
  }
  return ReturnValue::eSUCCESS;
}

ReturnValue do_metastat(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  CharacterPtr victim;
  argument = one_argument(argument, arg);
  if (arg[0] == '\0' || !(victim = get_pc_vis(ch, arg)))
  {
    ch->sendln(u"metastat who?"_s);
    return ReturnValue::eFAILURE;
  }
  QString buf;

  dc_sprintf(buf, "Hps metad: %d Mana metad: %d Moves Metad: %d\r\n",
             GET_HP_METAS(victim), GET_MANA_METAS(victim), GET_MOVE_METAS(ch));
  ch->send(buf);

  dc_sprintf(buf, "Hit points: %d\r\n   Exp spent: %ld\r\n   Plats spent: %ld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
             GET_RAW_HIT(victim), victim->hps_exp_spent(), victim->hps_plats_spent(),
             r_new_meta_platinum_cost(0, victim->hps_plats_spent()) + GET_RAW_HIT(victim) - GET_HP_METAS(victim),
             r_new_meta_exp_cost(0, victim->hps_exp_spent()) + GET_RAW_HIT(victim) - GET_HP_METAS(victim));
  ch->send(buf);

  dc_sprintf(buf, "Mana points: %d\r\n   Exp spent: %ld\r\n   Plats spent: %ld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
             GET_RAW_MANA(victim), victim->mana_exp_spent(), victim->mana_plats_spent(),
             r_new_meta_platinum_cost(0, victim->mana_plats_spent()) + GET_RAW_MANA(victim) - GET_MANA_METAS(victim),
             r_new_meta_exp_cost(0, victim->mana_exp_spent()) + GET_RAW_MANA(victim) - GET_MANA_METAS(victim));
  ch->send(buf);

  dc_sprintf(buf, "Move points: %d\r\n   Exp spent: %ld\r\n   Plats spent: %ld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
             GET_RAW_MOVE(victim), victim->moves_exp_spent(), victim->moves_plats_spent(),
             r_new_meta_platinum_cost(0, victim->moves_plats_spent()) + GET_RAW_MOVE(victim) - GET_MOVE_METAS(victim),
             r_new_meta_exp_cost(0, victim->moves_exp_spent()) + GET_RAW_MOVE(victim) - GET_MOVE_METAS(victim));
  ch->send(buf);

  buf[0] = '\0';
  quint32 l = {};
  for (qsizetype i = {}; i < Commands::commands_.length(); i++)
  {
    if ((l++ % 10) == 0)
      dc_sprintf(buf, "%s\r\n", buf);
    dc_sprintf(buf, "%s%d ", buf, Commands::commands_[i].getNumber());
  }
  ch->send(buf);
  return ReturnValue::eSUCCESS;
}

ReturnValue do_acfinder(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  argument = one_argument(argument, arg);

  if (!arg[0])
  {
    ch->sendln(u"Syntax: acfinder <wear slot>"_s);
    return ReturnValue::eFAILURE;
  }

  qint32 i = 1;
  for (; i < QFlagsToStrings<ObjectPositions>().size(); i++)
    if (QFlagsToStrings<ObjectPositions>().value(i) == QString(arg))
      break;
  if (i >= QFlagsToStrings<ObjectPositions>().size())
  {
    ch->sendln(u"Syntax: acfinder <wear slot>"_s);
    return ReturnValue::eFAILURE;
  }
  i = 1 << i;
  qint32 r, o = 1;
  ObjectPtr obj;
  QString buf;
  for (r = {}; r < top_of_objt; r++)
  {
    obj = dc_->obj_index_[r]->item;
    if (GET_ITEM_TYPE(obj) != ITEM_ARMOR)
      continue;
    if (!CAN_WEAR(obj, i))
      continue;
    qint32 ac = 0 - obj->flags_.value[0];
    for (qint32 z = {}; z < obj->num_affects; z++)
      if (obj->affected[z].location == APPLY_ARMOR)
        ac += obj->affected[z].modifier;
    dc_sprintf(buf, "$B%s%d. %-50s Vnum: %lu AC Apply: %d\r\n$R",
               o % 2 == 0 ? "$2" : "$3", o, qPrintable(obj->short_description()), dc_->obj_index_[r].vnum(), ac);
    ch->send(buf);
    o++;
    if (o == 150)
    {
      ch->sendln(u"Max number of items hit."_s);
      return ReturnValue::eSUCCESS;
    }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValue do_testhit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1, arg2, arg3;
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);

  if (!arg3[0])
  {
    ch->sendln(u"Syntax: <tohit> <level> <target level>"_s);
    return ReturnValue::eFAILURE;
  }
  qint32 toHit = dc_atoi(arg1), tlevel = dc_atoi(arg3), level = dc_atoi(arg2);
  qreal lvldiff = level - tlevel;
  if (lvldiff > 15 && lvldiff < 25)
    toHit += 25;
  else if (lvldiff > 5)
    toHit += 15;
  else if (lvldiff >= 0)
    toHit += 10;
  else if (lvldiff >= -5)
    toHit += 5;

  qreal lvl = (50.0 - level - tlevel / 2.0) / 10.0;

  if (lvl >= 1.0)
    toHit = (qint32)(toHit * lvl);

  for (qint32 AC = 100; AC > -1010; AC -= 10)
  {
    float num1 = 1.0 - (-300.0 - (qreal)AC) * 4.761904762 * 0.0001;
    float num2 = 20.0 + (-300.0 - (qreal)AC) * 0.0095238095;

    qreal percent = 30 + num1 * (qreal)(toHit)-num2;

    ch->send(u"%d AC - %f%% chance to hit\r\n"_s.arg(AC).arg(percent));
  }
  return ReturnValue::eSUCCESS;
}

void write_array_csv(QStringList array, std::ofstream &fout)
{
  qint32 index = {};
  const QString ptr = array[index];
  while (*ptr != '\n')
  {
    fout << ptr << ",";
    ptr = array[++index];
  }
}

void write_array_csv(QStringList names, std::ofstream &fout)
{
  for (const auto &entry : names)
  {
    fout << entry.toStdString() << ",";
  }
}

ReturnValue do_export(CharacterPtr ch, QString args, cmd_t cmd)
{
  QString export_type, filename;
  world_file_list_item *curr = dc_->obj_file_list;

  args = one_argument(args, export_type);
  one_argument(args, filename);

  if (*export_type == 0 || *filename == 0)
  {
    ch->sendln(u"Syntax: export obj <filename>\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  std::ofstream fout;
  fout.exceptions(std::ofstream::failbit | std::ofstream::badbit);

  try
  {
    fout.open(filename, std::ios_base::out);

    fout << "vnum,name,short_description,description,action_description,type,";
    fout << "size,value[0],value[1],value[2],value[3],level,weight,cost,";

    // Print individual array values as columns
    write_array_csv(QFlagsToStrings<ObjectPositions>(), fout);
    write_array_csv(Object::extra_bits, fout);
    write_array_csv(Object::more_obj_bits, fout);

    fout << "affects" << std::endl;

    while (curr)
    {
      for (qint32 x = curr->firstnum; x <= curr->lastnum; x++)
      {
        write_object_csv(dc_->obj_index_[x]->item, fout);
      }
      curr = curr->next;
    }

    fout.close();
  }
  catch (std::ofstream::failure &e)
  {
    std::stringstream errormsg;
    errormsg << "Exception while writing to " << filename << ".";
    dc_->logentry(errormsg.str().c_str(), 108, DC::LogChannel::LOG_MISC);
  }

  dc_->logf(110, DC::LogChannel::LOG_GOD, "Exported objects as %s.", filename);

  return ReturnValue::eSUCCESS;
}

ReturnValue do_world(CharacterPtr ch, QString args, cmd_t cmd)
{

  if (args == "rename")
  {
    auto world = dc_->world_file_list;
    while (world != nullptr)
    {
      QString potential_filename = u"%1-%2.txt"_s.arg(world->firstnum).arg(world->lastnum);
      if (world->filename != potential_filename)
      {
        ch->send(u"filename: %1 firstnum: %2 lastnum: %3 flag: %4\r\n"_s.arg(world->filename).arg(world->firstnum).arg(world->lastnum).arg(world->flags));
        ch->send(u"Renaming %1 to %2\r\n"_s.arg(world->filename).arg(potential_filename));

        if (rename(qPrintable(world->filename), qPrintable(potential_filename)) == -1)
        {
          auto rename_errno = errno;
          QString errStr = strerror(rename_errno);
          if (errStr != nullptr)
          {
            ch->send(u"Error renaming %1 to %2 was %3 %4\r\n"_s.arg(world->filename).arg(potential_filename).arg(rename_errno).arg(errStr));
          }
        }
      }
      world = world->next;
    }
  }

  return ReturnValue::eSUCCESS;
}