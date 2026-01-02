/********************************
| Level 110 wizard commands
| 11/20/95 -- Azrack
**********************/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#include <fmt/format.h>

#include "DC/wizard.h"
#include "DC/utility.h"

#include "DC/player.h"
#include "DC/mobile.h"
#include "DC/interp.h"
#include "DC/fileinfo.h"
#include "DC/clan.h"
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/interp.h"
#include "DC/const.h"
#include "DC/db.h"
#include "DC/Leaderboard.h"
#include "DC/guild.h"
#include "DC/const.h"
#include "DC/vault.h"
#include "DC/meta.h"

int get_max_stat_bonus(Character *ch, int attrs)
{
  int bonus = 0;

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
    bonus = 0;
  }

  return bonus;
}

// List skill maxes.
int do_maxes(Character *ch, char *argument, cmd_t cmd)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  class_skill_defines *classskill;
  argument = one_argument(argument, arg);
  one_argument(argument, arg2);
  int oclass = GET_CLASS(ch); // old class.

  // get_skill_list uses a char argument, and so to keep upkeep
  // at a min I'm just modifying this here.
  int i;
  for (i = 0; pc_clss_types2[i][0] != '\n'; i++)
    if (!str_cmp(pc_clss_types2[i], arg))
      break;
  if (pc_clss_types2[i][0] == '\n')
  {
    ch->sendln("No such class.");
    return eFAILURE;
  }
  GET_CLASS(ch) = i;
  if ((classskill = ch->get_skill_list()) == nullptr)
    return eFAILURE;
  GET_CLASS(ch) = oclass;
  // Same problem with races... get_max_stat(ch, attribute_t::STRENGTH
  for (i = 1; race_types[i][0] != '\n'; i++)
  {
    if (!str_cmp(race_types[i], arg2))
    {
      int orace = GET_RACE(ch);
      GET_RACE(ch) = i;
      for (i = 0; *classskill[i].skillname != '\n'; i++)
      {
        int max = classskill[i].maximum;

        float percent = max * 0.75;
        if (classskill[i].attrs)
        {
          percent += max / 100.0 * get_max_stat_bonus(ch, classskill[i].attrs);
        }
        percent = MIN(max, percent);
        percent = MAX(max * 0.75, percent);
        csendf(ch, "%s: %d\n\r", classskill[i].skillname, (int)percent);
      }
      GET_RACE(ch) = orace;
      return eSUCCESS;
    }
  }
  ch->sendln("No such race.");
  return eFAILURE;
}

// give a command to a god
command_return_t Character::do_bestow(QStringList arguments, cmd_t cmd)
{
  QString name = arguments.value(0);
  QString command = arguments.value(1);

  if (name.isEmpty())
  {
    send_to_char("Bestow gives a god command to a god.\r\n"
                 "Syntax:  bestow <god> <command>\r\n"
                 "Just 'bestow <god>' will list all available commands for that god.\r\n",
                 this);
    return eSUCCESS;
  }

  Character *victim = get_pc_vis(this, name);
  if (!victim)
  {
    this->sendln(QStringLiteral("You don't see anyone named '%1'.").arg(name));
    return eSUCCESS;
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
        this->sendln(QStringLiteral("%1 %2").arg(bgc.name, 22).arg(victim->has_skill(bgc.num) ? "YES" : "---"));
      }
    }

    this->sendln("");
    send_to_char("Test Command           Has command?\r\n"
                 "-----------------------------------\r\n\r\n",
                 this);
    for (const auto &bgc : DC::bestowable_god_commands)
    {
      if (bgc.testcmd)
      {
        this->sendln(QStringLiteral("%1 %2").arg(bgc.name, victim->has_skill(bgc.num) ? "YES" : "---"));
      }
    }

    return eSUCCESS;
  }

  auto bc = get_bestow_command(command);

  if (!bc.has_value())
  {
    this->sendln(QStringLiteral("There is no god command named '%1'.").arg(command));
    return eSUCCESS;
  }

  // if has
  if (victim->has_skill(bc->num))
  {
    this->sendln(QStringLiteral("%1 already has that command.").arg(victim->getName()));
    return eSUCCESS;
  }

  // give it
  victim->learn_skill(bc->num, 1, 1);
  logentry(QStringLiteral("%1 has been bestowed %2 by %3.").arg(GET_NAME(victim)).arg(bc->name).arg(GET_NAME(this)), this->getLevel(), DC::LogChannel::LOG_GOD);
  this->sendln(QStringLiteral("%1 has been bestowed %2.").arg(GET_NAME(victim)).arg(bc->name));
  this->sendln(QStringLiteral("%1 has bestowed %2 upon you.").arg(getName()).arg(bc->name));
  return eSUCCESS;
}

