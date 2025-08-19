/********************************
| Level 109 wizard commands
| 11/20/95 -- Azrack
**********************/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <signal.h>

#include <fmt/format.h>

#include "DC/wizard.h"
#include "DC/spells.h"
#include "DC/fileinfo.h"
#include "DC/connect.h"
#include "DC/utility.h"
#include "DC/player.h"

#include "DC/mobile.h"
#include "DC/handler.h"
#include "DC/interp.h"
#include "DC/db.h"
#include "DC/returnvals.h"
#include "DC/comm.h"
#include "DC/vault.h"
#include "DC/utility.h"

#ifdef BANDWIDTH
#include "DC/bandwidth.h"
#endif

command_return_t Character::do_linkload(QStringList arguments, int cmd)
{
  class Connection d;
  Character *new_new;

  if (arguments.isEmpty())
  {
    this->sendln("Linkload whom?");
    return eFAILURE;
  }

  QString arg1 = arguments.value(0).trimmed();

  if (get_pc(arg1))
  {
    this->sendln("That person is already on the game!");
    return eFAILURE;
  }

  if (!(load_char_obj(&d, arg1)))
  {
    this->sendln("Unable to load! (Character might not exist...)");
    return eFAILURE;
  }

  new_new = d.character;
  new_new->desc = 0;
  auto &character_list = DC::getInstance()->character_list;
  character_list.insert(new_new);
  new_new->add_to_bard_list();

  redo_hitpoints(new_new);
  redo_mana(new_new);
  if (!GET_TITLE(new_new))
    GET_TITLE(new_new) = str_dup("is a virgin");
  if (GET_CLASS(new_new) == CLASS_MONK)
    GET_AC(new_new) -= new_new->getLevel() * 3;
  isr_set(new_new);

  char_to_room(new_new, in_room);
  act("$n gestures sharply and $N comes into existence!", this, 0, new_new, TO_ROOM, 0);
  act("You linkload $N.", this, 0, new_new, TO_CHAR, 0);
  logf(level_, DC::LogChannel::LOG_GOD, "%s linkloads %s.", GET_NAME(this), GET_NAME(new_new));
  return eSUCCESS;
}

