/************************************************************************
| $Id: group.cpp,v 1.30 2011/11/29 02:21:54 jhhudso Exp $
| group.C
| Description:  Group related commands; join, abandon, follow, etc..
*/

#include <cctype>

#include "DC/character.h"
#include "DC/room.h"
#include "DC/affect.h"
#include "DC/utility.h"
#include "DC/mobile.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/clan.h"
#include "DC/act.h"
#include "DC/db.h"
#include "DC/player.h"
#include "DC/sing.h" // stop_grouped_bards
#include <cstring>
#include "DC/returnvals.h"
#include "DC/spells.h"
#include "DC/terminal.h"
#include "DC/comm.h"

int do_abandon(Character *ch, char *argument, int cmd)
{
  Character *k;
  char buf[MAX_INPUT_LENGTH + 1];

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  if ((!ch->master) && (IS_AFFECTED(ch, AFF_GROUP)))
  {
    ch->sendln("You can't abandon a group you're leading.");
    ch->sendln("You must disband the group.");
    return eFAILURE;
  }

  if ((!ch->master) || (!IS_AFFECTED(ch, AFF_GROUP)))
  {
    ch->sendln("Who you gonna abandon?!");
    return eFAILURE;
  }

  if ((IS_NPC(ch)) && (IS_AFFECTED(ch, AFF_CHARM)))
  {
    ch->sendln("You're in love. Forget it!");
    return eFAILURE;
  }

  stop_grouped_bards(ch, 1);

  k = ch->master;

  sprintf(buf, "You abandon: %s\n\r", k->group_name);
  ch->send(buf);
  sprintf(buf, "%s abandons: %s", GET_SHORT(ch), k->group_name);
  act(buf, ch, 0, 0, TO_ROOM, 0);

  if (IS_PC(ch))
  {
    ch->player->group_pkills = 0;
    ch->player->grpplvl = 0;
    ch->player->group_kills = 0;
  }

  stop_follower(ch);
  return eSUCCESS;
}

int do_found(Character *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH + 1];

  if (IS_NPC(ch))
    return eFAILURE;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  if (!*argument)
  {
    ch->sendln("Found what?!");
    return eFAILURE;
  }
  if ((ch->master) && (!IS_AFFECTED(ch, AFF_GROUP)))
  {
    ch->sendln("You can't found your own group while following someone around!");
    return eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You can't found a group if you're already in one!");
    return eFAILURE;
  }

  for (; isspace(*argument); argument++)
    ;

  if (strlen(argument) > 50)
  {
    ch->sendln("You gonna name your party? Or write a book?!  50 characters max.");
    return eFAILURE;
  }

  ch->group_name = str_dup(argument);
  sprintf(buf, "You found: %s\n\r", argument);
  ch->send(buf);
  sprintf(buf, "%s founds: %s", GET_SHORT(ch), argument);
  act(buf, ch, 0, 0, TO_ROOM, 0);

  if (IS_PC(ch))
  {
    ch->player->group_pkills = 0;
    ch->player->grpplvl = 0;
    ch->player->group_kills = 0;
  }

  SETBIT(ch->affected_by, AFF_GROUP);
  REMOVE_BIT(ch->player->toggles, Player::PLR_LFG);
  return eSUCCESS;
}