// take away a command from a god
int do_revoke(Character *ch, char *arg, cmd_t cmd)
{
  Character *vict = nullptr;
  char buf[MAX_INPUT_LENGTH];
  char command[MAX_INPUT_LENGTH];
  int i;

  half_chop(arg, arg, command);

  if (!*arg)
  {
    send_to_char("Bestow gives a god command to a god.\r\n"
                 "Syntax:  revoke <god> <command|all>\r\n"
                 "Use 'bestow <god>' to view what commands a god has.\r\n",
                 ch);
    return eSUCCESS;
  }

  if (!(vict = get_pc_vis(ch, arg)))
  {
    sprintf(buf, "You don't see anyone named '%s'.", arg);
    ch->send(buf);
    return eSUCCESS;
  }

  struct char_skill_data *last = nullptr;

  if (!strcmp(command, "all"))
  {
    vict->skills.clear();

    sprintf(buf, "%s has had all comands revoked.\r\n", GET_NAME(vict));
    ch->send(buf);
    sprintf(buf, "%s has had all commands revoked by %s.\r\n", GET_NAME(vict),
            GET_NAME(ch));
    logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    sprintf(buf, "%s has revoked all commands from you.\r\n", GET_NAME(ch));
    vict->send(buf);
    return eSUCCESS;
  }

  for (i = 0; i < DC::bestowable_god_commands.size(); i++)
    if (DC::bestowable_god_commands[i].name == command)
      break;

  if (i == DC::bestowable_god_commands.size())
  {
    sprintf(buf, "There is no god command named '%s'.\r\n", command);
    ch->send(buf);
    return eSUCCESS;
  }

  if (vict->skills.contains(DC::bestowable_god_commands[i].num) == false)
  {
    snprintf(buf, sizeof(buf), "%s does not have %s.\r\n", GET_NAME(vict), DC::bestowable_god_commands[i].name.toStdString().c_str());
    ch->send(buf);
    return eSUCCESS;
  }

  vict->skills.erase(DC::bestowable_god_commands[i].num);
  snprintf(buf, sizeof(buf), "%s has had %s revoked.\r\n", GET_NAME(vict),
           DC::bestowable_god_commands[i].name.toStdString().c_str());
  ch->send(buf);
  snprintf(buf, sizeof(buf), "%s has had %s revoked by %s.", GET_NAME(vict),
           DC::bestowable_god_commands[i].name.toStdString().c_str(), GET_NAME(ch));
  logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
  snprintf(buf, sizeof(buf), "%s has revoked %s from you.\r\n", GET_NAME(ch),
           DC::bestowable_god_commands[i].name.toStdString().c_str());
  vict->send(buf);
  return eSUCCESS;
}

/* Thunder is currently in wiz_104.c */

int do_wizlock(Character *ch, char *argument, cmd_t cmd)
{
  wizlock = !wizlock;

  if (wizlock)
  {
    char log_buf[MAX_STRING_LENGTH] = {};
    sprintf(log_buf, "Game has been wizlocked by %s.", GET_NAME(ch));
    logentry(log_buf, ANGEL, DC::LogChannel::LOG_GOD);
    ch->sendln("Game wizlocked.");
  }
  else
  {
    char log_buf[MAX_STRING_LENGTH] = {};
    sprintf(log_buf, "Game has been un-wizlocked by %s.", GET_NAME(ch));
    logentry(log_buf, ANGEL, DC::LogChannel::LOG_GOD);
    ch->sendln("Game un-wizlocked.");
  }
  return eSUCCESS;
}