int do_processes(Character *ch, char *arg, int cmd)
{
  FILE *fl;
  char *tmp;
  char buf[100];

  strcpy(buf, "ps -ux > ../lib/whassup.txt");

  system(buf);

  if (!(fl = fopen("../lib/whassup.txt", "a")))
  {
    logentry(QStringLiteral("Unable to open whassup.txt for adding in do_processes!"), IMPLEMENTER,
             DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }
  if (fprintf(fl, "~\n") < 0)
  {
    fclose(fl);
    ch->sendln("Failure writing to transition file.");
    return eFAILURE;
  }

  fclose(fl);

  if (!(fl = fopen("../lib/whassup.txt", "r")))
  {
    logentry(QStringLiteral("Unable to open whassup.txt for reading in do_processes!"), IMPLEMENTER,
             DC::LogChannel::LOG_BUG);
    return eFAILURE;
  }
  tmp = fread_string(fl, 0);
  fclose(fl);

  ch->send(tmp);
  FREE(tmp);
  return eSUCCESS;
}

command_return_t Character::do_guide(QStringList arguments, int cmd)
{
  Character *victim{};
  QString name = arguments.value(0);

  if (!name.isEmpty())
  {
    if (!(victim = get_pc_vis(this, name)))
    {
      this->sendln("That player is not here.");
      return eFAILURE;
    }
  }
  else
  {
    this->sendln("Who exactly would you like to be a guide?");
    return eFAILURE;
  }

  if (!isSet(victim->player->toggles, Player::PLR_GUIDE))
  {
    send(QStringLiteral("%1 is now a guide.\r\n").arg(victim->getNameC()));
    victim->sendln("You have been selected to be a DC Guide!");
    SET_BIT(victim->player->toggles, Player::PLR_GUIDE);
    SET_BIT(victim->player->toggles, Player::PLR_GUIDE_TOG);
  }
  else
  {
    send(QStringLiteral("%1 is no longer a guide.\r\n").arg(victim->getNameC()));
    victim->sendln("You have been removed as a DC guide.");
    REMOVE_BIT(victim->player->toggles, Player::PLR_GUIDE);
    REMOVE_BIT(victim->player->toggles, Player::PLR_GUIDE_TOG);
  }

  return eSUCCESS;
}

int do_advance(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[100], level[100], buf[300], passwd[100];
  int new_newlevel;

  void gain_exp(Character * ch, int gain);

  if (IS_NPC(ch))
    return eFAILURE;

  half_chop(argument, name, buf);
  argument_interpreter(buf, level, passwd);

  if (*name)
  {
    if (!(victim = get_char_vis(ch, name)))
    {
      ch->sendln("That player is not here.");
      return eFAILURE;
    }
  }
  else
  {
    ch->sendln("Advance whom?");
    return eFAILURE;
  }

  if (IS_NPC(victim))
  {
    ch->sendln("NO! Not on NPC's.");
    return eFAILURE;
  }

  if (!*level ||
      (new_newlevel = atoi(level)) <= 0 || new_newlevel > IMPLEMENTER)
  {
    ch->sendln("Level must be 1 to 110.");
    return eFAILURE;
  }

  if ((new_newlevel > DC::MAX_MORTAL_LEVEL) && (new_newlevel < MIN_GOD))
  {
    ch->sendln("That level doesn't exist!!");
    return eFAILURE;
  }

  if (ch->getLevel() < OVERSEER && new_newlevel >= IMMORTAL)
  {
    ch->sendln("Limited to levels lower than Titan.");
    return eFAILURE;
  }

  /* Who the fuck took ths out in the first place? -Sadus */
  if (new_newlevel > ch->getLevel())
  {
    ch->sendln("Yeah right.");
    return eFAILURE;
  }

  /* Lower level:  First reset the player to level 1. Remove all special
     abilities (all spells, BASH, STEAL, et).  Reset practices to
     zero.  Then act as though you're raising a first level character to
     a higher level.  Note, currently, an implementor can lower another imp.
     -- Swifttest */

  if (new_newlevel <= victim->getLevel())
  {
    ch->sendln("Warning:  Lowering a player's level!");

    victim->setLevel(1);
    GET_EXP(victim) = 1;

    victim->max_hit = 5; /* These are BASE numbers  */
    victim->raw_hit = 5;

    victim->fillHPLimit();
    GET_MANA(victim) = mana_limit(victim);
    victim->setMove(victim->move_limit());

    advance_level(victim, 0);
    redo_hitpoints(victim);
  }

  ch->sendln("You feel generous.");
  act("$n makes some strange gestures.\n\rA strange feeling comes upon you,"
      "like a giant hand. Light comes\n\rdown from above, grabbing your "
      "body, which begins to pulse\n\rwith coloured lights from inside.\n\rYo"
      "ur head seems to be filled with deamons\n\rfrom another plane as your"
      " body dissolves\n\rto the elements of time and space itself.\n\rSudde"
      "nly a silent explosion of light snaps\n\ryou back to reality. You fee"
      "l slightly\n\rdifferent.",
      ch, 0, victim, TO_VICT, 0);

  sprintf(buf, "%s advances %s to level %d.", GET_NAME(ch),
          victim->getNameC(), new_newlevel);
  logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);

  if (victim->getLevel() == 0)
    do_start(victim);
  else
    while (victim->getLevel() < new_newlevel)
    {
      victim->send("You raise a level!!  ");
      victim->incrementLevel();
      advance_level(victim, 0);
    }
  DC::getInstance()->update_wizlist(victim);
  return eSUCCESS;
}

