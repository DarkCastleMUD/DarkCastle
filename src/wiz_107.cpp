/********************************
| Level 107 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/DC.h"
ReturnValues do_archive(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  CharacterPtr victim;

  argument = one_argument(argument, name);

  if (name.isEmpty())
  {
    ch->sendln(u"Archive whom?"_s);
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

ReturnValues do_unarchive(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  argument = one_argument(argument, name);
  name[0] = toupper(name[0]);
  util_unarchive(name, ch);
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_pview(QStringList arguments, cmd_t cmd)
{
  auto name = arguments.value(0);
  auto victim = get_pc_vis(this, name);

  if (name.isEmpty() || !victim)
  {
    sendln(u"View the prompt of whom?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!victim->conn_)
  {
    sendln(u"This can only be used on linkalive players."_s);
    return ReturnValue::eFAILURE;
  }

  sendln(u"Target's prompt is: %1"_s.arg(victim->getPrompt()));
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_snoop(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim;

  if (!conn_)
    return ReturnValue::eFAILURE;

  if (isNonPlayer())
  {
    send(u"Did you ever try this before?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!has_skill(COMMAND_SNOOP))
  {
    sendln(u"Huh?"_s);
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    sendln(u"Snoop whom?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_active_pc_vis(arg1)))
  {
    send_to_char("Your victim is either not available or "
                 "linkdead.\r\n",
                 this);
    sendln(u"(You can only snoop a link-active pc.)"_s);
    return ReturnValue::eFAILURE;
  }
  if ((victim->getLevel() > getLevel()) && (qPrintable(name()) != qPrintable(victim->name())))
  {
    sendln(u"Can't do that. That mob is higher than you!"_s);
    dc_->logentry(u"%1 tried to snoop a higher mob\r\n"_s.arg(qPrintable(name())), OVERSEER, DC::LogChannel::LOG_GOD);
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln(u"Ok, you just snoop yourself."_s);
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
    sendln(u"What are you!? Crazy! You can't snoop an Imp."_s);
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

  sendln(u"Ok."_s);

  if (conn_->snooping)
    conn_->snooping->snoop_by = {};

  conn_->snooping = victim->conn_;
  victim->conn_->snoop_by = conn_;
  dc_->logentry(u"%1 snoops %2."_s.arg(name()).arg(victim->name()), getLevel(), DC::LogChannel::LOG_GOD);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_stealth(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (argument[0] != '\0')
  {
    ch->sendln(u"STEALTH doesn't take any arguments; arg ignored."_s);
  } /* if */

  if (ch->player->stealth)
  {
    ch->player->stealth = false;
    ch->sendln(u"Stealth mode off."_s);
  }
  else
  {
    ch->player->stealth = true;
    ch->sendln(u"Stealth mode on."_s);
  } /* if */
  return ReturnValue::eSUCCESS;
}

ReturnValues do_send(CharacterPtr ch, QString argument, cmd_t cmd)
{

  CharacterPtr vict;
  QString name, message, buf;

  half_chop(argument, name, message);
  name[99] = '\0';
  message[199] = '\0';

  if (name.isEmpty() || message.isEmpty())
  {
    ch->sendln(u"Send what to who?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!(vict = ch->get_active_pc_vis(name)))
  {
    ch->sendln(u"Noone by that name here."_s);
    return ReturnValue::eFAILURE;
  }

  if (ch == vict)
  {
    ch->sendln(u"That's you, ya moron."_s);
    return ReturnValue::eFAILURE;
  }

  dc_sprintf(buf, "You send '%s' to %s.\r\n", message, qPrintable(vict->name()));
  ch->send(buf);
  dc_sprintf(buf, "%s\r\n", message);
  vict->send(buf);
  return ReturnValue::eSUCCESS;
}
