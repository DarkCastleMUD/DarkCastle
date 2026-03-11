/********************************
| Level 107 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/wizard.h"
#include "DC/interp.h"
#include "DC/utility.h"

#include "DC/player.h"
#include "DC/mobile.h"
#include "DC/connect.h"
#include "DC/handler.h"
#include "DC/returnvals.h"
#include "DC/spells.h"

int do_archive(Character *ch, char *argument, cmd_t cmd)
{
  char name[50];
  Character *victim;

  argument = one_argument(argument, name);

  if (!*name)
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
  csendf(victim, "You have been archived by %s.  Goodbye.\r\n", GET_NAME(ch));
  act("$N is grabbed up and packed into a small ball by $n.", ch, 0,
      victim, TO_ROOM, 0);
  do_quit(victim, "", cmd_t::SAVE_SILENTLY);

  util_archive(name, ch);
  return ReturnValue::eSUCCESS;
}

int do_unarchive(Character *ch, char *argument, cmd_t cmd)
{
  char name[50];
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

  if (!victim->desc)
  {
    sendln("This can only be used on linkalive players.");
    return ReturnValue::eFAILURE;
  }

  sendln(QStringLiteral("Target's prompt is: %1").arg(victim->getPrompt()));
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_snoop(QStringList arguments, cmd_t cmd)
{
  Character *victim;

  if (!this->desc)
    return ReturnValue::eFAILURE;

  if (IS_NPC(this))
  {
    this->send("Did you ever try this before?");
    return ReturnValue::eFAILURE;
  }

  if (!has_skill(COMMAND_SNOOP))
  {
    this->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    this->sendln("Snoop whom?");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_active_pc_vis(arg1)))
  {
    send_to_char("Your victim is either not available or "
                 "linkdead.\r\n",
                 this);
    this->sendln("(You can only snoop a link-active pc.)");
    return ReturnValue::eFAILURE;
  }
  if ((victim->getLevel() > this->getLevel()) && (GET_NAME(this) != victim->getNameC()))
  {
    this->sendln("Can't do that. That mob is higher than you!");
    logentry(QStringLiteral("%1 tried to snoop a higher mob\r\n").arg(GET_NAME(this)), OVERSEER, DC::LogChannel::LOG_GOD);
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    this->sendln("Ok, you just snoop yourself.");
    if (this->desc->snooping)
    {
      this->desc->snooping->snoop_by = 0;
      this->desc->snooping = 0;
    }
    logentry(QStringLiteral("%1 snoops themself.").arg(getName()), this->getLevel(), DC::LogChannel::LOG_GOD);
    return ReturnValue::eSUCCESS;
  }

  if (victim->getLevel() == IMPLEMENTER)
  {
    sendln("What are you!? Crazy! You can't snoop an Imp.");
    victim->sendln(QStringLiteral("%1 failed snooping you.").arg(getName()));
    return ReturnValue::eFAILURE;
  }

  if (victim->desc->snoop_by)
  {
    if (victim->desc->snoop_by->character)
    {
      sendln(QStringLiteral("%1 is snooping them already.").arg(victim->desc->snoop_by->character->getName()));
    }
    else
    {
      sendln(QStringLiteral("Descriptor #%1 is snooping them already.").arg(victim->desc->snoop_by->descriptor));
    }

    return ReturnValue::eFAILURE;
  }

  /* It's power trip time again, eh? */

  sendln("Ok.");

  if (this->desc->snooping)
    this->desc->snooping->snoop_by = nullptr;

  this->desc->snooping = victim->desc;
  victim->desc->snoop_by = this->desc;
  logentry(QStringLiteral("%1 snoops %2.").arg(getName()).arg(victim->getName()), getLevel(), DC::LogChannel::LOG_GOD);
  return ReturnValue::eSUCCESS;
}

int do_stealth(Character *ch, char *argument, cmd_t cmd)
{
  if (IS_NPC(ch))
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

int do_send(Character *ch, char *argument, cmd_t cmd)
{

  Character *vict;
  char name[100], message[200], buf[350];

  half_chop(argument, name, message);
  name[99] = '\0';
  message[199] = '\0';

  if (!*name || !*message)
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

  sprintf(buf, "You send '%s' to %s.\r\n", message, GET_NAME(vict));
  ch->send(buf);
  sprintf(buf, "%s\r\n", message);
  vict->send(buf);
  return ReturnValue::eSUCCESS;
}