command_return_t Character::do_split(QStringList arguments, int cmd)
{
  quint64 share = 0, extra = 0;
  quint64 no_members = 0;
  Character *k = nullptr;
  struct follow_type *f = nullptr;

  if (arguments.isEmpty())
  {
    send("Split what?\r\n");
    return eFAILURE;
  }

  if (isPlayerGoldThief())
  {
    send("Nobody wants any part of your stolen booty!\r\n");
    return eFAILURE;
  }

  QString number = arguments.at(0);

  bool ok = false;
  const qulonglong amount = number.toULongLong(&ok);
  if (ok == false)
  {
    send("Invalid value.\r\n");
    send(QStringLiteral("Valid values are %1 to %2.\r\n").arg(1).arg(static_cast<quint64>(-1)));
    return eFAILURE;
  }

  if (amount == 0)
  {
    send("You hand out zero coins to everyone, but no one notices.\r\n");
    return eSUCCESS;
  }

  if (getGold() < amount)
  {
    send("You don't have that much gold!\r\n");
    return eFAILURE;
  }

  if (master)
    k = master;
  else
    k = this;

  if ((!IS_AFFECTED(k, AFF_GROUP)) || (!IS_AFFECTED(this, AFF_GROUP)))
  {
    send("You are not grouped. You must be grouped to split your money!\r\n");
    return eFAILURE;
  }

  if (k->in_room == in_room)
    no_members = 1;
  else
    no_members = 0;

  for (f = k->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&
        (f->follower->in_room == in_room) &&
        !IS_NPC(f->follower))
      no_members++;
  }

  if (no_members == 0) // should be impossible
  {
    send("You got a divide by 0.  Tell a god how.\r\n");
    return eFAILURE;
  }

  share = amount / no_members;
  extra = amount % no_members;
  removeGold(amount);
  save(666);

  send(QStringLiteral("You split %L1 $B$5gold$R coins. Your share is %L2 gold coins.\r\n").arg(amount).arg(share + extra));
  addGold(share + extra);

  if (k != this && k->in_room == in_room)
  {
    k->send(QStringLiteral("%1 splits %L2 $B$5gold$R coins. Your share is %L3 $B$5gold$R coins.\r\n").arg(GET_SHORT(this)).arg(amount).arg(share));
    int lost = 0;
    if (k->clan && get_clan(k)->tax && !isSet(GET_TOGGLES(k), Player::PLR_NOTAX) &&
        (k->clan != clan || (k->clan == clan && isSet(GET_TOGGLES(this), Player::PLR_NOTAX))))
    {
      lost = (int)((float)share * (float)((float)get_clan(k)->tax / 100));
      k->send(QStringLiteral("Your clan taxes %L1 $B$5gold$R of your share.\r\n").arg(lost));
      get_clan(k)->cdeposit(lost);
      save_clans();
    }
    k->addGold(share - lost);
  }
  for (f = k->followers; f; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP) &&
        f->follower->in_room == in_room &&
        f->follower != this &&
        !IS_NPC(f->follower))
    {
      f->follower->send(QStringLiteral("%1 splits %L2 $B$5gold$R coins. Your share is %L3 $B$5gold$R coins.\r\n").arg(GET_SHORT(this)).arg(amount).arg(share));
      int lost = 0;
      if (f->follower->clan && get_clan(f->follower)->tax && !isSet(GET_TOGGLES(f->follower), Player::PLR_NOTAX) &&
          (f->follower->clan != clan || (f->follower->clan == clan && isSet(GET_TOGGLES(this), Player::PLR_NOTAX))))
      {
        lost = (int)((float)share * (float)((float)get_clan(f->follower)->tax / 100));
        f->follower->send(QStringLiteral("Your clan taxes %L1 gold of your share.\r\n").arg(lost));
        get_clan(f->follower)->cdeposit(lost);
        save_clans();
      }
      f->follower->addGold(share - lost);
    }
  }
  return eSUCCESS;
}

