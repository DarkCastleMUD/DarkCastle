/********************************
| Level 107 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"
#include "interp.h"
#include "utility.h"
#include "levels.h"
#include "player.h"
#include "mobile.h"
#include "connect.h"
#include "handler.h"
#include "returnvals.h"
#include "spells.h"

int do_archive(Character *ch, char *argument, int cmd)
{
  char name[50];
  Character *victim;

  argument = one_argument(argument, name);

  if (!*name)
  {
    send_to_char("Archive whom?\n\r", ch);
    return eFAILURE;
  }

  name[0] = toupper(name[0]);

  if (!(victim = get_pc(name)))
    return eFAILURE;

  if (victim->getLevel() >= MIN_GOD)
  {
    act("I think $N can archive $Mself, thank you.", ch, 0, victim,
        TO_CHAR, 0);
    return eFAILURE;
  }

  send_to_char("Suddenly someone reaches down and packs you into a "
               "little ball.\r\n",
               victim);
  csendf(victim, "You have been archived by %s.  Goodbye.\r\n", GET_NAME(ch));
  act("$N is grabbed up and packed into a small ball by $n.", ch, 0,
      victim, TO_ROOM, 0);
  do_quit(victim, "", 666);

  util_archive(name, ch);
  return eSUCCESS;
}

int do_unarchive(Character *ch, char *argument, int cmd)
{
  char name[50];
  argument = one_argument(argument, name);
  name[0] = toupper(name[0]);
  util_unarchive(name, ch);
  return eSUCCESS;
}

int do_pview(Character *ch, char *argument, int cmd)
{
  char name[200];
  Character *victim;
  std::string tprompt;

  argument = one_argument(argument, name);

  if ((!*name) || (!(victim = get_pc_vis(ch, name))))
  {
    send_to_char("View the prompt of whom?\n\r", ch);
    return eFAILURE;
  }

  if (!victim->desc)
  {
    send_to_char("This can only be used on linkalive players.\r\n", ch);
    return eFAILURE;
  }

  make_prompt(victim->desc, tprompt);
  send_to_char("Target's prompt is:\r\n", ch);
  send_to_char(tprompt, ch);
  send_to_char("\r\n\r\n", ch);

  return eSUCCESS;
}

command_return_t Character::do_snoop(QStringList arguments, int cmd)
{
  Character *victim;

  if (!this->desc)
    return eFAILURE;

  if (IS_NPC(this))
  {
    send_to_char("Did you ever try this before?", this);
    return eFAILURE;
  }

  if (!has_skill(COMMAND_SNOOP))
  {
    send_to_char("Huh?\r\n", this);
    return eFAILURE;
  }

  QString arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    send_to_char("Snoop whom?\n\r", this);
    return eFAILURE;
  }

  if (!(victim = get_active_pc_vis(arg1)))
  {
    send_to_char("Your victim is either not available or "
                 "linkdead.\r\n",
                 this);
    send_to_char("(You can only snoop a link-active pc.)\n\r", this);
    return eFAILURE;
  }
  if ((victim->getLevel() > this->getLevel()) && (GET_NAME(this) != victim->getNameC()))
  {
    send_to_char("Can't do that. That mob is higher than you!\n\r", this);
    logentry(QString("%1 tried to snoop a higher mob\n\r").arg(GET_NAME(this)), OVERSEER, LogChannels::LOG_GOD);
    return eFAILURE;
  }

  if (victim == this)
  {
    send_to_char("Ok, you just snoop yourself.\r\n", this);
    if (this->desc->snooping)
    {
      this->desc->snooping->snoop_by = 0;
      this->desc->snooping = 0;
    }
    logentry(QString("%1 snoops themself.").arg(getName()), this->getLevel(), LogChannels::LOG_GOD);
    return eSUCCESS;
  }

  if (victim->getLevel() == IMPLEMENTER)
  {
    sendln("What are you!? Crazy! You can't snoop an Imp.");
    victim->sendln(QString("%1 failed snooping you.").arg(getName()));
    return eFAILURE;
  }

  if (victim->desc->snoop_by)
  {
    if (victim->desc->snoop_by->character)
    {
      sendln(QString("%1 is snooping them already.").arg(victim->desc->snoop_by->character->getName()));
    }
    else
    {
      sendln(QString("Descriptor #%1 is snooping them already.").arg(victim->desc->snoop_by->descriptor));
    }

    return eFAILURE;
  }

  /* It's power trip time again, eh? */

  sendln("Ok.");

  if (this->desc->snooping)
    this->desc->snooping->snoop_by = nullptr;

  this->desc->snooping = victim->desc;
  victim->desc->snoop_by = this->desc;
  logentry(QString("%1 snoops %2.").arg(getName()).arg(victim->getName()), getLevel(), LogChannels::LOG_GOD);
  return eSUCCESS;
}

int do_stealth(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (argument[0] != '\0')
  {
    send_to_char(
        "STEALTH doesn't take any arguments; arg ignored.\r\n", ch);
  } /* if */

  if (ch->player->stealth)
  {
    ch->player->stealth = false;
    send_to_char("Stealth mode off.\r\n", ch);
  }
  else
  {
    ch->player->stealth = true;
    send_to_char("Stealth mode on.\r\n", ch);
  } /* if */
  return eSUCCESS;
}

int do_send(Character *ch, char *argument, int cmd)
{

  Character *vict;
  char name[100], message[200], buf[350];

  half_chop(argument, name, message);
  name[99] = '\0';
  message[199] = '\0';

  if (!*name || !*message)
  {
    send_to_char("Send what to who?\r\n", ch);
    return eFAILURE;
  }

  if (!(vict = ch->get_active_pc_vis(name)))
  {
    send_to_char("Noone by that name here.\r\n", ch);
    return eFAILURE;
  }

  if (ch == vict)
  {
    send_to_char("That's you, ya moron.\r\n", ch);
    return eFAILURE;
  }

  sprintf(buf, "You send '%s' to %s.\r\n", message, GET_NAME(vict));
  send_to_char(buf, ch);
  sprintf(buf, "%s\r\n", message);
  send_to_char(buf, vict);
  return eSUCCESS;
}