/************************************************************************
| do_chpwd
| Precondition: ch != 0, arg != 0
| Postcondition: ch->password is arg
| Side effects: None
| Returns: None
*/
int do_chpwd(Character *ch, char *arg, cmd_t cmd)
{
  Character *victim;
  char name[100], buf[50];

  /* Verify preconditions */
  assert(ch != 0);
  if (arg == 0)
    return eFAILURE;

  half_chop(arg, name, buf);

  if (!*name)
  {
    ch->sendln("Change whose password?");
    return eFAILURE;
  }

  if (!(victim = get_pc_vis(ch, name)))
  {
    ch->sendln("That player was not found.");
    return eFAILURE;
  }

  one_argument(buf, name);

  if (!*name || strlen(name) > 10)
  {
    ch->sendln("Password must be 10 characters or less.");
    return eFAILURE;
  }

  strncpy(victim->player->pwd, (char *)crypt((char *)name, (char *)victim->getNameC()), PASSWORD_LEN);
  victim->player->pwd[PASSWORD_LEN] = '\0';

  ch->sendln("Ok.");
  return eSUCCESS;
}

int do_fakelog(Character *ch, char *argument, cmd_t cmd)
{
  char command[MAX_INPUT_LENGTH];
  char lev_str[MAX_INPUT_LENGTH];
  uint64_t lev_nr = 110;

  if (IS_NPC(ch))
    return eFAILURE;

  half_chop(argument, lev_str, command);

  if (!*lev_str)
  {
    ch->sendln("Also, you must supply a level.");
    return eFAILURE;
  }

  if (isdigit(*lev_str))
  {
    lev_nr = atoi(lev_str);
    if (lev_nr < IMMORTAL || lev_nr > IMPLEMENTER)
    {
      ch->sendln("You must use a valid level from 100-110.");
      return eFAILURE;
    }
  }

  send_to_gods(command, lev_nr, DC::LogChannel::LOG_BUG);
  return eSUCCESS;
}