void setup_group_buf(char *report, Character *j, Character *i)
{
  if (IS_NPC(j) || (IS_ANONYMOUS(j) && (i->clan != j->clan || !i->clan)))
  {
    if (GET_CLASS(j) == CLASS_MONK || GET_CLASS(j) == CLASS_BARD)
      sprintf(report, "[-====-|      %3d%%    hp     %3d%%   k     %3d%%   mv]",
              MAX(1, j->getHP()) * 100 / MAX(1, GET_MAX_HIT(j)),
              MAX(1, GET_KI(j)) * 100 / MAX(1, GET_MAX_KI(j)),
              MAX(1, GET_MOVE(j)) * 100 / MAX(1, GET_MAX_MOVE(j)));
    else if (GET_CLASS(j) == CLASS_WARRIOR || GET_CLASS(j) == CLASS_THIEF ||
             GET_CLASS(j) == CLASS_BARBARIAN)
      sprintf(report, "[-====-|      %3d%%    hp    -====-        %3d%%   mv]",
              MAX(1, j->getHP()) * 100 / MAX(1, GET_MAX_HIT(j)),
              MAX(1, GET_MOVE(j)) * 100 / MAX(1, GET_MAX_MOVE(j)));
    else
      sprintf(report, "[-====-|      %3d%%    hp     %3d%%   m     %3d%%   mv]",
              MAX(1, j->getHP()) * 100 / MAX(1, GET_MAX_HIT(j)),
              MAX(1, GET_MANA(j)) * 100 / MAX(1, GET_MAX_MANA(j)),
              MAX(1, GET_MOVE(j)) * 100 / MAX(1, GET_MAX_MOVE(j)));
  }
  else
  {
    if (IS_PC(i) && isSet(i->player->toggles, Player::PLR_ANSI))
    {
      if (GET_CLASS(j) == CLASS_MONK || GET_CLASS(j) == CLASS_BARD)
        sprintf(report, "[Lv %3d| %s%6d%s/%-6dhp %s%5d%s/%-5dk %s%5d%s/%-5dmv]",
                j->getLevel(),
                calc_color(j->getHP(), GET_MAX_HIT(j)), j->getHP(), NTEXT, GET_MAX_HIT(j),
                calc_color(GET_KI(j), GET_MAX_KI(j)), GET_KI(j), NTEXT, GET_MAX_KI(j),
                calc_color(GET_MOVE(j), GET_MAX_MOVE(j)), GET_MOVE(j), NTEXT, GET_MAX_MOVE(j));
      else if (GET_CLASS(j) == CLASS_WARRIOR || GET_CLASS(j) == CLASS_THIEF ||
               GET_CLASS(j) == CLASS_BARBARIAN)
        sprintf(report, "[Lv %3d| %s%6d%s/%-6dhp    -====-    %s%5d%s/%-5dmv]",
                j->getLevel(),
                calc_color(j->getHP(), GET_MAX_HIT(j)), j->getHP(), NTEXT, GET_MAX_HIT(j),
                calc_color(GET_MOVE(j), GET_MAX_MOVE(j)), GET_MOVE(j), NTEXT, GET_MAX_MOVE(j));
      else
        sprintf(report, "[Lv %3d| %s%6d%s/%-6dhp %s%5d%s/%-5dm %s%5d%s/%-5dmv]",
                j->getLevel(),
                calc_color(j->getHP(), GET_MAX_HIT(j)), j->getHP(), NTEXT, GET_MAX_HIT(j),
                calc_color(GET_MANA(j), GET_MAX_MANA(j)), GET_MANA(j), NTEXT, GET_MAX_MANA(j),
                calc_color(GET_MOVE(j), GET_MAX_MOVE(j)), GET_MOVE(j), NTEXT, GET_MAX_MOVE(j));
    }
    else
    {
      if (GET_CLASS(j) == CLASS_MONK || GET_CLASS(j) == CLASS_BARD)
        sprintf(report, "[Lv %3d| %6d/%-6dhp %5d/%-5dk %5d/%-5dmv]",
                j->getLevel(), j->getHP(), GET_MAX_HIT(j), GET_KI(j),
                GET_MAX_KI(j), GET_MOVE(j), GET_MAX_MOVE(j));
      else if (GET_CLASS(j) == CLASS_WARRIOR || GET_CLASS(j) == CLASS_THIEF ||
               GET_CLASS(j) == CLASS_BARBARIAN)
        sprintf(report, "[Lv %3d| %6d/%-6dhp    -====-    %5d/%-5dmv]",
                j->getLevel(), j->getHP(), GET_MAX_HIT(j),
                GET_MOVE(j), GET_MAX_MOVE(j));
      else
        sprintf(report, "[Lv %3d| %6d/%-6dhp %5d/%-5dm %5d/%-5dmv]",
                j->getLevel(), j->getHP(), GET_MAX_HIT(j), GET_MANA(j),
                GET_MAX_MANA(j), GET_MOVE(j), GET_MAX_MOVE(j));
    }
  }
}

