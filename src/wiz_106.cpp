/********************************
| Level 106 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"
#include "handler.h"
#include "spells.h"
#include "utility.h"
#include "connect.h"
#include "levels.h"
#include "mobile.h"
#include "interp.h"
#include "player.h"
#include "returnvals.h"

#include <fmt/format.h>

int do_plats(Character *ch, char *argument, int cmd)
{
  Character *i;
  class Connection *d;
  char arg[100];
  char buf[100];
  int minamt;

  one_argument(argument, arg);
  if (*arg)
    minamt = atoi(arg);
  else
    minamt = 1;

  ch->sendln("          Plats - Player");
  ch->sendln("          --------------");

  for (d = DC::getInstance()->descriptor_list; d; d = d->next)
  {
    if ((d->connected) || (!CAN_SEE(ch, d->character)))
      continue;

    if (d->original)
      i = d->original;
    else
      i = d->character;

    if (GET_PLATINUM(i) < (uint32_t)minamt)
      continue;

    sprintf(buf, "%15d - %s - %ld - %d\n\r", GET_PLATINUM(i), GET_NAME(i), i->getGold(), GET_BANK(i));
    ch->send(buf);
  }
  return eSUCCESS;
}

int do_force(Character *ch, std::string argument, int cmd)
{
  class Connection *i = {};
  class Connection *next_i = {};
  Character *vict = {};
  std::string name = {}, to_force = {}, buf = {};

  if (IS_NPC(ch))
  {
    return eFAILURE;
  }

  if (!ch->has_skill(COMMAND_FORCE) && cmd != CMD_FORCE)
  {
    ch->send("Huh?\r\n");
    return eFAILURE;
  }

  std::tie(name, to_force) = half_chop(argument);

  if (name.empty() || to_force.empty())
  {
    ch->send("Who do you wish to force to do what?\r\n");
    return eFAILURE;
  }
  else if (name != "all")
  {
    if (!(vict = get_char_vis(ch, name)))
      ch->sendln("No one by that name here..");
    else
    {
      if (ch->getLevel() < vict->getLevel() && IS_NPC(vict))
      {
        ch->sendln("Now doing that would just tick off the IMPS!");
        logentry(QStringLiteral("%1 just tried to force %2 to %3").arg(GET_NAME(ch)).arg(GET_NAME(vict)).arg(to_force.c_str()), OVERSEER, LogChannels::LOG_GOD);
        return eSUCCESS;
      }
      if ((ch->getLevel() <= vict->getLevel()) && IS_PC(vict))
      {
        ch->sendln("Why be forceful?");
        buf = fmt::format("$n has failed to force you to '{}'.", to_force);
        act(buf, ch, 0, vict, TO_VICT, 0);
      }
      else
      {
        if (ch->player->stealth == false)
        {
          buf = fmt::format("$n has forced you to '{}'.", to_force);
          act(buf, ch, 0, vict, TO_VICT, 0);
          ch->sendln("Ok.");
        }
        buf = fmt::format("{} just forced %s to %s.", GET_NAME(ch),
                          GET_NAME(vict), to_force);
        vict->command_interpreter(to_force.c_str());
        logentry(buf.c_str(), ch->getLevel(), LogChannels::LOG_GOD);
      }
    }
  }

  else
  { /* force all */
    if (ch->getLevel() < OVERSEER)
    {
      ch->sendln("Not gonna happen.");
      return eFAILURE;
    }
    for (i = DC::getInstance()->descriptor_list; i; i = next_i)
    {
      next_i = i->next;
      if (i->character != ch && !i->connected)
      {
        vict = i->character;
        if (ch->getLevel() <= vict->getLevel())
          continue;
        else
        {
          if (ch->player->stealth == false || ch->getLevel() < 109)
          {
            buf = fmt::format("$n has forced you to '{}'.", to_force);
            act(buf, ch, 0, vict, TO_VICT, 0);
          }
          vict->command_interpreter(to_force.c_str());
        }
      }
    }
    ch->sendln("Ok.");
    buf = fmt::format("{} just forced all to {}.", GET_NAME(ch), to_force);
    logentry(buf.c_str(), ch->getLevel(), LogChannels::LOG_GOD);
  }
  return eSUCCESS;
}

typedef command_return_t (*test_function_t)(Character *ch);