command_return_t Character::do_zap(QStringList arguments, int cmd)
{
  Character *victim{};
  int room{};

  void remove_clan_member(int clannumber, Character *ch);

  QString name = arguments.value(0);

  if (name.isEmpty())
  {
    send_to_char("Zap who??\n\rOh, BTW this deletes anyone "
                 "lower than you.\r\n",
                 this);
    return eFAILURE;
  }

  victim = get_pc_vis(this, name);

  if (victim)
  {
    if (IS_PC(victim) && (this->getLevel() < victim->getLevel()))
    {
      act("$n casts a massive lightning bolt at you.", this, 0, victim,
          TO_VICT, 0);
      act("$n casts a massive lightning bolt at $N.", this, 0, victim,
          TO_ROOM, NOTVICT);
      return eFAILURE;
    }

    if (victim->getLevel() == IMPLEMENTER)
    { // Hehe..
      this->sendln("Get stuffed.");
      return eFAILURE;
    }

    if (IS_PC(victim))
    {
      victim->sendln(QString("A massive bolt of lightning arcs down from the "
                             "heavens, striking you\n\rbetween the eyes. You have "
                             "been utterly destroyed by %1.")
                         .arg(GET_SHORT(this)));
    }

    room = victim->in_room;

    QString buf = QString("A massive bolt of lightning arcs down from the heavens,"
                          " striking\r\n%1 between the eyes.\r\n  %2 has been utterly"
                          " destroyed by %3.\r\n")
                      .arg(victim->getName())
                      .arg(GET_SHORT(victim))
                      .arg(GET_SHORT(this));

    remove_familiars(victim->getNameC(), ZAPPED);
    if (cmd == CMD_DEFAULT) // cmd9 = someone typed it. 10 = rename.
      remove_vault(victim->getNameC(), ZAPPED);

    victim->setLevel(1);
    DC::getInstance()->update_wizlist(victim);

    if (this->clan)
      remove_clan_member(this->clan, this);

    DC::getInstance()->TheAuctionHouse.HandleDelete(victim->getName());

    do_quit(victim, "", 666);
    remove_character(victim->getName(), ZAPPED);

    send_to_room(buf, room);
    send_to_all("You hear an ominous clap of thunder in the distance.\r\n");
    logentry(QStringLiteral("%1 has deleted %2.\r\n").arg(getName()).arg(victim->getName()), ANGEL, DC::LogChannel::LOG_GOD);
  }

  else
  {
    send_to_char("Zap who??\n\rOh, BTW this deletes anyone "
                 "lower than you.\r\n",
                 this);
  }

  return eFAILURE;
}

int do_global(Character *ch, char *argument, int cmd)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  class Connection *point;

  if (IS_NPC(ch))
    return eFAILURE;

  for (i = 0; *(argument + i) == ' '; i++)
    ;

  if (!*(argument + i))
    ch->sendln("What message do you want to send to all players?");
  else
  {
    sprintf(buf, "\n\r%s\n\r", argument + i);
    for (point = DC::getInstance()->descriptor_list; point; point = point->next)
      if (!point->connected && point->character)
        point->character->send(buf);
  }
  return eSUCCESS;
}