int do_group(Character *ch, char *argument, int cmd)
{
  char name[256];
  char buf[256], report[256];
  Character *victim, *k, *j;
  struct follow_type *f;
  bool found;

  one_argument(argument, name);

  if (!*name)
  {
    if (!IS_AFFECTED(ch, AFF_GROUP))
      ch->sendln("But you are a member of no group?!");
    else
    {
      ch->sendln("Your group consists of:");
      if (ch->master)
        k = ch->master;
      else
        k = ch;

      setup_group_buf(report, k, ch);
      sprintf(buf, "%s    $N (Leader)", report);

      if (IS_AFFECTED(k, AFF_GROUP))
        act(buf, ch, 0, k, TO_CHAR, ASLEEP);

      for (f = k->followers; f; f = f->next)
      {
        if (IS_AFFECTED(f->follower, AFF_GROUP))
        {
          j = f->follower;
          setup_group_buf(report, j, ch);
          sprintf(buf, "%s    $N", report);
          act(buf, ch, 0, f->follower, TO_CHAR, ASLEEP);
        }
      }
    }

    return eSUCCESS;
  }

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  if (!(victim = ch->get_char_room_vis(name)))
    ch->sendln("No one here by that name.");

  else
  {
    if (!IS_AFFECTED(ch, AFF_GROUP))
    {
      ch->sendln("You must first found a group!");
      return eFAILURE;
    }

    if (ch->master)
    {
      act("You can not enroll group members without being head of a group.", ch, 0, 0, TO_CHAR, 0);
      return eFAILURE;
    }

    found = false;

    if (victim == ch)
      found = true;
    else
    {
      for (f = ch->followers; f; f = f->next)
      {
        if (f->follower == victim)
        {
          found = true;
          break;
        }
      }
    }

    if (found)
    {
      if (ch == victim)
      {
        ch->sendln("You must found a group, or Disband a group.");
        return eFAILURE;
      }

      if (IS_AFFECTED(victim, AFF_GROUP))
      {
        stop_grouped_bards(victim, 1);
        act("$n has been kicked out of the group!", victim, 0, ch, TO_ROOM, 0);
        act("You are no longer a member of the group!", victim, 0, 0, TO_CHAR, ASLEEP);
        REMBIT(victim->affected_by, AFF_GROUP);
      }
      else
      {
        act("$n is now a group member.", victim, 0, 0, TO_ROOM, 0);
        act("You are now a group member.", victim, 0, 0, TO_CHAR, ASLEEP);
        SETBIT(victim->affected_by, AFF_GROUP);
        if (IS_PC(victim))
          REMOVE_BIT(victim->player->toggles, Player::PLR_LFG);
      }
      return eSUCCESS;
      //    }
      //  else
      //	  act("$n is not of the right caliber to join this group.", victim, 0, 0, TO_ROOM, ASLEEP);
    }
    else
      act("$N must follow you, to enter the group.", ch, 0, victim, TO_CHAR, ASLEEP);
  }
  return eFAILURE;
}