class Test
{
public:
  Test() : function_(nullptr) {}
  Test(QString name, test_function_t function = nullptr)
      : name_(name), function_(function)
  {
  }
  command_return_t run(Character *ch)
  {
    if (function_ && ch)
    {
      return function_(ch);
    }
    return eFAILURE;
  }
  QString getName(void) const { return name_; }

private:
  QString name_;
  test_function_t function_;
};
typedef QMap<QString, Test> tests_t;

command_return_t run_all_events(Character *ch = nullptr)
{
  uint64_t counter{};
  while (timer_list != nullptr && counter++ < 1000)
  {
    if (ch)
    {
      ch->send("Running check_timer()\r\n");
      process_output(ch->desc);
    }
    check_timer();
    process_output(ch->desc);
  }
  if (counter >= 1000)
  {
    return eFAILURE;
  }
  return eSUCCESS;
}

QString rc_to_qstring(const command_return_t &rc)
{
  QStringList strings;
  if (isSet(rc, eFAILURE))
  {
    strings += "eFAILURE";
  }
  if (isSet(rc, eSUCCESS))
  {
    strings += "eSUCCESS ";
  }
  if (isSet(rc, eCH_DIED))
  {
    strings += "eCH_DIED ";
  }
  if (isSet(rc, eDELAYED_EXEC))
  {
    strings += "eDELAYED_EXEC ";
  }
  if (isSet(rc, eEXTRA_VAL2))
  {
    strings += "eEXTRA_VAL2 ";
  }
  if (isSet(rc, eEXTRA_VALUE))
  {
    strings += "eEXTRA_VALUE ";
  }
  if (isSet(rc, eIMMUNE_VICTIM))
  {
    strings += "eIMMUNE_VICTIM ";
  }
  if (isSet(rc, eINTERNAL_ERROR))
  {
    strings += "eINTERNAL_ERROR ";
  }
  if (isSet(rc, eVICT_DIED))
  {
    strings += "eVICT_DIED ";
  }

  return strings.join(',');
}

void run_check(Character *ch, command_return_t *rc, auto *function, char *arguments = nullptr, int cmd = CMD_DEFAULT)
{
  command_return_t new_rc{};
  if (ch)
  {
    if (function)
    {
      new_rc = function(ch, arguments, cmd);
    }
    ch->send(QString("Return code is %1 (%2)\r\n").arg(new_rc).arg(rc_to_qstring(new_rc)));
    if (ch->desc)
    {
      process_output(ch->desc);
    }
  }

  if (rc)
  {
    *rc = *rc | new_rc;
  }
}

void run_check(Character *ch, command_return_t *rc, command_gen2_t function, std::string arguments = std::string(), int cmd = CMD_DEFAULT)
{
  command_return_t new_rc{};
  if (ch)
  {
    if (function)
    {
      new_rc = function(ch, arguments, cmd);
    }
    ch->send(QString("Return code is %1 (%2)\r\n").arg(new_rc).arg(rc_to_qstring(new_rc)));
    if (ch->desc)
    {
      process_output(ch->desc);
    }
  }

  if (rc)
  {
    *rc = *rc | new_rc;
  }
}

void run_check(Character *ch, command_return_t *rc, command_gen3_t function, QStringList arguments = QStringList(), int cmd = CMD_DEFAULT)
{
  command_return_t new_rc{};
  if (ch)
  {
    if (function)
    {
      new_rc = (*ch.*(function))(arguments, cmd);
    }
    ch->send(QString("Return code is %1 (%2)\r\n").arg(new_rc).arg(rc_to_qstring(new_rc)));
    if (ch->desc)
    {
      process_output(ch->desc);
    }
  }

  if (rc)
  {
    *rc = *rc | new_rc;
  }
}

void run_check(Character *ch, command_return_t *rc, command_special_t function, QString arguments = "", int cmd = CMD_DEFAULT)
{
  command_return_t new_rc{};
  if (ch)
  {
    if (function)
    {
      new_rc = (*ch.*(function))(arguments, cmd);
    }
    ch->send(QString("Return code is %1 (%2)\r\n").arg(new_rc).arg(rc_to_qstring(new_rc)));
    if (ch->desc)
    {
      process_output(ch->desc);
    }
  }

  if (rc)
  {
    *rc = *rc | new_rc;
  }
}