command_return_t Character::do_shutdown(QStringList arguments, int cmd)
{
  extern int _shutdown;
  extern int try_to_hotboot_on_crash;
  extern int do_not_save_corpses;
  char **new_argv = 0;

  if (IS_NPC(this))
    return eFAILURE;

  if (!has_skill(COMMAND_SHUTDOWN))
  {
    this->sendln("Huh?");
    return eFAILURE;
  }

  QString arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    send_to_char("Syntax:  shutdown [sub command] [options ...]\n\r"
                 " Sub Commands:\n\r"
                 "--------------\n\r"
                 "   hot - Rerun current DC filename and keep players' links active.\r\n"
                 "         Options: [path/dc executable] [dc options ...]\n\r"
                 "  cold - Go ahead and kill the links.\r\n"
                 " crash - Crash the mud by referencing an invalid pointer.\r\n"
                 "  core - Produce a core file.\r\n"
                 "  auto - Toggle auto-hotboot on crash setting.\r\n"
                 "   die - Kill boot script and crash mud so it won't reboot.\r\n",
                 this);
    return eFAILURE;
  }

  if (arg1 == "cold")
  {
    QString buffer = QStringLiteral("Shutdown by %1.\r\n").arg(GET_SHORT(this));
    send_to_all(buffer);
    logentry(buffer, ANGEL, DC::LogChannel::LOG_GOD);
    _shutdown = 1;
    DC::getInstance()->quit();
  }
  else if (arg1 == "hot")
  {
    for (const auto &victim : DC::getInstance()->character_list)
    {
      if (IS_PC(victim))
      {
        std::vector<Character *> followers = victim->getFollowers();
        for (const auto &follower : followers)
        {
          if (IS_NPC(follower) && IS_AFFECTED(follower, AFF_CHARM))
          {
            if (follower->carrying != nullptr)
            {
              send(QStringLiteral("Player %1 has charmie %2 with equipment.\r\n").arg(victim->getNameC()).arg(GET_NAME(follower)));
              return eFAILURE;
            }
          }
        }
      }
    }

    do_not_save_corpses = 1;
    QString buffer = QStringLiteral("Hot reboot by %1.\r\n").arg(GET_SHORT(this));
    send_to_all(buffer);
    logentry(buffer, ANGEL, DC::LogChannel::LOG_GOD);
    logentry(QStringLiteral("Writing sockets to file for hotboot recovery."), 0, DC::LogChannel::LOG_MISC);
    do_force(this, "all save");
    if (!DC::getInstance()->write_hotboot_file())
    {
      logentry(QStringLiteral("Hotboot failed.  Closing all sockets."), 0, DC::LogChannel::LOG_MISC);
      this->sendln("Hot reboot failed.");
    }
  }
  else if (arg1 == "auto")
  {
    if (try_to_hotboot_on_crash)
    {
      this->sendln("Mud will not try to hotboot when it crashes next.");
      try_to_hotboot_on_crash = 0;
    }
    else
    {
      this->sendln("Mud will now TRY to hotboot when it crashes next.");
      try_to_hotboot_on_crash = 1;
    }
  }
  else if (arg1 == "crash")
  {
    // let's crash the mud!
    Character *crashus = nullptr;
    if (crashus->in_room == DC::NOWHERE)
    {
      return eFAILURE; // this should never be reached
    }
  }
  else if (arg1 == "core")
  {
    produce_coredump(this);
    logentry(QStringLiteral("Corefile produced."), IMMORTAL, DC::LogChannel::LOG_BUG);
  }
  else if (arg1 == "die")
  {
    fclose(fopen("died_in_bootup", "w"));
    try_to_hotboot_on_crash = 0;

    // let's crash the mud!
    Character *crashus = nullptr;
    if (crashus->in_room == DC::NOWHERE)
    {
      return eFAILURE; // this should never be reached
    }
  }
  else if (arg1 == "check")
  {
    for (const auto &victim : DC::getInstance()->character_list)
    {
      if (IS_PC(victim))
      {
        std::vector<Character *> followers = victim->getFollowers();
        for (const auto &follower : followers)
        {
          if (IS_NPC(follower))
          {
            if (IS_AFFECTED(follower, AFF_CHARM))
            {
              if (follower->carrying != nullptr || follower->equipment != nullptr)
              {
                send(QStringLiteral("Player %1 has charmie %2 with equipment. Use Force to override.\r\n").arg(victim->getNameC()).arg(GET_NAME(follower)));
                return eFAILURE;
              }
            }
          }
        }
      }
    }
    send("Ok.\r\n");
    return eSUCCESS;
  }
  return eSUCCESS;
}

command_return_t Character::do_shutdow(QStringList arguments, int cmd)
{
  if (!has_skill(COMMAND_SHUTDOWN))
  {
    this->sendln("Huh?");
    return eFAILURE;
  }

  this->sendln("If you want to shut something down - say so!");
  return eSUCCESS;
}

int do_testport(Character *ch, char *argument, int cmd)
{
  int errnosave = 0;
  static pid_t child = 0;
  char arg1[MAX_INPUT_LENGTH];

  if (ch == nullptr)
  {
    return eFAILURE;
  }

  if (IS_NPC(ch) || !ch->has_skill(COMMAND_TESTPORT))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  argument = one_argument(argument, arg1);

  if (*arg1 == 0)
  {
    ch->sendln("testport <start | stop>\n\r");
    return eFAILURE;
  }

  if (!str_cmp(arg1, "start"))
  {
    child = fork();
    if (child == 0)
    {
      system("sudo -n /usr/bin/systemctl start --no-block dcastle_test");
      exit(0);
    }

    logf(105, DC::LogChannel::LOG_MISC, "Starting testport.");
    ch->sendln("Testport successfully started.");
  }
  else if (!str_cmp(arg1, "stop"))
  {
    child = fork();
    if (child == 0)
    {
      system("sudo -n /usr/bin/systemctl stop --no-block dcastle_test");
      exit(0);
    }

    logf(105, DC::LogChannel::LOG_MISC, "Shutdown testport under pid %d", child);
    ch->sendln("Testport successfully shutdown.");
  }

  return eSUCCESS;
}

