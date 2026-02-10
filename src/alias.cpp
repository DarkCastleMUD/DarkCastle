/************************************************************************
| $Id: alias.cpp,v 1.8 2011/11/26 03:35:36 jhhudso Exp $
| alias.C
| Description:  Commands for the alias processor.
*/
#include <cstring>

#include "DC/character.h"
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/returnvals.h"
#include "DC/interp.h"
#include "DC/db.h"
#include "DC/const.h"

command_return_t Character::do_alias(QStringList arguments, cmd_t cmd)
{
  if (!player)
  {
    return eFAILURE;
  }

  if (arguments.isEmpty())
  {
    if (player->aliases_.isEmpty())
    {
      sendln("No aliases defined.");
      return eSUCCESS;
    }

    auto removed_count = player->aliases_.remove("");
    if (removed_count)
    {
      sendln("Removed an alias with an empty alias.");
    }

    uint64_t x{};
    sendln("Aliases:");
    for (const auto [alias, command] : player->aliases_.asKeyValueRange())
    {
      sendln(QStringLiteral("%2=%3").arg(alias).arg(command));
    }
    return eSUCCESS;
  }

  QString arg1 = arguments.value(0).trimmed();
  QString arg2 = arguments.value(1).trimmed();

  // Alias assignment
  if (arg1.contains("=") || arg2.contains("="))
  {
    auto new_alias_arguments = arguments.join(' ').trimmed().split('=');
    auto alias = new_alias_arguments.value(0).trimmed().toLower();
    auto command = new_alias_arguments.value(1).trimmed();

    if (alias == "alias" || alias == "deleteall")
    {
      sendln("You cannot create a command alias named 'alias' or 'deleteall'.");
      return eFAILURE;
    }

    if (alias.isEmpty())
    {
      sendln("You need to specify an alias.");
      return eFAILURE;
    }

    if (command.isEmpty())
    {
      sendln("You need to specify a command for your alias.");
      return eFAILURE;
    }

    if (player->aliases_.contains(alias) && player->aliases_[alias] == command)
    {
      sendln(QStringLiteral("Alias '%1' with command '%2' already set.").arg(alias).arg(command));
      return eFAILURE;
    }
    else if (player->aliases_.contains(alias))
    {
      sendln(QStringLiteral("Alias '%1' with command '%2' replaced with '%3'.").arg(alias).arg(player->aliases_[alias]).arg(command));
    }
    else
    {
      sendln(QStringLiteral("Alias '%1' defined with command '%2'.").arg(alias).arg(command));
    }
    player->aliases_[alias] = command;
    save();
    return eSUCCESS;
  }

  if (arg1 == "deleteall")
  {
    if (player->aliases_.isEmpty())
    {
      sendln("No aliases defined.");
      return eFAILURE;
    }

    for (const auto [alias, command] : player->aliases_.asKeyValueRange())
    {
      sendln(QStringLiteral("Removed alias %2=%3").arg(alias).arg(command));
    }

    player->aliases_.clear();
    save();
    return eSUCCESS;
  }

  if (!player->aliases_.contains(arg1))
  {
    sendln(QStringLiteral("Alias '%1' not found to delete.").arg(arg1));
    return eFAILURE;
  }

  player->aliases_.remove(arg1);
  sendln(QStringLiteral("Alias '%1' deleted.").arg(arg1));
  save();
  return eFAILURE;
}

QString pet_info(Character *ch, QString type, unsigned victim_count)
{
  unsigned attacks = 1;

  if (ISSET(ch->mobdata->actflags, ACT_2ND_ATTACK))
    attacks++;
  if (ISSET(ch->mobdata->actflags, ACT_3RD_ATTACK))
    attacks++;
  if (ISSET(ch->mobdata->actflags, ACT_4TH_ATTACK))
    attacks++;

  auto bare_damage_str = QStringLiteral("$7$B%1$Rd$7$B%2$R").arg(ch->mobdata->damnodice).arg(ch->mobdata->damsizedice);

  char buffer[MAX_STRING_LENGTH]{};
  sprintbit(ch->affected_by, affected_bits, buffer);
  QString affected_by_str = QString(buffer).trimmed();

  return QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8, $B%9$R [$B$0%10$R] %11")
      .arg(ch->getLevel(), 3)
      .arg(attacks, 3)
      .arg(GET_REAL_HITROLL(ch), 3)
      .arg(GET_REAL_DAMROLL(ch), 3)
      .arg(ch->hit, 5)
      .arg(GET_ARMOR(ch), 4)
      .arg(bare_damage_str, 17)
      .arg(type, 7)
      .arg(ch->short_desc)
      .arg(affected_by_str)
      .arg((victim_count ? "$B$5*$R" : ""));
}