command_return_t test_casino(Character *ch)
{
  if (!ch || !ch->player)
  {
    return eFAILURE;
  }

  int max_rc{};

  run_check(ch, &max_rc, &Character::do_goto, {"1"});
  run_check(ch, &max_rc, &Character::do_goto, {"21900"});

  run_check(ch, &max_rc, do_look, "sign", CMD_LOOK);
  run_check(ch, &max_rc, do_examine, "sign", CMD_EXAMINE);

  run_check(ch, &max_rc, do_move, "", CMD_NORTH);

  run_check(ch, &max_rc, do_look, "fountain", CMD_LOOK);
  run_check(ch, &max_rc, do_examine, "fountain", CMD_EXAMINE);
  run_check(ch, &max_rc, do_drink, "fountain");

  run_check(ch, &max_rc, do_look, "machine", CMD_LOOK);
  run_check(ch, &max_rc, do_examine, "machine", CMD_EXAMINE);

  // saving the player's finances and giving them a consistent
  // amount of cash in bank to be withdrawn
  auto original_bank = ch->player->bank;
  ch->player->bank = 1000000;
  auto original_gold = ch->getGold();
  ch->setGold(0);

  // These are special code procedures that should be on the objects in this room
  run_check(ch, &max_rc, &Character::special, "", CMD_BALANCE);
  run_check(ch, &max_rc, &Character::special, "", CMD_WITHDRAW);
  run_check(ch, &max_rc, &Character::special, "1000000", CMD_WITHDRAW);

  run_check(ch, &max_rc, do_move, "", CMD_NORTH);

  run_check(ch, &max_rc, do_look, "table", CMD_LOOK);
  run_check(ch, &max_rc, do_examine, "table", CMD_EXAMINE);

  auto original_random = DC::getInstance()->random_;
  DC::getInstance()->random_ = QRandomGenerator(1);

  run_check(ch, &max_rc, &Character::special, "", CMD_BET);
  run_check(ch, &max_rc, &Character::special, "-1", CMD_BET);
  run_check(ch, &max_rc, &Character::special, "0", CMD_BET);
  run_check(ch, &max_rc, &Character::special, "1", CMD_BET);
  run_check(ch, &max_rc, &Character::special, "1000001", CMD_BET);
  run_check(ch, &max_rc, &Character::special, "100000111111111111111111111111111111111", CMD_BET);
  run_check(ch, &max_rc, &Character::special, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", CMD_BET);
  run_check(ch, &max_rc, &Character::special, "500", CMD_BET);
  run_check(ch, &max_rc, &Character::special, "500", CMD_BET);

  check_timer();
  check_timer();
  process_output(ch->desc);

  run_check(ch, &max_rc, &Character::special, "", CMD_HIT);

  check_timer();
  check_timer();
  process_output(ch->desc);

  run_check(ch, &max_rc, &Character::special, "", CMD_STAY);

  check_timer();
  check_timer();
  process_output(ch->desc);

  run_check(ch, &max_rc, &Character::special, "", CMD_DOUBLE);

  check_timer();
  check_timer();
  process_output(ch->desc);

  run_check(ch, &max_rc, &Character::special, "", CMD_DOUBLE);

  check_timer();
  check_timer();
  process_output(ch->desc);

  run_check(ch, &max_rc, &Character::special, "", CMD_DOUBLE);

  check_timer();
  check_timer();
  process_output(ch->desc);

  if (ch->getGold() > 2000000)
  {
    ch->send(QStringLiteral("Possible problem. After test, player gold amount is %1 from 1,000,000.\r\n").arg(ch->getGold()));
    max_rc = max_rc | eFAILURE;
  }

  ch->player->bank = original_bank;
  ch->setGold(original_gold);
  DC::getInstance()->random_ = original_random;
  return max_rc;
}

tests_t tests = {{"casino", Test("casino", test_casino)}};

command_return_t Character::do_test(QStringList arguments, int cmd)
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
    return eSUCCESS;
  }
  else if (arg1 == "all")
  {
    sendln("Running all tests.");
    command_return_t rc{};
    for (auto &test : tests)
    {
      send(QStringLiteral("Running %1..").arg(test.getName()));
      rc = test.run(this) & rc;
      sendln(QString("Return code is %1 (%2)").arg(rc).arg(rc_to_qstring(rc)));
    }
    return rc;
  }
  else if (tests.contains(arg1))
  {
    auto rc = tests[arg1].run(this);
    send(QString("Return code is %1 (%2)\r\n").arg(rc).arg(rc_to_qstring(rc)));
    return rc;
  }

  return eFAILURE;
}