/********************************
| Level 106 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/DC.h"
#include "DC/interp.h"
#include <fmt/format.h>

command_return_t do_plats(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr i;
  class Connection *d;
  QString arg;
  QString buf;
  qint32 minamt;

  one_argument(argument, arg);
  if (*arg)
    minamt = atoi(arg);
  else
    minamt = 1;

  ch->sendln("          Plats - Player");
  ch->sendln("          --------------");

  for (const auto &d : DC::getInstance()->connections_)
  {
    if ((conn->connected) || (!CAN_SEE(ch, conn->character)))
      continue;

    if (conn->original)
      i = conn->original;
    else
      i = conn->character;

    if (GET_PLATINUM(i) < (quint32)minamt)
      continue;

    dc_sprintf(buf, "%15d - %s - %ld - %d\r\n", GET_PLATINUM(i), qPrintable(i->name()), i->getGold(), GET_BANK(i));
    ch->send(buf);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_force(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
  {
    return ReturnValue::eFAILURE;
  }

  if (!has_skill(COMMAND_FORCE) && cmd != cmd_t::FORCE)
  {
    sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  auto name = arguments.value(0);
  auto to_force = arguments.value(1);
  auto vict = get_char_vis(this, name);

  if (name.isEmpty() || to_force.isEmpty())
  {
    sendln("Who do you wish to force to do what?");
    return ReturnValue::eFAILURE;
  }
  else if (name != "all")
  {
    if (!vict)
      sendln("No one by that name here..");
    else
    {
      if (getLevel() < vict->getLevel() && vict->isNonPlayer())
      {
        sendln("Now doing that would just tick off the IMPS!");
        DC::getInstance()->logentry(QStringLiteral("%1 just tried to force %2 to %3").arg(name()).arg(vict->name()).arg(to_force), OVERSEER, DC::LogChannel::LOG_GOD);
        return ReturnValue::eSUCCESS;
      }
      if ((getLevel() <= vict->getLevel()) && vict->isPlayer())
      {
        sendln("Why be forceful?");
        buf = fmt::format("$n has failed to force you to '{}'.", to_force);
        act(buf, ch, 0, vict, TO_VICT, 0);
      }
      else
      {
        if (player->stealth == false)
        {
          buf = fmt::format("$n has forced you to '{}'.", to_force);
          act(buf, ch, 0, vict, TO_VICT, 0);
          sendln("Ok.");
        }
        buf = fmt::format("{} just forced %s to %s.", qPrintable(name()),
                          qPrintable(vict->name()), to_force);
        vict->command_interpreter(to_force.c_str());
        DC::getInstance()->logentry(buf.c_str(), getLevel(), DC::LogChannel::LOG_GOD);
      }
    }
  }

  else
  { /* force all */
    if (getLevel() < OVERSEER)
    {
      sendln("Not gonna happen.");
      return ReturnValue::eFAILURE;
    }
    for (i = DC::getInstance()->connections_; i; i = next_i)
    {
      next_i = i->next;
      if (i->character != ch && !i->connected)
      {
        vict = i->character;
        if (getLevel() <= vict->getLevel())
          continue;
        else
        {
          if (player->stealth == false || getLevel() < 109)
          {
            buf = fmt::format("$n has forced you to '{}'.", to_force);
            act(buf, ch, 0, vict, TO_VICT, 0);
          }
          vict->command_interpreter(to_force.c_str());
        }
      }
    }
    sendln("Ok.");
    buf = fmt::format("{} just forced all to {}.", qPrintable(name()), to_force);
    DC::getInstance()->logentry(buf.c_str(), getLevel(), DC::LogChannel::LOG_GOD);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t run_all_events(CharacterPtr ch = {})
{
  quint64 counter = {};
  while (timer_list != nullptr && counter++ < 1000)
  {
    if (ch)
    {
      ch->send("Running check_timer()\r\n");
      ch->desc->process_output();
    }
    check_timer();
    ch->desc->process_output();
  }
  if (counter >= 1000)
  {
    return ReturnValue::eFAILURE;
  }
  return ReturnValue::eSUCCESS;
}