command_return_t Character::do_pets(QStringList arguments, cmd_t cmd)
{
  QString arg1 = arguments.value(0);
  bool arg1_level_ok = false;
  auto arg1_level = arg1.toUInt(&arg1_level_ok);

  QString arg2 = arguments.value(1);
  bool arg2_level_ok = false;
  auto arg2_level = arg2.toUInt(&arg2_level_ok);

  extern int top_of_mobt;
  QMultiMap<level_t, QString> results;

  for (vnum_t vnum = 0; (vnum <= DC::getInstance()->mob_index[top_of_mobt].virt); ++vnum)
  {
    auto nr = real_mobile(vnum);
    if (nr < 0)
      continue;

    auto victim = (Character *)(DC::getInstance()->mob_index[nr].item);
    if ((arg1_level_ok && arg1_level > victim->getLevel()) || (arg2_level_ok && arg2_level > victim->getLevel()))
      continue;

    auto victim_qty = DC::getInstance()->mob_index[nr].qty;
    if (((!arg1.isEmpty() && QStringLiteral("bard").startsWith(arg1, Qt::CaseInsensitive)) || arg1 == "all" || GET_CLASS(this) == CLASS_BARD) && ISSET(victim->mobdata->actflags, ACT_BARDCHARM))
      results.insert(victim->getLevel(), pet_info(victim, QStringLiteral("Bard"), victim_qty));

    if (((!arg1.isEmpty() && QStringLiteral("ranger").startsWith(arg1, Qt::CaseInsensitive)) || arg1 == "all" || GET_CLASS(this) == CLASS_RANGER) && ISSET(victim->mobdata->actflags, ACT_CHARM) && !isSet(victim->immune, ISR_CHARM))
      results.insert(victim->getLevel(), pet_info(victim, QStringLiteral("Ranger"), victim_qty));

    if (((!arg1.isEmpty() && QStringLiteral("antipal").startsWith(arg1, Qt::CaseInsensitive)) || arg1 == "all" || GET_CLASS(this) == CLASS_ANTI_PAL) && (vnum >= 22389 && vnum <= 22398))
      results.insert(victim->getLevel(), pet_info(victim, QStringLiteral("AntiPal"), victim_qty));

    if (((!arg1.isEmpty() && QStringLiteral("cleric").startsWith(arg1, Qt::CaseInsensitive)) || arg1 == "all" || GET_CLASS(this) == CLASS_CLERIC) && (vnum >= 22389 && vnum <= 22398))
      results.insert(victim->getLevel(), pet_info(victim, QStringLiteral("Cleric"), victim_qty));

    if (((!arg1.isEmpty() && QStringLiteral("druid").startsWith(arg1, Qt::CaseInsensitive)) || arg1 == "all" || GET_CLASS(this) == CLASS_DRUID) && (vnum >= 6 && vnum <= 7))
      results.insert(victim->getLevel(), pet_info(victim, QStringLiteral("Druid"), victim_qty));

    if (((!arg1.isEmpty() && QStringLiteral("mage").startsWith(arg1, Qt::CaseInsensitive)) || arg1 == "all" || GET_CLASS(this) == CLASS_MAGE) && (vnum >= 4 && vnum <= 5))
      results.insert(victim->getLevel(), pet_info(victim, QStringLiteral("Mage"), victim_qty));
  }

  if (results.isEmpty())
  {
    sendln(QStringLiteral("No charmable pets found for a %1.").arg(classes[GET_CLASS(this)].name.c_str()));
    sendln("Type 'pets all' to see charmable pets for all classes.");
    sendln("Type 'pets bard' to see charmable pets for bards.");
    sendln("Type 'pets bard 50' or 'pets 50' to only see pets level 50 or above.");
    return eSUCCESS;
  }
  sendln("$B$7LVL,ATK,HIT,DAM,   HP, -AC, dice, class, description$R");
  for (const auto &line : results)
    sendln(line);
  sendln(QStringLiteral("$B$5*$R = in world now"));
  // loop though all npcs

  // search npcs affect of charmable
  // sort by class

  return command_return_t();
}