int do_promote(Character *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH + 1];
  char buf[250];
  Character *new_new_leader, *k;
  struct follow_type *f, *next_f;

  one_argument(argument, name);

  if (ch->master)
  {
    ch->sendln("You aren't running the show here, pal.");
    return eFAILURE;
  }

  if ((!ch->master) && !IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You don't even have a group to promote anyone.");
    return eFAILURE;
  }

  if (!*name)
  {
    ch->sendln("Who do you wish to promote to group leader? ");
    return eFAILURE;
  }

  if (!(new_new_leader = ch->get_char_room_vis(name)))
  {
    ch->sendln("I see no person by that name here!");
    return eFAILURE;
  }

  if (new_new_leader == ch)
  {
    ch->sendln("You truly must have an EGO the size of the Tarrasque!");
    send_to_char("You are already the group leader! Maybe you SHOULD "
                 "step down?\n\r",
                 ch);
    return eFAILURE;
  }

  if (IS_NPC(new_new_leader))
  {
    ch->sendln("Yeah right!");
    return eFAILURE;
  }

  if (!ARE_GROUPED(ch, new_new_leader))
  {
    ch->sendln("But you aren't even in the same group!");
    return eFAILURE;
  }

  sprintf(buf, "You step down, appointing %s as the new leader.\r\n",
          GET_SHORT(new_new_leader));
  ch->send(buf);
  sprintf(buf, "%s steps down as leader of: %s\n\r%s appoints YOU as "
               "the New Leader of: %s\n\r",
          GET_SHORT(ch), ch->group_name,
          GET_SHORT(ch), ch->group_name);
  new_new_leader->send(buf);
  sprintf(buf, "%s steps down as leader of: %s\n\r%s appoints %s as "
               "the New Leader of: %s",
          GET_SHORT(ch), ch->group_name,
          GET_SHORT(ch), GET_SHORT(new_new_leader), ch->group_name);
  act(buf, ch, 0, new_new_leader, TO_ROOM, NOTVICT);

  if (IS_PC(ch) && IS_PC(new_new_leader))
  {
    new_new_leader->player->grpplvl = ch->player->grpplvl;
    new_new_leader->player->group_pkills = ch->player->group_pkills;
    new_new_leader->player->group_kills = ch->player->group_kills;
    ch->player->group_pkills = 0;
    ch->player->grpplvl = 0;
    ch->player->group_kills = 0;
  }

  if (ch->group_name)
  {
    new_new_leader->group_name = ch->group_name;
    ch->group_name = nullptr;
  }
  else
    new_new_leader->group_name = str_dup("I am a dork");

  stop_follower(new_new_leader, follower_reasons_t::CHANGE_LEADER);

  for (f = ch->followers; f; f = next_f)
  {
    next_f = f->next;
    if (IS_PC(f->follower))
    {
      k = f->follower;
      stop_follower(k, follower_reasons_t::CHANGE_LEADER);
      add_follower(k, new_new_leader, follower_reasons_t::CHANGE_LEADER);
    }
    else
      REMBIT(f->follower->affected_by, AFF_GROUP);
  }

  add_follower(ch, new_new_leader, follower_reasons_t::CHANGE_LEADER);
  return eSUCCESS;
}

int do_disband(Character *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH + 1];
  char buf[200];
  Character *adios, *k;
  struct follow_type *f, *next_f;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  one_argument(argument, name);

  if (ch->master)
  {
    ch->sendln("You aren't running the show here pal!");
    return eFAILURE;
  }

  if ((!ch->master) && (!IS_AFFECTED(ch, AFF_GROUP)))
  {
    ch->sendln("You don't even have a group to disband.");
    return eFAILURE;
  }

  if (!*name)
  {
    ch->sendln("Who do you wish to disband? ");
    ch->sendln("Disband 'all' will disband the group.");
    return eFAILURE;
  }

  if (isexact(name, "all"))
  {
    k = ch;
    sprintf(buf, "You disband your group: %s", k->group_name);
    act(buf, k, 0, 0, TO_CHAR, 0);
    sprintf(buf, "$n disbands $s group: %s", k->group_name);
    act(buf, k, 0, 0, TO_ROOM, 0);

    dc_free(k->group_name);
    k->group_name = 0;

    if (IS_PC(k))
    {
      k->player->group_pkills = 0;
      k->player->grpplvl = 0;
      k->player->group_kills = 0;
    }

    for (f = k->followers; f; f = next_f)
    {
      next_f = f->next;
      if (IS_PC(f->follower))
      {
        stop_grouped_bards(f->follower, 1);
        stop_follower(f->follower);
      }
      else
        REMBIT(f->follower->affected_by, AFF_GROUP);
    }

    REMBIT(k->affected_by, AFF_GROUP);
    return eSUCCESS;
  }

  if (!(adios = ch->get_char_room_vis(name)))
  {
    ch->sendln("I see no person by that name here!");
    return eFAILURE;
  }

  if (adios == ch)
  {
    ch->sendln("You can't disband yourself from a group you're leading!");
    ch->sendln("Either Promote someone to Group Leader, or Disband All.");
    return eFAILURE;
  }

  if (adios->master != ch)
  {
    ch->sendln("Try someone in YOUR group, fartknocker.");
    return eFAILURE;
  }

  if ((IS_NPC(adios)) && (IS_AFFECTED(adios, AFF_CHARM)))
  {
    ch->sendln("Can't kick out a charmee.");
    return eFAILURE;
  }

  stop_grouped_bards(adios, 1);
  if (IS_PC(adios))
  {
    adios->player->grpplvl = 0;
    adios->player->group_pkills = 0;
    adios->player->group_kills = 0;
  }
  stop_follower(adios);
  return eSUCCESS;
}