int do_testuser(Character *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char savefile[255];
  char bsavefile[255];
  char command[512];
  char username[20];

  if (ch == nullptr)
  {
    return eFAILURE;
  }

  if (IS_NPC(ch) || !ch->has_skill(COMMAND_TESTUSER))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  argument = one_argument(argument, arg1);
  one_argument(argument, arg2);

  if (*arg1 == 0 || *arg2 == 0)
  {
    ch->sendln("testuser <user> <on|off>\n\r");
    return eFAILURE;
  }

  if (strlen(arg1) > 19 || _parse_name(arg1, username))
  {
    ch->sendln("Invalid username passed.");
    return eFAILURE;
  }

  username[0] = UPPER(username[0]);
  for (unsigned int i = 1; i < strlen(username); i++)
  {
    username[i] = LOWER(username[i]);
  }

  snprintf(savefile, 255, "../save/%c/%s", UPPER(username[0]), username);
  snprintf(bsavefile, 255, "/srv/dcastle_test/bsave/%c/%s", UPPER(username[0]), username);

  if (!file_exists(savefile))
  {
    ch->sendln("Player file not found.");
    return eFAILURE;
  }

  if (!str_cmp(arg2, "on"))
  {
    sprintf(command, "cp %s %s", savefile, bsavefile);
  }
  else if (!str_cmp(arg2, "off"))
  {
    sprintf(command, "rm %s", bsavefile);
  }
  else
  {
    ch->sendln("Only on or off are valid second arguments to this command.");
    return eFAILURE;
  }

  logf(110, DC::LogChannel::LOG_GOD, "testuser: %s initiated %s", ch->getNameC(), command);

  if (system(command))
  {
    ch->sendln("Error occurred.");
  }
  else
  {
    ch->sendln("Ok.");
  }

  return eSUCCESS;
}

#ifdef BANDWIDTH
int do_bandwidth(Character *ch, char *argument, int cmd)
{
  csendf(ch, "Bytes sent in %ld seconds: %ld\n\r",
         get_bandwidth_start(), get_bandwidth_amount());
  return eSUCCESS;
}
#endif

int do_skilledit(Character *ch, char *argument, int cmd)
{
  Character *victim;
  char name[MAX_INPUT_LENGTH];
  char type[MAX_INPUT_LENGTH];
  char value[MAX_INPUT_LENGTH];

  if (!(*argument))
  {
    send_to_char("Syntax:  skilledit <character> <action> <value>\n\r"
                 "Possible actions are:  list, add, delete\n\r",
                 ch);
    return eFAILURE;
  }
  half_chop(argument, name, argument);
  half_chop(argument, type, value);

  if (!(victim = get_pc_vis(ch, name)))
  {
    ch->sendln("Edit the skills of whom?");
    return eFAILURE;
  }

  if (isexact(type, "list"))
  {
    if (victim->skills.empty())
    {
      ch->send(fmt::format("{} has no skills.\r\n", victim->getNameC()));
      return eSUCCESS;
    }

    ch->send(fmt::format("Skills for {}:\r\n", victim->getNameC()));
    for (const auto &curr : ch->skills)
    {
      ch->send(fmt::format("  {}  -  {}  [{}] [{}] [{}] [{}] [{}]\r\n", curr.first, curr.second.learned, curr.second.unused[0], curr.second.unused[1], curr.second.unused[2], curr.second.unused[3], curr.second.unused[4]));
    }
  }
  else if (isexact(type, "add"))
  {
  }
  else if (isexact(type, "delete"))
  {
  }
  else
  {
    ch->send(fmt::format("Invalid action '{}'.  Must be 'list', 'add', or 'delete'.\r\n", type));
  }

  return eSUCCESS;
}
