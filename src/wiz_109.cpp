/********************************
| Level 109 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/DC.h"

command_return_t Character::do_linkload(QStringList arguments, cmd_t cmd)
{
  CharacterPtr new_new;

  if (arguments.isEmpty())
  {
    sendln("Linkload whom?");
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0).trimmed();

  if (get_pc(arg1))
  {
    sendln("That person is already on the game!");
    return ReturnValue::eFAILURE;
  }

  auto result = dc_->load_char_obj(arg1);
  if (!result || !result.value())
  {
    sendln("Unable to load! (character might not exist...)");
    return ReturnValue::eFAILURE;
  }
  auto conn = result.value();

  new_new = conn->character;
  new_new->conn_ = {};
  auto &character_list = dc_->character_list;
  character_list.insert(new_new);
  new_new->add_to_bard_list();

  redo_hitpoints(new_new);
  redo_mana(new_new);
  if (!GET_TITLE(new_new))
    new_new->title = u"is a virgin"_s;
  if (GET_CLASS(new_new) == CLASS_MONK)
    GET_AC(new_new) -= new_new->getLevel() * 3;
  isr_set(new_new);

  char_to_room(new_new, in_room);
  act_to_room("$n gestures sharply and $N comes into existence!", this, 0, new_new, 0);
  act_to_character("You linkload $N.", this, 0, new_new, 0);
  dc_->logf(level_t, DC::LogChannel::LOG_GOD, "%s linkloads %s.", qPrintable(name()), qPrintable(new_new->name()));
  return ReturnValue::eSUCCESS;
}

command_return_t do_processes(CharacterPtr ch, QString arg, cmd_t cmd)
{
  FILE *stream;
  QString tmp;
  QString buf;

  dc_strcpy(buf, "ps -ux > ../lib/whassup.txt");

  system(buf);

  if (!(stream = fopen("../lib/whassup.txt", "a")))
  {
    dc_->logentry(u"Unable to open whassup.txt for adding in do_processes!"_s, IMPLEMENTER,
                  DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }
  if (dc_fprintf(stream, "~\n") < 0)
  {

    ch->sendln("Failure writing to transition file.");
    return ReturnValue::eFAILURE;
  }

  if (!(stream = fopen("../lib/whassup.txt", "r")))
  {
    dc_->logentry(u"Unable to open whassup.txt for reading in do_processes!"_s, IMPLEMENTER,
                  DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }
  tmp = fread_string(stream);

  ch->send(tmp);
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_guide(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim = {};
  QString name = arguments.value(0);

  if (!name.isEmpty())
  {
    if (!(victim = get_pc_vis(this, name)))
    {
      sendln("That player is not here.");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    sendln("Who exactly would you like to be a guide?");
    return ReturnValue::eFAILURE;
  }

  if (!isSet(victim->player->toggles, Player::PLR_GUIDE))
  {
    send(u"%1 is now a guide.\r\n"_s.arg(qPrintable(victim->name())));
    victim->sendln("You have been selected to be a DC Guide!");
    SET_BIT(victim->player->toggles, Player::PLR_GUIDE);
    SET_BIT(victim->player->toggles, Player::PLR_GUIDE_TOG);
  }
  else
  {
    send(u"%1 is no longer a guide.\r\n"_s.arg(qPrintable(victim->name())));
    victim->sendln("You have been removed as a DC guide.");
    REMOVE_BIT(victim->player->toggles, Player::PLR_GUIDE);
    REMOVE_BIT(victim->player->toggles, Player::PLR_GUIDE_TOG);
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_advance(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name, level, buf, passwd;
  qint32 new_newlevel;

  void gain_exp(CharacterPtr ch, qint32 gain);

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  half_chop(argument, name, buf);
  argument_interpreter(buf, level, passwd);

  if (!name.isEmpty())
  {
    if (!(victim = get_char_vis(ch, name)))
    {
      ch->sendln("That player is not here.");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    ch->sendln("Advance whom?");
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer())
  {
    ch->sendln("NO! Not on NPC's.");
    return ReturnValue::eFAILURE;
  }

  if (level.isEmpty() ||
      (new_newlevel = dc_atoi(level)) <= 0 || new_newlevel > IMPLEMENTER)
  {
    ch->sendln("Level must be 1 to 110.");
    return ReturnValue::eFAILURE;
  }

  if ((new_newlevel > DC::MAX_MORTAL_LEVEL) && (new_newlevel < MIN_GOD))
  {
    ch->sendln("That level doesn't exist!!");
    return ReturnValue::eFAILURE;
  }

  if (ch->getLevel() < OVERSEER && new_newlevel >= IMMORTAL)
  {
    ch->sendln("Limited to levels lower than Titan.");
    return ReturnValue::eFAILURE;
  }

  /* Who the fuck took ths out in the first place? -Sadus */
  if (new_newlevel > ch->getLevel())
  {
    ch->sendln("Yeah right.");
    return ReturnValue::eFAILURE;
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
    victim->exp = 1;

    victim->max_hit = 5; /* These are BASE numbers  */
    victim->raw_hit = 5;

    victim->fillHPLimit();
    GET_MANA(victim) = mana_limit(victim);
    victim->setMove(victim->move_limit());

    advance_level(victim, 0);
    redo_hitpoints(victim);
  }

  ch->sendln("You feel generous.");
  act("$n makes some strange gestures.\r\nA strange feeling comes upon you,"
      "like a giant hand. Light comes\r\ndown from above, grabbing your "
      "body, which begins to pulse\r\nwith coloured lights from inside.\r\nYo"
      "ur head seems to be filled with deamons\r\nfrom another plane as your"
      " body dissolves\r\nto the elements of time and space itself.\r\nSudde"
      "nly a silent explosion of light snaps\r\nyou back to reality. You fee"
      "l slightly\r\ndifferent.",
      ch, 0, victim, TO_VICT, 0);

  dc_sprintf(buf, "%s advances %s to level %d.", qPrintable(ch->name()),
             qPrintable(victim->name()), new_newlevel);
  dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);

  if (victim->getLevel() == 0)
    do_start(victim);
  else
    while (victim->getLevel() < new_newlevel)
    {
      victim->send("You raise a level!!  ");
      victim->incrementLevel();
      advance_level(victim, 0);
    }
  dc_->update_wizlist(victim);
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_zap(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim = {};
  qint32 room = {};

  void remove_clan_member(qint32 clannumber, CharacterPtr ch);

  QString victim_name = arguments.value(0);

  if (victim_name.isEmpty())
  {
    send_to_char("Zap who??\r\nOh, BTW this deletes anyone "
                 "lower than you.\r\n",
                 this);
    return ReturnValue::eFAILURE;
  }

  victim = get_pc_vis(this, victim_name);

  if (victim)
  {
    if (victim->isPlayer() && (getLevel() < victim->getLevel()))
    {
      act("$n casts a massive lightning bolt at you.", this, 0, victim, TO_VICT, 0);
      act_to_room("$n casts a massive lightning bolt at $N.", this, 0, victim, NOTVICT);
      return ReturnValue::eFAILURE;
    }

    if (victim->getLevel() == IMPLEMENTER)
    { // Hehe..
      sendln("Get stuffed.");
      return ReturnValue::eFAILURE;
    }

    if (victim->isPlayer())
    {
      victim->sendln(QString("A massive bolt of lightning arcs down from the "
                             "heavens, striking you\r\nbetween the eyes. You have "
                             "been utterly destroyed by %1.")
                         .arg(qPrintable(shortdesc_or_name())));
    }

    room = victim->in_room;

    QString buf = QString("A massive bolt of lightning arcs down from the heavens,"
                          " striking\r\n%1 between the eyes.\r\n  %2 has been utterly"
                          " destroyed by %3.\r\n")
                      .arg(victim->name())
                      .arg(qPrintable(victim->shortdesc_or_name()))
                      .arg(qPrintable(shortdesc_or_name()));

    remove_familiars(qPrintable(victim->name()), ZAPPED);
    if (cmd == cmd_t::DEFAULT) // cmd_t::DEFAULT = someone typed it. cmd_t::TRACK = rename.
      dc_->vaults_.remove_vault(qPrintable(victim->name()), ZAPPED);

    victim->setLevel(1);
    dc_->update_wizlist(victim);

    if (clan)
      remove_clan_member(clan, this);

    dc_->TheAuctionHouse.HandleDelete(victim->name());

    do_quit(victim, "", cmd_t::SAVE_SILENTLY);
    remove_character(victim->name(), ZAPPED);

    send_to_room(buf, room);
    send_to_all("You hear an ominous clap of thunder in the distance.\r\n");
    dc_->logentry(u"%1 has deleted %2.\r\n"_s.arg(name()).arg(victim->name()), ANGEL, DC::LogChannel::LOG_GOD);
  }

  else
  {
    send_to_char("Zap who??\r\nOh, BTW this deletes anyone "
                 "lower than you.\r\n",
                 this);
  }

  return ReturnValue::eFAILURE;
}