command_return_t Character::do_rename_char(QStringList arguments, cmd_t cmd)
{
  if (arguments.size() < 2)
  {
    send("Usage: rename <oldname> <newname> [takeplats]\n\r");
    return eFAILURE;
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

  Character *victim = get_pc(oldname);
  if (!victim)
  {
    send(QStringLiteral("%1 is not in the game.\r\n").arg(oldname));
    return eFAILURE;
  }

  if (level_ <= victim->getLevel())
  {
    send("You can't rename someone your level or higher.\r\n");
    send(QStringLiteral("%1 just tried to rename you.\r\n").arg(GET_NAME(this)));
    return eFAILURE;
  }

  // +1 cause you can actually have 13 char names
  if (newname.length() > (MAX_NAME_LENGTH + 1))
  {
    send(QStringLiteral("New name too long. Maximum allowed length is %1 characters.\r\n").arg(MAX_NAME_LENGTH + 1));
    return eFAILURE;
  }

  QString arg3 = arguments.value(2);
  if (arg3 == "takeplats")
  {
    if (GET_PLATINUM(victim) < 500)
    {
      send(QStringLiteral("They don't have enough plats. They need 500 but have %1\r\n").arg(GET_PLATINUM(victim)));
      return eFAILURE;
    }
    else
    {
      GET_PLATINUM(victim) -= 500;
      send(QStringLiteral("You reach into %1's soul and remove 500 platinum leaving them %2 platinum.\r\n").arg(GET_SHORT(victim)).arg(GET_PLATINUM(victim)));
      victim->send(QStringLiteral("You feel the hand of god slip into your soul and remove 500 platinum leaving you %1 platinum.\r\n").arg(GET_PLATINUM(victim)));
      logentry(QStringLiteral("500 platinum removed from %1 for rename.").arg(victim->getNameC()), level_, DC::LogChannel::LOG_GOD);
    }
  }

  QString strsave;
  if (DC::getInstance()->cf.bport == false)
  {
    strsave = QStringLiteral("%1/%2/%3").arg(SAVE_DIR).arg(newname[0]).arg(newname);
  }
  else
  {
    strsave = QStringLiteral("%1/%2/%3").arg(BSAVE_DIR).arg(newname[0]).arg(newname);
  }

  if (QFile(strsave).exists())
  {
    send(QStringLiteral("The name '%1' is already in use at %2.\r\n").arg(newname).arg(strsave));
    return eFAILURE;
  }

  for (unsigned iWear = 0; iWear < MAX_WEAR; iWear++)
  {
    if (victim->equipment[iWear] &&
        isSet(victim->equipment[iWear]->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      QString tmp = victim->equipment[iWear]->Name();
      qsizetype x = tmp.length() - strlen(victim->getNameC()) - 1;
      if (x >= 0 && x < tmp.length())
      {
        tmp[x] = '\0';
      }

      tmp = QStringLiteral("%1 %2").arg(tmp).arg(newname);
      victim->equipment[iWear]->Name(tmp);
    }
    if (victim->equipment[iWear] && victim->equipment[iWear]->obj_flags.type_flag == ITEM_CONTAINER)
    {
      for (Object *obj = victim->equipment[iWear]->contains; obj; obj = obj->next_content)
      {
        if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
        {
          QString tmp = obj->Name();
          qsizetype x = tmp.length() - strlen(victim->getNameC()) - 1;
          if (x >= 0 && x < tmp.length())
          {
            tmp[x] = '\0';
          }
          tmp = QStringLiteral("%1 %2").arg(tmp).arg(newname);
          obj->Name(tmp);
        }
      }
    }
  }

  Object *obj = victim->carrying;
  while (obj)
  {
    if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      QString tmp = QStringLiteral("%1").arg(obj->Name());
      qsizetype x = tmp.length() - strlen(victim->getNameC()) - 1;
      if (x >= 0 && x < tmp.length())
      {
        tmp[x] = '\0';
      }
      tmp = QStringLiteral("%1 %2").arg(tmp).arg(newname);
      obj->Name(tmp);
    }
    if (GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
    {
      Object *obj2;
      for (obj2 = obj->contains; obj2; obj2 = obj2->next_content)
      {
        if (isSet(obj2->obj_flags.extra_flags, ITEM_SPECIAL))
        {
          QString tmp = QStringLiteral("%1").arg(obj2->Name());
          qsizetype x = tmp.length() - strlen(victim->getNameC()) - 1;
          if (x >= 0 && x < tmp.length())
          {
            tmp[x] = '\0';
          }
          tmp = QStringLiteral("%1 %2").arg(tmp).arg(newname);
          obj2->Name(tmp);
        }
      }
    }
    obj = obj->next_content;
  }

  auto clan = GET_CLAN(victim);
  auto rights = plr_rights(victim);
  victim->do_outcast(victim->getName().split(' '));

  do_fsave(this, victim->getNameC());

  // Copy the pfile
  QString buffer;
  if (DC::getInstance()->cf.bport == false)
  {
    buffer = QStringLiteral("cp %1/%2/%3 %4/%5/%6").arg(SAVE_DIR).arg(victim->getName()[0]).arg(victim->getNameC()).arg(SAVE_DIR).arg(newname[0]).arg(newname);
  }
  else
  {
    buffer = QStringLiteral("cp %1/%2/%3 %4/%5/%6").arg(BSAVE_DIR).arg(victim->getName()[0]).arg(victim->getNameC()).arg(BSAVE_DIR).arg(newname[0]).arg(newname);
  }

  system(buffer.toStdString().c_str());

  struct stat buf = {};

  // Only copy golems if they exist
  for (unsigned i = 0; i < MAX_GOLEMS; i++)
  {
    QString src_filename = QStringLiteral("%s/%c/%s.%d").arg(FAMILIAR_DIR).arg(victim->getNameC()[0]).arg(victim->getNameC()).arg(i);
    if (0 == stat(src_filename.toStdString().c_str(), &buf))
    {
      // Make backup
      QString dst_filename = QStringLiteral("%1/%2/%3.%4.old").arg(FAMILIAR_DIR).arg(victim->getNameC()[0]).arg(victim->getNameC()).arg(i);
      QString command = QStringLiteral("cp -f %1 %2").arg(src_filename).arg(dst_filename);
      system(command.toStdString().c_str());

      // Rename
      dst_filename = QStringLiteral("%1/%2/%3.%4").arg(FAMILIAR_DIR).arg(newname[0]).arg(newname).arg(i);
      command = QStringLiteral("mv -f %1 %2").arg(src_filename).arg(dst_filename);
      system(command.toStdString().c_str());
    }
  }

  buffer = QStringLiteral("%1 renamed to %2.").arg(victim->getNameC()).arg(newname);
  logentry(buffer, level_, DC::LogChannel::LOG_GOD);

  // handle the renames
  DC::getInstance()->TheAuctionHouse.HandleRename(this, victim->getNameC(), newname);

  // Get rid of the existing one
  do_zap(victim->getName().split(' '), cmd_t::TRACK);

  // load the new guy
  do_linkload(newname.split(' '), cmd_t::DEFAULT);

  if (!(victim = get_pc(newname)))
  {
    this->sendln("Major problem...coudn't find target after pfile copied.  Notify Urizen immediatly.");
    return eFAILURE;
  }
  do_name(victim, " %");

  ClanMember *pmember = nullptr;

  if (clan)
  {
    clan_data *tc = get_clan(clan);
    victim->clan = clan;
    add_clan_member(tc, victim);
    if ((pmember = get_member(victim->getName(), this->clan)))
      pmember->Rights(rights);
    add_totem_stats(victim);
  }
  rename_vault_owner(oldname, newname);
  leaderboard.rename(oldname, newname);

  return eSUCCESS;
}
int do_install(Character *ch, char *arg, cmd_t cmd)
{
  char buf[256], type[256], arg1[256], err[256], arg2[256];
  int range = 0, type_ok = 0, numrooms = 0;
  int ret;

  /*  if(!ch->has_skill( COMMAND_INSTALL)) {
          ch->sendln("Huh?");
          return eFAILURE;
    }
  */
  half_chop(arg, arg1, buf);
  half_chop(buf, arg2, type);

  if (!*arg1 || !*type || !*arg2)
  {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n");
    ch->send(err);
    return eFAILURE;
  }

  if (!(range = atoi(arg1)))
  {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n");
    ch->send(err);
    return eFAILURE;
  }

  if (range <= 0)
  {
    ch->sendln("Range number must be greater than 0");
    return eFAILURE;
  }

  if (!(numrooms = atoi(arg2)))
  {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n");
    ch->send(err);
    return eFAILURE;
  }

  if (numrooms <= 0)
  {
    ch->sendln("Number of rooms must be greater than 0.");
    return eFAILURE;
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
    type_ok = 0;
    break;
  }

  if (type_ok != 1)
  {
    sprintf(err, "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n");
    ch->send(err);
    return eFAILURE;
  }

  sprintf(buf, "./new_zone %d %d %c true %s", range, numrooms, *type, DC::getInstance()->cf.bport == true ? "b" : "n");
  ret = system(buf);
  // ret = bits, but I didn't use bits because I'm lazy and it only returns 2 values I gives a flyging fuck about!
  // if you change the script, you gotta change this too. - Rahz

  if (ret == 0)
  {
    sprintf(err, "Range %d Installed!  These changes will not take effect until the next reboot!\r\n", range);
  }
  else if (ret == 256)
  {
    sprintf(err, "That range would overlap another range!\r\n");
  }
  else
  {
    sprintf(err, "Error Code: %d\r\n"
                 "Usage: install <range #> <# of rooms> <world|obj|mob|zone|all>\n\r"
                 "  ie.. install 29100 100 m = installs mob range 29100-29199.\r\n",
            ret);
  }
  ch->send(err);
  return eSUCCESS;
}

int do_range(Character *ch, char *arg, cmd_t cmd)
{
  Character *victim;
  char name[160], buf[160];
  char kind[160];
  char trail[160];
  char message[256];

  /*



  */
  if (!ch->has_skill(COMMAND_RANGE))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  arg = one_argument(arg, name);
  arg = one_argument(arg, kind);
  arg = one_argument(arg, buf);
  arg = one_argument(arg, trail);

  if (!(*name) || !(*buf) || !(*kind))
  {
    ch->sendln("Syntax: range <god> <low vnum> <high vnum>");
    ch->sendln("Syntax: range <god> <r/m/o> <low vnum> <high vnum>");
    return eFAILURE;
  }

  if (!(victim = get_pc_vis(ch, name)))
  {
    ch->sendln("Set whose range?!");
    return eFAILURE;
  }
  int low, high;
  if (trail[0] != '\0')
  {
    if (!isdigit(*buf) || !isdigit(*trail))
    {
      ch->sendln("Specify valid numbers. To remove, set the ranges to 0 low and 0 high.");
      return eFAILURE;
    }
    low = atoi(buf);
    high = atoi(trail);
  }
  else
  {
    if (!isdigit(*buf) || !isdigit(*kind))
    {
      ch->sendln("Specify valid numbers. To remove, set the ranges to 0 low and 0 high.");
      return eFAILURE;
    }
    low = atoi(kind);
    high = atoi(buf);
  }
  if (low < 0 || high < 0)
  {
    ch->sendln("The number needs to be positive.");
    return eFAILURE;
  }
  if (trail[0])
  {
    switch (LOWER(kind[0]))
    {
    case 'm':
      victim->player->buildMLowVnum = low;
      victim->player->buildMHighVnum = high;
      sprintf(message, "%s M range set to %d-%d.\r\n", victim->getNameC(), low, high);
      ch->send(message);
      sprintf(message, "Your M range has been set to %d-%d.\r\n", low, high);
      victim->send(message);
      return eSUCCESS;
    case 'o':
      victim->player->buildOLowVnum = low;
      victim->player->buildOHighVnum = high;
      sprintf(message, "%s O range set to %d-%d.\r\n", victim->getNameC(), low, high);
      ch->send(message);
      sprintf(message, "Your O range has been set to %d-%d.\r\n", low, high);
      victim->send(message);
      return eSUCCESS;
    case 'r':
      victim->player->buildLowVnum = low;
      victim->player->buildHighVnum = high;
      sprintf(message, "%s R range set to %d-%d.\r\n", victim->getNameC(), low, high);
      ch->send(message);
      sprintf(message, "Your R range has been set to %d-%d.\r\n", low, high);
      victim->send(message);
      return eSUCCESS;
    default:
      ch->sendln("Invalid type. Valid ones are r/o/m.");
      return eFAILURE;
    }
  }
  else
  {
    victim->player->buildLowVnum = victim->player->buildOLowVnum = victim->player->buildMLowVnum = low;
    victim->player->buildHighVnum = victim->player->buildOHighVnum = victim->player->buildMHighVnum = high;
    sprintf(message, "%s range set to %d-%d.\r\n", victim->getNameC(), low, high);
    ch->send(message);
    sprintf(message, "Your range has been set to %d-%d.\r\n", low, high);
    victim->send(message);
  }
  return eSUCCESS;
}

int do_metastat(Character *ch, char *argument, cmd_t cmd)
{
  char arg[MAX_INPUT_LENGTH];
  Character *victim;
  argument = one_argument(argument, arg);
  if (arg[0] == '\0' || !(victim = get_pc_vis(ch, arg)))
  {
    ch->sendln("metastat who?");
    return eFAILURE;
  }
  char buf[MAX_STRING_LENGTH];

  sprintf(buf, "Hps metad: %d Mana metad: %d Moves Metad: %d\r\n",
          GET_HP_METAS(victim), GET_MANA_METAS(victim), GET_MOVE_METAS(ch));
  ch->send(buf);

  sprintf(buf, "Hit points: %d\r\n   Exp spent: %ld\r\n   Plats spent: %ld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
          GET_RAW_HIT(victim), victim->hps_exp_spent(), victim->hps_plats_spent(),
          r_new_meta_platinum_cost(0, victim->hps_plats_spent()) + GET_RAW_HIT(victim) - GET_HP_METAS(victim),
          r_new_meta_exp_cost(0, victim->hps_exp_spent()) + GET_RAW_HIT(victim) - GET_HP_METAS(victim));
  ch->send(buf);

  sprintf(buf, "Mana points: %d\r\n   Exp spent: %ld\r\n   Plats spent: %ld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
          GET_RAW_MANA(victim), victim->mana_exp_spent(), victim->mana_plats_spent(),
          r_new_meta_platinum_cost(0, victim->mana_plats_spent()) + GET_RAW_MANA(victim) - GET_MANA_METAS(victim),
          r_new_meta_exp_cost(0, victim->mana_exp_spent()) + GET_RAW_MANA(victim) - GET_MANA_METAS(victim));
  ch->send(buf);

  sprintf(buf, "Move points: %d\r\n   Exp spent: %ld\r\n   Plats spent: %ld\r\n   Plats enough for: %d\r\n   Exp enough for: %d\r\n",
          GET_RAW_MOVE(victim), victim->moves_exp_spent(), victim->moves_plats_spent(),
          r_new_meta_platinum_cost(0, victim->moves_plats_spent()) + GET_RAW_MOVE(victim) - GET_MOVE_METAS(victim),
          r_new_meta_exp_cost(0, victim->moves_exp_spent()) + GET_RAW_MOVE(victim) - GET_MOVE_METAS(victim));
  ch->send(buf);

  buf[0] = '\0';
  unsigned int i = 1, l = 0;
  for (auto i = 0; i < Commands::commands_.length(); i++)
  {
    if ((l++ % 10) == 0)
      sprintf(buf, "%s\r\n", buf);
    sprintf(buf, "%s%d ", buf, Commands::commands_[i].getNumber());
  }
  ch->send(buf);
  return eSUCCESS;
}

int do_acfinder(Character *ch, char *argument, cmd_t cmd)
{
  char arg[MAX_STRING_LENGTH];
  argument = one_argument(argument, arg);

  if (!arg[0])
  {
    ch->sendln("Syntax: acfinder <wear slot>");
    return eFAILURE;
  }

  int i = 1;
  for (; i < QFlagsToStrings<ObjectPositions>().size(); i++)
    if (QFlagsToStrings<ObjectPositions>().value(i) == QString(arg))
      break;
  if (i >= QFlagsToStrings<ObjectPositions>().size())
  {
    ch->sendln("Syntax: acfinder <wear slot>");
    return eFAILURE;
  }
  i = 1 << i;
  int r, o = 1;
  Object *obj;
  char buf[MAX_STRING_LENGTH];
  for (r = 0; r < top_of_objt; r++)
  {
    obj = (Object *)DC::getInstance()->obj_index[r].item;
    if (GET_ITEM_TYPE(obj) != ITEM_ARMOR)
      continue;
    if (!CAN_WEAR(obj, i))
      continue;
    int ac = 0 - obj->obj_flags.value[0];
    for (qsizetype z = 0; z < obj->affected.size(); z++)
      if (obj->affected[z].location == APPLY_ARMOR)
        ac += obj->affected[z].modifier;
    sprintf(buf, "$B%s%d. %-50s Vnum: %d AC Apply: %d\r\n$R",
            o % 2 == 0 ? "$2" : "$3", o, obj->short_description, DC::getInstance()->obj_index[r].virt, ac);
    ch->send(buf);
    o++;
    if (o == 150)
    {
      ch->sendln("Max number of items hit.");
      return eSUCCESS;
    }
  }
  return eSUCCESS;
}

int do_testhit(Character *ch, char *argument, cmd_t cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);

  if (!arg3[0])
  {
    ch->sendln("Syntax: <tohit> <level> <target level>");
    return eFAILURE;
  }
  int toHit = atoi(arg1), tlevel = atoi(arg3), level = atoi(arg2);
  float lvldiff = level - tlevel;
  if (lvldiff > 15 && lvldiff < 25)
    toHit += 25;
  else if (lvldiff > 5)
    toHit += 15;
  else if (lvldiff >= 0)
    toHit += 10;
  else if (lvldiff >= -5)
    toHit += 5;

  float lvl = (50.0 - level - tlevel / 2.0) / 10.0;

  if (lvl >= 1.0)
    toHit = (int)(toHit * lvl);

  for (int AC = 100; AC > -1010; AC -= 10)
  {
    float num1 = 1.0 - (-300.0 - (float)AC) * 4.761904762 * 0.0001;
    float num2 = 20.0 + (-300.0 - (float)AC) * 0.0095238095;

    float percent = 30 + num1 * (float)(toHit)-num2;

    csendf(ch, "%d AC - %f%% chance to hit\r\n", AC,
           percent);
  }
  return eSUCCESS;
}

void write_array_csv(const char *const *array, std::ofstream &fout)
{
  int index = 0;
  const char *ptr = array[index];
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

int do_export(Character *ch, char *args, cmd_t cmd)
{
  char export_type[MAX_INPUT_LENGTH], filename[MAX_INPUT_LENGTH];
  world_file_list_item *curr = DC::getInstance()->obj_file_list;

  args = one_argument(args, export_type);
  one_argument(args, filename);

  if (*export_type == 0 || *filename == 0)
  {
    ch->sendln("Syntax: export obj <filename>\n\r");
    return eFAILURE;
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
      for (int x = curr->firstnum; x <= curr->lastnum; x++)
      {
        write_object_csv((Object *)DC::getInstance()->obj_index[x].item, fout);
      }
      curr = curr->next;
    }

    fout.close();
  }
  catch (std::ofstream::failure &e)
  {
    std::stringstream errormsg;
    errormsg << "Exception while writing to " << filename << ".";
    logmisc(errormsg.str().c_str());
  }

  logf(110, DC::LogChannel::LOG_GOD, "Exported objects as %s.", filename);

  return eSUCCESS;
}

command_return_t do_world(Character *ch, std::string args, cmd_t cmd)
{

  if (args == "rename")
  {
    auto world = DC::getInstance()->world_file_list;
    while (world != nullptr)
    {
      QString potential_filename = QStringLiteral("%1-%2.txt").arg(world->firstnum).arg(world->lastnum);
      if (world->filename != potential_filename)
      {
        ch->send(QStringLiteral("filename: %1 firstnum: %2 lastnum: %3 flag: %4\r\n").arg(world->filename).arg(world->firstnum).arg(world->lastnum).arg(world->flags));
        ch->send(QStringLiteral("Renaming %1 to %2\r\n").arg(world->filename).arg(potential_filename));

        if (rename(world->filename.toStdString().c_str(), potential_filename.toStdString().c_str()) == -1)
        {
          auto rename_errno = errno;
          char *errStr = strerror(rename_errno);
          if (errStr != nullptr)
          {
            ch->send(QStringLiteral("Error renaming %1 to %2 was %3 %4\r\n").arg(world->filename).arg(potential_filename).arg(rename_errno).arg(errStr));
          }
        }
      }
      world = world->next;
    }
  }

  return eSUCCESS;
}