QString rc_to_qstring(const command_return_t &rc)
{
  QStringList strings;
  if (isSet(rc, ReturnValue::eFAILURE))
  {
    strings += "ReturnValue::eFAILURE";
  }
  if (isSet(rc, ReturnValue::eSUCCESS))
  {
    strings += "ReturnValue::eSUCCESS ";
  }
  if (isSet(rc, ReturnValue::eCH_DIED))
  {
    strings += "ReturnValue::eCH_DIED ";
  }
  if (isSet(rc, ReturnValue::eDELAYED_EXEC))
  {
    strings += "ReturnValue::eDELAYED_EXEC ";
  }
  if (isSet(rc, ReturnValue::eEXTRA_VAL2))
  {
    strings += "ReturnValue::eEXTRA_VAL2 ";
  }
  if (isSet(rc, ReturnValue::eEXTRA_VALUE))
  {
    strings += "ReturnValue::eEXTRA_VALUE ";
  }
  if (isSet(rc, ReturnValue::eIMMUNE_VICTIM))
  {
    strings += "ReturnValue::eIMMUNE_VICTIM ";
  }
  if (isSet(rc, ReturnValue::eINTERNAL_ERROR))
  {
    strings += "ReturnValue::eINTERNAL_ERROR ";
  }
  if (isSet(rc, ReturnValue::eVICT_DIED))
  {
    strings += "ReturnValue::eVICT_DIED ";
  }

  return strings.join(',');
}

void run_check(CharacterPtr ch, command_return_t *rc, auto *function, QString arguments = {}, cmd_t cmd = cmd_t::DEFAULT)
{
  command_return_t new_rc = {};
  if (ch)
  {
    if (function)
    {
      new_rc = function(ch, arguments, cmd);
    }
    ch->send(QStringLiteral("Return code is %1 (%2)\r\n").arg(new_rc).arg(rc_to_qstring(new_rc)));
    if (ch->desc)
    {
      ch->desc->process_output();
    }
  }

  if (rc)
  {
    *rc = *rc | new_rc;
  }
}

void run_check(CharacterPtr ch, command_return_t *rc, command_gen2_t function, QString arguments = QString(), cmd_t cmd = cmd_t::DEFAULT)
{
  command_return_t new_rc = {};
  if (ch)
  {
    if (function)
    {
      new_rc = function(ch, arguments, cmd);
    }
    ch->send(QStringLiteral("Return code is %1 (%2)\r\n").arg(new_rc).arg(rc_to_qstring(new_rc)));
    if (ch->desc)
    {
      ch->desc->process_output();
    }
  }

  if (rc)
  {
    *rc = *rc | new_rc;
  }
}

void run_check(CharacterPtr ch, command_return_t *rc, command_gen3_t function, QStringList arguments = QStringList(), cmd_t cmd = cmd_t::DEFAULT)
{
  command_return_t new_rc = {};
  if (ch)
  {
    if (function)
    {
      new_rc = (*ch.*(function))(arguments, cmd);
    }
    ch->send(QStringLiteral("Return code is %1 (%2)\r\n").arg(new_rc).arg(rc_to_qstring(new_rc)));
    if (ch->desc)
    {
      ch->desc->process_output();
    }
  }

  if (rc)
  {
    *rc = *rc | new_rc;
  }
}

void run_check(CharacterPtr ch, command_return_t *rc, command_special_t function, QString arguments = "", cmd_t cmd = cmd_t::DEFAULT)
{
  command_return_t new_rc = {};
  if (ch)
  {
    if (function)
    {
      new_rc = (*ch.*(function))(arguments, cmd);
    }
    ch->send(QStringLiteral("Return code is %1 (%2)\r\n").arg(new_rc).arg(rc_to_qstring(new_rc)));
    if (ch->desc)
    {
      ch->desc->process_output();
    }
  }

  if (rc)
  {
    *rc = *rc | new_rc;
  }
}