int do_follow(Character *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH + 1];
  Character *leader;

  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eFAILURE;
  }

  one_argument(argument, name);

  if (*name)
  {
    if (!(leader = get_char_room(name, ch->in_room)))
    {
      ch->sendln("I see no person by that name here!");
      return eFAILURE;
    }
  }
  else
  {
    ch->sendln("Who do you wish to follow?");
    return eFAILURE;
  }
  if (cmd == 9 && !CAN_SEE(ch, leader))
  { // check it like this instead o' get_char_room_vis 'cause stalk checks.
    ch->sendln("I see no person by that name here!");
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_GROUP))
  {
    ch->sendln("You must first abandon your group.");
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master))
  {
    act("But you only feel like following $N!", ch, 0, ch->master, TO_CHAR, 0);
  }
  else
  { /* Not Charmed follow person */

    if (leader == ch)
    {
      if (!ch->master)
      {
        ch->sendln("You are already following yourself.");
        return eFAILURE;
      }
      stop_follower(ch);
    }
    else
    {
      if (circle_follow(ch, leader))
      {
        act("Sorry, but following in 'loops' is not allowed.",
            ch, 0, 0, TO_CHAR, 0);
        return eFAILURE;
      }
      if (ch->master)
      {
        if (cmd == 10)
          stop_follower(ch, follower_reasons_t::END_STALK); /* stalk  */
        else
          stop_follower(ch); /* follow */
      }

      //	    if((abs(ch->getLevel()-leader->getLevel())<60) || ch->getLevel()>=IMMORTAL) {
      if (cmd == 10)
        add_follower(ch, leader, follower_reasons_t::END_STALK); /* stalk  */
      else
        add_follower(ch, leader); /* follow */
                                  //          }
                                  //	    else
                                  //	      {
                                  //		act("Sorry, but you are not of the right caliber to follow.",
                                  //		ch, 0, 0, TO_CHAR, 0);
                                  //		return eFAILURE;
                                  //	      }
    }
  }
  return eSUCCESS;
}

command_return_t do_autojoin(Character *ch, std::string str_arguments, int cmd)
{
  if (ch->player == nullptr)
  {
    return eFAILURE;
  }

  QString arguments = QString::fromStdString(str_arguments).trimmed().toLower();

  if (arguments.isEmpty())
  {
    if (ch->player->joining.empty())
    {
      ch->send("You are not configured to auto-joining anyone.\r\n");
    }
    else
    {
      ch->send(QStringLiteral("You are configured to auto-join: %1\r\n").arg(ch->player->getJoining()));
    }

    ch->send("Syntax: autojoin <player1> <player2> <player3> ...\r\n");

    return eFAILURE;
  }

  else if (arguments == "clear")
  {
    ch->player->joining.clear();
    ch->send("Auto-join list cleared.\r\n");
    return eSUCCESS;
  }

  auto parts = arguments.toLower().split(' ');
  for (const auto &i : parts)
  {
    ch->player->toggleJoining(i);
  }

  ch->send(QStringLiteral("You are now autojoining: %1\r\n").arg(ch->player->getJoining()));
  return eSUCCESS;
}

std::vector<Character *> Character::getFollowers(void)
{
  std::vector<Character *> followers = {};
  follow_type *f = {};
  Character *leader = nullptr;

  if (!IS_AFFECTED(this, AFF_GROUP))
  {
    return followers;
  }

  if (this->master != nullptr)
  {
    leader = this->master;
  }
  else
  {
    leader = this;
  }
  followers.push_back(leader);

  for (f = leader->followers; f != nullptr; f = f->next)
  {
    if (IS_AFFECTED(f->follower, AFF_GROUP))
    {
      followers.push_back(f->follower);
    }
  }

  return followers;
}
