/********************************
| Level 107 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/DC.h"
command_return_t do_archive(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  CharacterPtr victim;

  argument = one_argument(argument, name);

  if (name.isEmpty())
  {
    ch->sendln("Archive whom?");
    return ReturnValue::eFAILURE;
  }

  name[0] = toupper(name[0]);

  if (!(victim = get_pc(name)))
    return ReturnValue::eFAILURE;

  if (victim->getLevel() >= MIN_GOD)
  {
    act("I think $N can archive $Mself, thank you.", ch, 0, victim,
        TO_CHAR, 0);
    return ReturnValue::eFAILURE;
  }

  send_to_char("Suddenly someone reaches down and packs you into a "
               "little ball.\r\n",
               victim);
  victim->send(u"You have been archived by %s.  Goodbye.\r\n"_s.arg(qPrintable(ch->name())));
  act("$N is grabbed up and packed into a small ball by $n.", ch, 0,
      victim, TO_ROOM, 0);
  do_quit(victim, "", cmd_t::SAVE_SILENTLY);

  util_archive(name, ch);
  return ReturnValue::eSUCCESS;
}

command_return_t do_unarchive(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  argument = one_argument(argument, name);
  name[0] = toupper(name[0]);
  util_unarchive(name, ch);
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_pview(QStringList arguments, cmd_t cmd)
{
  auto name = arguments.value(0);
  auto victim = get_pc_vis(this, name);

  if (name.isEmpty() || !victim)
  {
    sendln("View the prompt of whom?");
    return ReturnValue::eFAILURE;
  }

  if (!victim->conn_)
  {
    sendln("This can only be used on linkalive players.");
    return ReturnValue::eFAILURE;
  }

  sendln(u"Target's prompt is: %1"_s.arg(victim->getPrompt()));
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_snoop(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim;

  if (!conn_)
    return ReturnValue::eFAILURE;

  if (isNonPlayer())
  {
    send("Did you ever try this before?");
    return ReturnValue::eFAILURE;
  }

  if (!has_skill(COMMAND_SNOOP))
  {
    sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    sendln("Snoop whom?");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_active_pc_vis(arg1)))
  {
    send_to_char("Your victim is either not available or "
                 "linkdead.\r\n",
                 this);
    sendln("(You can only snoop a link-active pc.)");
    return ReturnValue::eFAILURE;
  }
  if ((victim->getLevel() > getLevel()) && (qPrintable(name()) != qPrintable(victim->name())))
  {
    sendln("Can't do that. That mob is higher than you!");
    dc_->logentry(u"%1 tried to snoop a higher mob\r\n"_s.arg(qPrintable(name())), OVERSEER, DC::LogChannel::LOG_GOD);
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln("Ok, you just snoop yourself.");
    if (conn_->snooping)
    {
      conn_->snooping->snoop_by = {};
      conn_->snooping = {};
    }
    dc_->logentry(u"%1 snoops themself."_s.arg(name()), getLevel(), DC::LogChannel::LOG_GOD);
    return ReturnValue::eSUCCESS;
  }

  if (victim->getLevel() == IMPLEMENTER)
  {
    sendln("What are you!? Crazy! You can't snoop an Imp.");
    victim->sendln(u"%1 failed snooping you."_s.arg(name()));
    return ReturnValue::eFAILURE;
  }

  if (victim->conn_->snoop_by)
  {
    if (victim->conn_->snoop_by->character)
    {
      sendln(u"%1 is snooping them already."_s.arg(victim->conn_->snoop_by->character->name()));
    }
    else
    {
      sendln(u"Descriptor #%1 is snooping them already."_s.arg(victim->conn_->snoop_by->descriptor));
    }

    return ReturnValue::eFAILURE;
  }

  /* It's power trip time again, eh? */

  sendln("Ok.");

  if (conn_->snooping)
    conn_->snooping->snoop_by = {};

  conn_->snooping = victim->conn_;
  victim->conn_->snoop_by = conn_;
  dc_->logentry(u"%1 snoops %2."_s.arg(name()).arg(victim->name()), getLevel(), DC::LogChannel::LOG_GOD);
  return ReturnValue::eSUCCESS;
}

command_return_t do_stealth(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (argument[0] != '\0')
  {
    ch->sendln("STEALTH doesn't take any arguments; arg ignored.");
  } /* if */

  if (ch->player->stealth)
  {
    ch->player->stealth = false;
    ch->sendln("Stealth mode off.");
  }
  else
  {
    ch->player->stealth = true;
    ch->sendln("Stealth mode on.");
  } /* if */
  return ReturnValue::eSUCCESS;
}

command_return_t do_send(CharacterPtr ch, QString argument, cmd_t cmd)
{

  CharacterPtr vict;
  QString name, message[200], buf[350];

  half_chop(argument, name, message);
  name[99] = '\0';
  message[199] = '\0';

  if (name.isEmpty() || message.isEmpty())
  {
    ch->sendln("Send what to who?");
    return ReturnValue::eFAILURE;
  }

  if (!(vict = ch->get_active_pc_vis(name)))
  {
    ch->sendln("Noone by that name here.");
    return ReturnValue::eFAILURE;
  }

  if (ch == vict)
  {
    ch->sendln("That's you, ya moron.");
    return ReturnValue::eFAILURE;
  }

  dc_sprintf(buf, "You send '%s' to %s.\r\n", message, qPrintable(vict->name()));
  ch->send(buf);
  dc_sprintf(buf, "%s\r\n", message);
  vict->send(buf);
  return ReturnValue::eSUCCESS;
}