command_return_t test_casino(CharacterPtr ch)
{
  if (!ch || !ch->player)
  {
    return ReturnValue::eFAILURE;
  }

  qint32 max_rc = {};

  run_check(ch, &max_rc, &Character::do_goto, {"1"});
  run_check(ch, &max_rc, &Character::do_goto, {"21900"});

  run_check(ch, &max_rc, do_look, QStringLiteral("sign"), cmd_t::LOOK);
  run_check(ch, &max_rc, do_examine, QStringLiteral("sign"), cmd_t::EXAMINE);

  run_check(ch, &max_rc, do_move, QStringLiteral(""), cmd_t::NORTH);

  run_check(ch, &max_rc, do_look, QStringLiteral("fountain"), cmd_t::LOOK);
  run_check(ch, &max_rc, do_examine, QStringLiteral("fountain"), cmd_t::EXAMINE);
  run_check(ch, &max_rc, &Character::do_drink, {"fountain"});

  run_check(ch, &max_rc, do_look, QStringLiteral("machine"), cmd_t::LOOK);
  run_check(ch, &max_rc, do_examine, QStringLiteral("machine"), cmd_t::EXAMINE);

  // saving the player's finances and giving them a consistent
  // amount of cash in bank to be withdrawn
  auto original_bank = ch->player->bank;
  ch->player->bank = 1000000;
  auto original_gold = ch->getGold();
  ch->setGold(0);

  // These are special code procedures that should be on the objects in this room
  run_check(ch, &max_rc, &Character::special, "", cmd_t::BALANCE);
  run_check(ch, &max_rc, &Character::special, "", cmd_t::WITHDRAW);
  run_check(ch, &max_rc, &Character::special, "1000000", cmd_t::WITHDRAW);

  run_check(ch, &max_rc, do_move, QStringLiteral(""), cmd_t::NORTH);

  run_check(ch, &max_rc, do_look, QStringLiteral("table"), cmd_t::LOOK);
  run_check(ch, &max_rc, do_examine, QStringLiteral("table"), cmd_t::EXAMINE);

  auto original_random = DC::getInstance()->random_;
  DC::getInstance()->random_ = QRandomGenerator(1);

  run_check(ch, &max_rc, &Character::special, "", cmd_t::BET);
  run_check(ch, &max_rc, &Character::special, "-1", cmd_t::BET);
  run_check(ch, &max_rc, &Character::special, "0", cmd_t::BET);
  run_check(ch, &max_rc, &Character::special, "1", cmd_t::BET);
  run_check(ch, &max_rc, &Character::special, "1000001", cmd_t::BET);
  run_check(ch, &max_rc, &Character::special, "100000111111111111111111111111111111111", cmd_t::BET);
  run_check(ch, &max_rc, &Character::special, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd_t::BET);
  run_check(ch, &max_rc, &Character::special, "500", cmd_t::BET);
  run_check(ch, &max_rc, &Character::special, "500", cmd_t::BET);

  check_timer();
  check_timer();
  ch->desc->process_output();

  run_check(ch, &max_rc, &Character::special, "", cmd_t::HIT);

  check_timer();
  check_timer();
  ch->desc->process_output();

  run_check(ch, &max_rc, &Character::special, "", cmd_t::STAY);

  check_timer();
  check_timer();
  ch->desc->process_output();

  run_check(ch, &max_rc, &Character::special, "", cmd_t::DOUBLE);

  check_timer();
  check_timer();
  ch->desc->process_output();

  run_check(ch, &max_rc, &Character::special, "", cmd_t::DOUBLE);

  check_timer();
  check_timer();
  ch->desc->process_output();

  run_check(ch, &max_rc, &Character::special, "", cmd_t::DOUBLE);

  check_timer();
  check_timer();
  ch->desc->process_output();

  if (ch->getGold() > 2000000)
  {
    ch->send(QStringLiteral("Possible problem. After test, player gold amount is %1 from 1,000,000.\r\n").arg(ch->getGold()));
    max_rc = max_rc | ReturnValue::eFAILURE;
  }

  ch->player->bank = original_bank;
  ch->setGold(original_gold);
  DC::getInstance()->random_ = original_random;
  return max_rc;
}

tests_t tests = {{"casino", Test("casino", test_casino)}};

command_return_t Character::do_test(QStringList arguments, cmd_t cmd)
{
  QString arg1 = arguments.value(0);

  if (arg1.isEmpty() || arg1 == "help")
  {
    send("Usage: test [help] [test name]\r\n");
    send("Test names:\r\n");
    for (auto const &test : tests)
    {
      send(test.getName() + "\r\n");
    }
    return ReturnValue::eSUCCESS;
  }
  else if (arg1 == "all")
  {
    sendln("Running all tests.");
    command_return_t rc = {};
    for (auto &test : tests)
    {
      send(QStringLiteral("Running %1..").arg(test.getName()));
      rc = test.run(this) & rc;
      sendln(QStringLiteral("Return code is %1 (%2)").arg(rc).arg(rc_to_qstring(rc)));
    }
    return rc;
  }
  else if (tests.contains(arg1))
  {
    auto rc = tests[arg1].run(this);
    send(QStringLiteral("Return code is %1 (%2)\r\n").arg(rc).arg(rc_to_qstring(rc)));
    return rc;
  }

  return ReturnValue::eFAILURE;
}