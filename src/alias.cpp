/************************************************************************
| $Id: alias.cpp,v 1.8 2011/11/26 03:35:36 jhhudso Exp $
| alias.C
| Description:  Commands for the alias processor.
*/
#include <cstring>

#include "character.h"
#include "utility.h"
#include "levels.h"
#include "player.h"
#include "returnvals.h"
#include "interp.h"

command_return_t Character::do_alias(QStringList arguments, int cmd)
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
      sendln(QString("%2=%3").arg(alias).arg(command));
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
      sendln(QString("Alias '%1' with command '%2' already set.").arg(alias).arg(command));
      return eFAILURE;
    }
    else if (player->aliases_.contains(alias))
    {
      sendln(QString("Alias '%1' with command '%2' replaced with '%3'.").arg(alias).arg(player->aliases_[alias]).arg(command));
    }
    else
    {
      sendln(QString("Alias '%1' defined with command '%2'.").arg(alias).arg(command));
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
      sendln(QString("Removed alias %2=%3").arg(alias).arg(command));
    }

    player->aliases_.clear();
    save();
    return eSUCCESS;
  }

  if (!player->aliases_.contains(arg1))
  {
    sendln(QString("Alias '%1' not found to delete.").arg(arg1));
    return eFAILURE;
  }

  player->aliases_.remove(arg1);
  sendln(QString("Alias '%1' deleted.").arg(arg1));
  save();
  return eFAILURE;
}