command_return_t do_global(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 i;
  QString buf;
  ConnectionPtr point;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  for (i = {}; *(argument + i) == ' '; i++)
    ;

  if (!*(argument + i))
    ch->sendln("What message do you want to send to all players?");
  else
  {
    dc_sprintf(buf, "\r\n%s\r\n", argument + i);
    for (point = dc_->connections_; point; point = point->next)
      if (!point->connected && point->character)
        point->character->send(buf);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_shutdown(QStringList arguments, cmd_t cmd)
{
  extern qint32 _shutdown;
  extern qint32 try_to_hotboot_on_crash;
  extern command_return_t do_not_save_corpses;
  QString *new_argv = {};

  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (!has_skill(COMMAND_SHUTDOWN))
  {
    sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    send_to_char("Syntax:  shutdown [sub command] [options ...]\r\n"
                 " Sub Commands:\r\n"
                 "--------------\r\n"
                 "   hot - Rerun current DC filename and keep players' links active.\r\n"
                 "         Options: [path/dc executable] [dc options ...]\r\n"
                 "  cold - Go ahead and kill the links.\r\n"
                 " crash - Crash the mud by referencing an invalid pointer.\r\n"
                 "  core - Produce a core file.\r\n"
                 "  auto - Toggle auto-hotboot on crash setting.\r\n"
                 "   die - Kill boot script and crash mud so it won't reboot.\r\n",
                 this);
    return ReturnValue::eFAILURE;
  }

  if (arg1 == "cold")
  {
    QString buffer = u"Shutdown by %1.\r\n"_s.arg(qPrintable(shortdesc_or_name()));
    send_to_all(buffer);
    dc_->logentry(buffer, ANGEL, DC::LogChannel::LOG_GOD);
    _shutdown = 1;
    dc_->quit();
  }
  else if (arg1 == "hot")
  {
    for (const auto &victim : dc_->character_list)
    {
      if (victim->isPlayer())
      {
        QList<CharacterPtr> followers = victim->getFollowers();
        for (const auto &follower : followers)
        {
          if (follower->isNonPlayer() && IS_AFFECTED(follower, AFF_CHARM))
          {
            if (follower->carrying != nullptr)
            {
              send(u"Player %1 has charmie %2 with equipment.\r\n"_s.arg(qPrintable(victim->name())).arg(qPrintable(follower->name())));
              return ReturnValue::eFAILURE;
            }
          }
        }
      }
    }

    do_not_save_corpses = 1;
    QString buffer = u"Hot reboot by %1.\r\n"_s.arg(qPrintable(shortdesc_or_name()));
    send_to_all(buffer);
    dc_->logentry(buffer, ANGEL, DC::LogChannel::LOG_GOD);
    dc_->logentry(u"Writing sockets to file for hotboot recovery."_s, 0, DC::LogChannel::LOG_MISC);
    do_force({u"all"_s, u"save"_s});
    if (!dc_->write_hotboot_file())
    {
      dc_->logentry(u"Hotboot failed.  Closing all sockets."_s, 0, DC::LogChannel::LOG_MISC);
      sendln("Hot reboot failed.");
    }
  }
  else if (arg1 == "auto")
  {
    if (try_to_hotboot_on_crash)
    {
      sendln("Mud will not try to hotboot when it crashes next.");
      try_to_hotboot_on_crash = {};
    }
    else
    {
      sendln("Mud will now TRY to hotboot when it crashes next.");
      try_to_hotboot_on_crash = 1;
    }
  }
  else if (arg1 == "crash")
  {
    // let's crash the mud!
    CharacterPtr crashus = {};
    if (crashus->in_room == DC::NOWHERE)
    {
      return ReturnValue::eFAILURE; // this should never be reached
    }
  }
  else if (arg1 == "core")
  {
    produce_coredump(this);
    dc_->logentry(u"Corefile produced."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
  }
  else if (arg1 == "die")
  {
    fclose(fopen("died_in_bootup", "w"));
    try_to_hotboot_on_crash = {};

    // let's crash the mud!
    CharacterPtr crashus = {};
    if (crashus->in_room == DC::NOWHERE)
    {
      return ReturnValue::eFAILURE; // this should never be reached
    }
  }
  else if (arg1 == "check")
  {
    for (const auto &victim : dc_->character_list)
    {
      if (victim->isPlayer())
      {
        QList<CharacterPtr> followers = victim->getFollowers();
        for (const auto &follower : followers)
        {
          if (follower->isNonPlayer())
          {
            if (IS_AFFECTED(follower, AFF_CHARM))
            {
              if (follower->carrying != nullptr || follower->equipment[0] != nullptr)
              {
                send(u"Player %1 has charmie %2 with equipment. Use Force to override.\r\n"_s.arg(qPrintable(victim->name())).arg(qPrintable(follower->name())));
                return ReturnValue::eFAILURE;
              }
            }
          }
        }
      }
    }
    send("Ok.\r\n");
    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_shutdow(QStringList arguments, cmd_t cmd)
{
  if (!has_skill(COMMAND_SHUTDOWN))
  {
    sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  sendln("If you want to shut something down - say so!");
  return ReturnValue::eSUCCESS;
}

command_return_t do_testport(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 errnosave = {};
  static pid_t child = {};
  QString arg1;

  if (ch == nullptr)
  {
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer() || !ch->has_skill(COMMAND_TESTPORT))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, arg1);

  if (*arg1 == 0)
  {
    ch->sendln("testport <start | stop>\r\n");
    return ReturnValue::eFAILURE;
  }

  if (!str_cmp(arg1, "start"))
  {
    child = fork();
    if (child == 0)
    {
      system("sudo -n /usr/bin/systemctl start --no-block dcastle_test");
      exit(0);
    }

    dc_->logf(105, DC::LogChannel::LOG_MISC, "Starting testport.");
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

    dc_->logf(105, DC::LogChannel::LOG_MISC, "Shutdown testport under pid %d", child);
    ch->sendln("Testport successfully shutdown.");
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_testuser(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1;
  QString arg2;
  QString savefile;
  QString bsavefile;
  QString command;
  QString username;

  if (ch == nullptr)
  {
    return ReturnValue::eFAILURE;
  }

  if (ch->isNonPlayer() || !ch->has_skill(COMMAND_TESTUSER))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, arg1);
  one_argument(argument, arg2);

  if (*arg1 == 0 || *arg2 == 0)
  {
    ch->sendln("testuser <user> <on|off>\r\n");
    return ReturnValue::eFAILURE;
  }

  if (dc_strlen(arg1) > 19 || _parse_name(arg1, username))
  {
    ch->sendln("Invalid username passed.");
    return ReturnValue::eFAILURE;
  }

  username[0] = UPPER(username[0]);
  for (quint32 i = 1; i < dc_strlen(username); i++)
  {
    username[i] = LOWER(username[i]);
  }

  dc_snprintf(savefile, 255, "../save/%c/%s", UPPER(username[0]), username);
  dc_snprintf(bsavefile, 255, "/srv/dcastle3/save/%c/%s", UPPER(username[0]), username);

  if (!file_exists(savefile))
  {
    ch->sendln("Player file not found.");
    return ReturnValue::eFAILURE;
  }

  if (!str_cmp(arg2, "on"))
  {
    dc_sprintf(command, "cp %s %s", savefile, bsavefile);
  }
  else if (!str_cmp(arg2, "off"))
  {
    dc_sprintf(command, "rm %s", bsavefile);
  }
  else
  {
    ch->sendln("Only on or off are valid second arguments to this command.");
    return ReturnValue::eFAILURE;
  }

  dc_->logf(110, DC::LogChannel::LOG_GOD, "testuser: %s initiated %s", qPrintable(ch->name()), command);

  if (system(command))
  {
    ch->sendln("Error occurred.");
  }
  else
  {
    ch->sendln("Ok.");
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_bandwidth(CharacterPtr ch, QString argument, cmd_t cmd)
{
  ch->send(u"Bytes sent in %ld seconds: %ld\r\n"_s.arg(get_bandwidth_start()).arg(get_bandwidth_amount()));
  return ReturnValue::eSUCCESS;
}

command_return_t do_skilledit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  QString type;
  QString value;

  if (!(*argument))
  {
    send_to_char("Syntax:  skilledit <character> <action> <value>\r\n"
                 "Possible actions are:  list, add, delete\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }
  half_chop(argument, name, argument);
  half_chop(argument, type, value);

  if (!(victim = get_pc_vis(ch, name)))
  {
    ch->sendln("Edit the skills of whom?");
    return ReturnValue::eFAILURE;
  }

  if (isexact(type, "list"))
  {
    if (victim->skills.isEmpty())
    {
      ch->send(fmt::format("{} has no skills.\r\n", qPrintable(victim->name())));
      return ReturnValue::eSUCCESS;
    }

    ch->send(fmt::format("Skills for {}:\r\n", qPrintable(victim->name())));
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

  return ReturnValue::eSUCCESS;
}
