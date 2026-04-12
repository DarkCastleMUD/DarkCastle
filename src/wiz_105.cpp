/********************************
| Level 105 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/DC.h"
#include "DC/punish.h"
#include "DC/spells.h" // Character::PLAYER_CANTQUIT
#include "DC/handler.h"
#include "DC/player.h"
#include "DC/fight.h"
#include "DC/interp.h"
#include "DC/innate.h"
#include "DC/const.h"
#include "DC/Timer.h"

#include <qdebug.h>
#include <qiodevicebase.h>

#include <cstdio>

command_return_t do_clearaff(CharacterPtr ch, QString argument, cmd_t cmd)
{
  bool found = false;
  QString buf;
  CharacterPtr victim;
  affected_type *af, *afpk;
  ObjectPtr dummy;

  one_argument(argument, buf);

  if (buf.isEmpty())
    victim = ch;
  else if (!generic_find(argument, FIND_CHAR_ROOM | FIND_CHAR_WORLD, ch, &victim, &dummy, true))
    ch->send(u"Couldn't find '%1' anywhere.\r\n"_s.arg(argument));
  if (victim)
  {
    for (af = victim->affected; af; af = afpk)
    {
      found = true;
      afpk = af->next;
      QString aff_name = get_skill_name(af->type);
      if (!aff_name.isEmpty())
      {
        ch->send(u"Removing %1 affect.\r\n"_s).arg(aff_name));
      }
      else
      {
        ch->sendln("Removing unknown affect.");
      }

      affect_remove(victim, af, 0);
    }

    if (found == false)
    {
      ch->sendln("No affects found.");
    }

    //    ch->sendln("Done.");
    //  victim->sendln("Your affects have been cleared.");
    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eFAILURE;
}

command_return_t do_reloadhelp(CharacterPtr ch, QString argument, cmd_t cmd)
{
  DC::getInstance()->free_help_from_memory();
  QFile help_keyword_file(HELP_KWRD_FILE);
  if (!help_keyword_file.open(QIODeviceBase::Text))
  {
    perror(HELP_KWRD_FILE);
    abort();
  }
  QTextStream help_fl(&help_keyword_file);
  help_index = build_help_index(help_fl);

  ch->sendln("Reloaded.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_log(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict;
  ObjectPtr dummy;
  QString buf;
  QString buf2;

  if (ch->isNonPlayer() || !ch->has_skill(COMMAND_LOG))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, buf);

  if (buf.isEmpty())
  {
    ch->sendln("Log who?");
  }
  else if (!(vict = get_pc_vis(ch, buf)))
    ch->sendln("Couldn't find any such creature.");
  else if (vict->isNonPlayer())
    ch->sendln("Can't do that to a beast.");
  else if (vict->getLevel() > ch->getLevel())
    act_to_character("$E might object to that.. better not.", ch, 0, vict, 0);
  else if (isSet(vict->player->punish, PUNISH_LOG))
  {
    ch->sendln("LOG removed.");
    REMOVE_BIT(vict->player->punish, PUNISH_LOG);
    dc_sprintf(buf2, "%s removed log on %s.", qPrintable(ch->name()), qPrintable(vict->name()));
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  else
  {
    ch->sendln("LOG set.");
    SET_BIT(vict->player->punish, PUNISH_LOG);
    dc_sprintf(buf2, "%s just logged %s.", qPrintable(ch->name()), qPrintable(vict->name()));
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_showbits(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString person;
  CharacterPtr victim;
  one_argument(argument, person);

  if (person.isEmpty())
  {
    QString buf;
    const auto &character_list = DC::getInstance()->character_list;
    for (const auto &victim : character_list)
    {
      if (victim->isNonPlayer())
        continue;
      dc_sprintf(buf, "0.%s", qPrintable(victim->name()));
      do_showbits(ch, buf, cmd);
    }
    return ReturnValue::eSUCCESS;
  }
  if (!(victim = get_char(person)))
  {
    ch->sendln("They aren't here.");
    return ReturnValue::eFAILURE;
  }

  ch->send(u"Player: %s\r\n"_s.arg(qPrintable(victim->name())));

  if (isSet(victim->combat, COMBAT_SHOCKED))
    ch->sendln("COMBAT_SHOCKED");

  if (isSet(victim->combat, COMBAT_BASH1))
    ch->sendln("COMBAT_BASH1");

  if (isSet(victim->combat, COMBAT_BASH2))
    ch->sendln("COMBAT_BASH2");

  if (isSet(victim->combat, COMBAT_STUNNED))
    ch->sendln("COMBAT_STUNNED");

  if (isSet(victim->combat, COMBAT_STUNNED2))
    ch->sendln("COMBAT_STUNNED2");

  if (isSet(victim->combat, COMBAT_CIRCLE))
    ch->sendln("COMBAT_CIRCLE");

  if (isSet(victim->combat, COMBAT_BERSERK))
    ch->sendln("COMBAT_BERSERK");

  if (isSet(victim->combat, COMBAT_HITALL))
    ch->sendln("COMBAT_HITALL");

  if (isSet(victim->combat, COMBAT_RAGE1))
    ch->sendln("COMBAT_RAGE1");

  if (isSet(victim->combat, COMBAT_RAGE1))
    ch->sendln("COMBAT_RAGE2");

  if (isSet(victim->combat, COMBAT_BLADESHIELD1))
    ch->sendln("COMBAT_BLADESHIELD1");

  if (isSet(victim->combat, COMBAT_BLADESHIELD2))
    ch->sendln("COMBAT_BLADESHIELD2");

  if (isSet(victim->combat, COMBAT_REPELANCE))
    ch->sendln("COMBAT_REPELANCE");

  if (isSet(victim->combat, COMBAT_VITAL_STRIKE))
    ch->sendln("COMBAT_VITAL_STRIKE");

  if (isSet(victim->combat, COMBAT_MONK_STANCE))
    ch->sendln("COMBAT_MONK_STANCE");

  if (isSet(victim->combat, COMBAT_MISS_AN_ATTACK))
    ch->sendln("COMBAT_MISS_AN_ATTACK");

  if (isSet(victim->combat, COMBAT_ORC_BLOODLUST1))
    ch->sendln("COMBAT_ORC_BLOODLUST1");

  if (isSet(victim->combat, COMBAT_ORC_BLOODLUST2))
    ch->sendln("COMBAT_ORC_BLOODLUST2");

  if (isSet(victim->combat, COMBAT_THI_EYEGOUGE))
    ch->sendln("COMBAT_THI_EYEGOUGE");

  if (isSet(victim->combat, COMBAT_THI_EYEGOUGE2))
    ch->sendln("COMBAT_THI_EYEGOUGE2");

  if (isSet(victim->combat, COMBAT_FLEEING))
    ch->sendln("COMBAT_FLEEING");

  if (isSet(victim->combat, COMBAT_SHOCKED2))
    ch->sendln("COMBAT_SHOCKED2");

  if (isSet(victim->combat, COMBAT_CRUSH_BLOW))
    ch->sendln("COMBAT_CRUSH_BLOW");

  if (isSet(victim->combat, COMBAT_CRUSH_BLOW2))
    ch->sendln("COMBAT_CRUSH_BLOW2");

  if (isSet(victim->combat, COMBAT_ATTACKER))
    ch->sendln("COMBAT_ATTACKER");

  ch->sendln("--------------------\r\n");

  return ReturnValue::eSUCCESS;
}

command_return_t do_debug(CharacterPtr ch, QString args, cmd_t cmd)
{
  QString arg1, arg2, arg3;
  QString remainder;

  std::tie(arg1, remainder) = half_chop(args);
  if (arg1 == "perf")
  {
    std::tie(arg2, remainder) = half_chop(remainder);
    if (arg2 == "list")
    {
      for (const auto &name : PerfTimers.keys())
      {
        ch->sendln(u"%1"_s.arg(name));
      }
    }
    else if (arg2 == "show")
    {
      std::tie(arg3, remainder) = half_chop(remainder);
      if (arg3 == "all")
      {
        for (const auto &[key, t] : PerfTimers.asKeyValueRange())
        {
          ch->sendln(QStringLiteral("%15s: cur:%lus %luμs\tmin:%lus %luμs\tmax:%lus %luμs\tavg:%lus %luμs")
                         .arg(key)
                         .arg(t.getDiff().tv_sec)
                         .arg(t.getDiff().tv_usec)
                         .arg(t.getDiffMin().tv_sec)
                         .arg(t.getDiffMin().tv_usec)
                         .arg(t.getDiffMax().tv_sec)
                         .arg(t.getDiffMax().tv_usec)
                         .arg(t.getDiffAvg().tv_sec)
                         .arg(t.getDiffAvg().tv_usec));
        }
      }
      else if (arg3 != "")
      {
        QMap<QString, Timer>::iterator i = PerfTimers.find(arg3);
        if (i != PerfTimers.end())
        {
          QString key = conn->key();
          Timer t = conn->value();
          ch->sendln(QStringLiteral(
                         "%15s: cur:%lus %luμs\tmin:%lus %luμs\tmax:%lus %luμs\tavg:%lus %luμs")
                         .arg(key)
                         .arg(t.getDiff().tv_sec)
                         .arg(t.getDiff().tv_usec)
                         .arg(t.getDiffMin().tv_sec)
                         .arg(t.getDiffMin().tv_usec)
                         .arg(t.getDiffMax().tv_sec)
                         .arg(t.getDiffMax().tv_usec)
                         .arg(t.getDiffAvg().tv_sec)
                         .arg(t.getDiffAvg().tv_usec));
        }
        else
        {
          ch->sendln("performance timer key not found");
        }
      }
      else
      {
        ch->sendln("Please specify a performance timer key. Run debug perf list");
      }
    }
  }
  else if (arg1 == "charmie")
  {
    std::tie(arg2, remainder) = half_chop(remainder);
    if (remainder == "previous")
    {
      ch->load_charmie_equipment(QString(arg2.c_str()), true);
    }
    else
    {
      ch->load_charmie_equipment(QString(arg2.c_str()));
    }
  }
  else if (arg1 == "player")
  {
    std::tie(arg2, remainder) = half_chop(remainder);
    auto victim = get_pc(arg2.c_str());
    if (!victim)
    {
      ch->sendln("Player not found.");
      return ReturnValue::eFAILURE;
    }
    victim->setDebug(!victim->getDebug());
    ch->sendln(u"Debug for %1 toggled %2"_s.arg(qPrintable(victim->name())).arg(victim->getDebug() ? "on" : "off"));
    return ReturnValue::eSUCCESS;
  }
  else if (arg1 == "mobile")
  {
    std::tie(arg2, remainder) = half_chop(remainder);
    auto match = QRegularExpression("^v{0,1}([0-9]+)$").match(arg2.c_str());

    if (match.hasMatch())
    {
      bool ok = false;
      vnum_t vnum = match.captured(1).toULongLong(&ok);
      if (ok)
      {

        // All NPCs instances of a specific VNUM will have debug toggled
        // according to the first matching NPC.
        quint64 change_count = {};
        bool first_npc_found = false;
        bool first_npc_debug_state = false;
        for (const auto &c : DC::getInstance()->character_list)
        {
          if (c->isNonPlayer() && c->mobdata && DC::getInstance()->mob_index[c->mobdata->nr].vnum() == vnum)
          {
            if (!first_npc_found)
            {
              first_npc_found = true;
              first_npc_debug_state = c->getDebug();
            }
            c->setDebug(!first_npc_debug_state);
            ch->sendln(u"Vnum %1 Rnum %2 debug turned %3."_s).arg(vnum).arg(c->mobdata->nr).arg(c->getDebug() ? "on" : "off"));
            change_count++;
          }
        }
        ch->sendln(u"%1 mobiles changed."_s).arg(change_count));
        return ReturnValue::eSUCCESS;
      }
    }

    ch->sendln("Invalid vnum. Valid example: 1 or v1");
    return ReturnValue::eFAILURE;
  }
  else
  {
    ch->sendln("debug perf list");
    ch->sendln("      perf show <key>");
    ch->sendln("      perf set <key> <value>");
    ch->sendln("      charmie <name> [previous]");
    ch->sendln("      player <name>");
    ch->sendln("      mobile <vnum>");
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_pardon(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString person;
  QString flag;
  CharacterPtr victim;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  half_chop(argument, person, flag);

  if (person.isEmpty())
  {
    ch->sendln("Pardon whom?");
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_pc_vis(ch, person)))
  {
    ch->sendln("They aren't here.");
    return ReturnValue::eFAILURE;
  }

  if (!str_cmp("thief", flag))
  {
    if (victim->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
    {
      ch->sendln("Thief flag removed.");
      affect_from_char(victim, Character::PLAYER_OBJECT_THIEF);
      victim->sendln("A nice god has pardoned you of your thievery.");
    }
    else
    {
      ch->sendln("That character is not a thief!");
      return ReturnValue::eFAILURE;
    }
  }
  else if (!str_cmp("killer", flag))
  {
    if (ISSET(victim->affected_by, AFF_CANTQUIT))
    {
      ch->sendln("Killer flag removed.");
      affect_from_char(victim, Character::PLAYER_CANTQUIT);
      victim->sendln("A nice god has pardoned you of your murdering.");
    }
    else
    {
      ch->sendln("That player has no CANTQUIT flag!");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    ch->sendln("No flag specified! (Flags are 'thief' & 'killer')");
    return ReturnValue::eFAILURE;
  }

  ch->sendln("Done.");
  QString log_buf = {};
  dc_sprintf(log_buf, "%s pardons %s for %s.",
             qPrintable(ch->name()), qPrintable(victim->name()), flag);
  DC::getInstance()->logentry(log_buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
  return ReturnValue::eSUCCESS;
}

command_return_t do_dmg_eq(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  ObjectPtr obj_object;
  qint32 eqdam;

  one_argument(argument, buf);

  if (buf.isEmpty())
  {
    ch->sendln("Syntax: damage <item>");
    return ReturnValue::eFAILURE;
  }

  obj_object = get_obj_in_list_vis(ch, buf, ch->carrying);

  if (!obj_object)
  {
    ch->sendln("You don't seem to have that item.");
    return ReturnValue::eFAILURE;
  }
  eqdam = damage_eq_once(obj_object);

  if (eqdam >= eq_max_damage(obj_object))
    eq_destroyed(ch, obj_object, -1);
  else
  {
    act_to_character("$p is damaged.", ch, obj_object, 0, 0);
    act_to_room("$p carried by $n is damaged.", ch, obj_object, 0, 0);
  }

  return ReturnValue::eSUCCESS;
}

QString print_classes(qint32 bitv)
{
  qint32 i = {};
  QString buf;
  for (; *pc_clss_types2[i] != '\n'; i++)
    if (isSet(bitv, 1 << (i - 1)))
      buf = u"%1 %2"_s.arg(buf).arg(pc_clss_types2[i]);
  return buf;
}

// do_string is in modify.C

skill_quest *find_sq(QString testa)
{
  for (auto curr = skill_list; curr; curr = curr->next)
    if (str_nosp_equal(get_skill_name(curr->num), testa))
      return curr;
  return {};
}

skill_quest *find_sq(qint32 sq)
{
  for (auto curr = skill_list; curr; curr = curr->next)
    if (sq == curr->num)
      return curr;
  return {};
}

command_return_t do_sqedit(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString command;
  argument = one_argument(argument, command);
  qint32 clas = 1;

  const QStringList fields = {
      "new",
      "delete",
      "message",
      "level",
      "class",
      "show",
      "list",
      "save",
      "\n"};
  if (!ch->has_skill(COMMAND_SQEDIT))
  {
    ch->sendln("You are unable to do so.");
    return ReturnValue::eFAILURE;
  }
  //  if (argument && *argument && !is_number(argument))
  //   argument = one_argument(argument, skill+strlen(skill));
  /*  if (!argument || argument.isEmpty())
    {
       send_to_char("$3Syntax:$R sqedit <level/class> <skill> <value> OR\r\n"
                    "$3Syntax:$R sqedit message/new/<skillname>\r\n",ch)={};
       ch->sendln("$3Syntax:$R sqedit list.");
       return ReturnValue::eFAILURE;
    }*/
  qint32 i;
  for (i = {};; i++)
  {
    if (!str_cmp(fields[i], command) ||
        !str_cmp(fields[i], "\n"))
      break;
  }

  if (!str_cmp(fields[i], "\n"))
  {

    send_to_char("$3Syntax:$R sqedit <message/level/class> <skill> <value> OR\r\n"
                 "$3Syntax:$R sqedit <show/new/delete> <skillname> OR\r\n",
                 ch);
    ch->sendln("$3Syntax:$R sqedit list <class>.");
    ch->sendln("$3Syntax:$R sqedit save.");
    return ReturnValue::eFAILURE;
  }
  QString arg1, arg2, arg3;
  bool done = false;
  argument = one_argument(argument, arg1);
  skill_quest *skill = {};

  if (argument && *argument)
  {
    argument = one_argument(argument, arg2);
    dc_strcpy(arg3, arg1);
    dc_strcat(arg3, " ");
    dc_strcat(arg3, arg2);
    skill = find_sq(arg3);
  }

  if (skill == nullptr && (skill = find_sq(arg1)) == nullptr && i != 0 && i != 6 && i != 7)
  {
    ch->sendln("Unknown skill.");
    return ReturnValue::eFAILURE;
  }
  skill_quest *curren, *last = {};
  switch (i)
  {
  case 0:
    ///	qint32 i;
    if (arg3[0] != '\0')
      i = find_skill_num(arg3);
    if (i <= 0)
      i = find_skill_num(arg1);
    if (i <= 0 || arg2[0] == '\0')
    {
      ch->sendln("Skill not found.");
      return ReturnValue::eFAILURE;
    }
    if (find_sq(i))
    {
      ch->sendln("Skill quest already exists. Stop duping the damn sqs ;)");
      return ReturnValue::eFAILURE;
    }
    //	argument = one_argument(argument,arg3);
    if (arg3[0] != '\0')
      for (qint32 x = {}; *pc_clss_types2[x] != '\n'; x++)
      {
        if (!str_cmp(pc_clss_types2[x], arg2))
          clas = x;
      }

    auto newOne = new skill_quest;
    newOne->num = i;
    newOne->level = 1;
    newOne->message = u"New skillquest."_s;
    newOne->clas = (1 << (clas - 1));
    newOne->next = skill_list;
    skill_list = newOne;
    ch->sendln("Skill quest added.");
    break;
  case 1:
    for (curren = skill_list; curren; curren = curren->next)
    {
      if (curren == skill)
      {
        if (last)
          last->next = curren->next;
        else
          skill_list = curren->next;
        ch->sendln("Deleted.");
        curren->message = {};
        curren = {};
        return ReturnValue::eSUCCESS;
      }
      last = curren;
    }
    ch->sendln("Error in sqedit. Tell Urizen.");
    break;
    break;
  case 2:
    ch->sendln("Enter new message. End with \\s.");
    ch->desc->connected = Connection::states::EDITING;
    ch->desc->strnew = &(skill->message);
    ch->desc->max_str = MAX_MESSAGE_LENGTH;
    break;
  case 3:
    if (is_number(arg2))
    {
      skill->level = atoi(arg2);
      ch->sendln("Level modified.");
    }
    else
    {
      ch->sendln("Invalid level.");
      return ReturnValue::eFAILURE;
    }
    break;
  case 4:
    qint32 i;
    if (!is_number(arg2))
    {
      for (i = {}; *pc_clss_types2[i] != '\n'; i++)
      {
        if (!str_cmp(pc_clss_types2[i], arg2))
          break;
      }
      /*	if (*pc_clss_types[i] == '\n')
        {
          ch->sendln("Invalid class.");
          return ReturnValue::eFAILURE;
        }*/
    }
    else
    {
      i = atoi(arg2);
    }
    if (i < 1 || i > 11)
    {
      ch->sendln("Invalid class.");
      return ReturnValue::eFAILURE;
    }
    if (isSet(skill->clas, 1 << (i - 1)))
      REMOVE_BIT(skill->clas, 1 << (i - 1));
    // skill->clas = i;
    else
      SET_BIT(skill->clas, 1 << (i - 1));
    ch->sendln("Class modified.");
    break;
  case 5: // show
    ch->send(u"$3Skill$R: %s\r\n$3Message$R: %s\r\n$3Class$R: %s\r\n$3Level$R: %d\r\n"_s.arg(qPrintable(get_skill_name(skill->num))).arg(skill->message).arg(print_classes(skill->clas)).arg(skill->level));
    break;
  case 6:
    qint32 l;
    for (l = {}; *pc_clss_types2[l] != '\n'; l++)
    {
      if (!str_cmp(pc_clss_types2[l], arg1))
        break;
    }
    if (*pc_clss_types2[l] == '\n')
    {
      ch->send("Unknown class.");
      return ReturnValue::eFAILURE;
    }
    ch->sendln("These are the current sqs:");
    ch->send(u"$3%s skillquests.$R\r\n"_s.arg(pc_clss_types2[l]));
    for (curren = skill_list; curren; curren = curren->next)
    {
      if (!isSet(curren->clas, 1 << (l - 1)))
        continue;
      ch->send(u"$3%d$R. %s\r\n"_s.arg(curren->num).arg(qPrintable(get_skill_name(curren->num))));
      done = true;
    }
    if (!done)
      ch->sendln("    No skill quests.");
    break;
  case 7: // save
    do_write_skillquest(ch, argument, cmd);
    break;
  default:
    DC::getInstance()->logentry(u"Incorrect -i- in do_sqedit"_s, 0, DC::LogChannel::LOG_WORLD);
    return ReturnValue::eFAILURE;
  }
  return ReturnValue::eSUCCESS;
}
qint32 max_aff(ObjectPtr obj, qint32 type)
{
  qint32 a, b = -1;
  for (a = {}; a < obj->num_affects; a++)
  {
    if (obj->affected[a].location > 30)
      continue;
    if (type & (1 << obj->affected[a].location))
      b = b > obj->affected[a].modifier ? b : obj->affected[a].modifier;
  }
  return b;
}

qint32 maxcheck(qint32 &check, qint32 max)
{
  if (check < max)
  {
    check = max;
    return 1;
  }
  else if (check == max)
    return 2;
  return 0;
}

qint32 wear_bitv[MAX_WEAR] = {
    65535, 2, 2, 4, 4, 8, 16, 32, 64, 128, 256, 512,
    1024, 2048, 4096, 4096, 8192, 8192, 16384, 16384, 131072,
    262144, 262144};

command_return_t do_eqmax(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr vict;
  QString arg;
  qint32 a = 0, o;
  argument = one_argument(argument, arg);
  extern qint32 class_restricted(CharacterPtr ch, ObjectPtr obj);
  extern qint32 size_restricted(CharacterPtr ch, ObjectPtr obj);

  if ((vict = get_pc_vis(ch, arg)) == nullptr)
  {
    ch->sendln("Who?");
    return ReturnValue::eFAILURE;
  }
  argument = one_argument(argument, arg);
  qint32 type;
  qint32 last_max[MAX_WEAR] =
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  qint32 last_vnum[5][MAX_WEAR] = {
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
  /*    bool last_unique[5][MAX_WEAR] = {
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
  */

  if (!str_cmp(arg, "damage"))
    type = (1 << APPLY_DAMROLL) | (1 << APPLY_HIT_N_DAM);
  else if (!str_cmp(arg, "hp"))
    type = 1 << APPLY_HIT;
  else if (!str_cmp(arg, "mana"))
    type = 1 << APPLY_MANA;
  else
  {
    ch->sendln("$3Syntax$R: eqmax <character> <damage/hp/mana> <optional: nodouble>");
    return ReturnValue::eFAILURE;
  }
  argument = one_argument(argument, arg);
  bool nodouble = false;
  if (!str_cmp(arg, "nodouble"))
    nodouble = true;
  qint32 i = 1;
  ObjectPtr obj;
  for (i = 1; i < 32000; i++)
  {
    if (real_object(i) < 0)
      continue;
    obj = DC::getInstance()->obj_index[real_object(i)].item;
    if (!class_restricted(vict, obj) &&
        !size_restricted(vict, obj) &&
        CAN_WEAR(obj, TAKE) &&
        !isSet(obj->obj_flags.extra_flags, ITEM_NOSAVE) &&
        obj->obj_flags.eq_level <= vict->getLevel() &&
        !isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      for (o = {}; o < MAX_WEAR; o++)
        if (CAN_WEAR(obj, wear_bitv[o]))
        {
          qint32 dam = max_aff(obj, type);
          if ((a = maxcheck(last_max[o], dam)))
          {
            if (a == 1)
            {
              last_vnum[0][o] = DC::getInstance()->obj_index[obj->item_number].vnum();
              last_vnum[1][o] = -1;
              last_vnum[2][o] = -1;
              last_vnum[3][o] = -1;
              last_vnum[4][o] = -1;
              if (nodouble)
                break;
            }
            else
            {
              qint32 v;
              for (v = {}; v < 5; v++)
                if (last_vnum[v][o] == -1)
                {
                  last_vnum[v][o] = DC::getInstance()->obj_index[obj->item_number].vnum();
                  break;
                }
            }
          }
        }
    }
  }
  QString buf1;
  qint32 tot = {};
  for (i = 1; i < MAX_WEAR; i++)
  {
    buf1[0] = '\0';
    dc_sprintf(buf1, "%d. ", i);
    if (last_vnum[a][0] != -1)
      tot += last_max[a];
    if (last_vnum[a][i] == -1)
      dc_sprintf(buf1, "%s Nothing\r\n", buf1);
    else
      for (a = {}; a < 5; a++)
      {
        if (last_vnum[a][i] == -1)
          continue;
        dc_sprintf(buf1, "%s %s(%d)   ", buf1, qPrintable((DC::getInstance()->obj_index[real_object(last_vnum[a][i])].item)->short_description()), last_vnum[a][i]);
        //    else dc_sprintf(buf1,"%s%d. %d\r\n",buf1,i,last_vnum[i]);
      }
    dc_sprintf(buf1, "%s\n", buf1);
    send_to_char(buf1, ch);
  }
  dc_sprintf(buf1, "Total %s: %d\r\n", arg, tot);
  send_to_char(buf1, ch);
  return ReturnValue::eSUCCESS;
}

command_return_t do_reload(CharacterPtr ch, QString argument, cmd_t cmd)
{
  extern QString motd;
  extern QString imotd;
  extern QString new_help;
  extern QString new_ihelp;
  extern QString credits;
  extern QString story;
  extern QString webpage;
  extern QString info;
  extern QString greetings1;
  extern QString greetings2;
  extern QString greetings3;
  extern QString greetings4;

  QString arg;

  one_argument(argument, arg);

  if (!str_cmp(arg, "all"))
  {
    file_to_string(MOTD_FILE, motd);
    file_to_string(IMOTD_FILE, imotd);
    file_to_string(NEW_HELP_PAGE_FILE, new_help);
    file_to_string(NEW_IHELP_PAGE_FILE, new_ihelp);
    file_to_string(CREDITS_FILE, credits);
    file_to_string(STORY_FILE, story);
    file_to_string(WEBPAGE_FILE, webpage);
    file_to_string(INFO_FILE, info);
    file_to_string(GREETINGS1_FILE, greetings1);
    file_to_string(GREETINGS2_FILE, greetings2);
    file_to_string(GREETINGS3_FILE, greetings3);
    file_to_string(GREETINGS4_FILE, greetings4);
    ch->sendln("Done!");
  }
  else if (!str_cmp(arg, "credits"))
    file_to_string(CREDITS_FILE, credits);
  else if (!str_cmp(arg, "story"))
    file_to_string(STORY_FILE, story);
  else if (!str_cmp(arg, "webpage"))
    file_to_string(WEBPAGE_FILE, webpage);
  else if (!str_cmp(arg, "INFO"))
    file_to_string(INFO_FILE, info);
  else if (!str_cmp(arg, "motd"))
    file_to_string(MOTD_FILE, motd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string(IMOTD_FILE, imotd);
  else if (!str_cmp(arg, "help"))
    file_to_string(NEW_HELP_PAGE_FILE, new_help);
  else if (!str_cmp(arg, "ihelp"))
    file_to_string(NEW_IHELP_PAGE_FILE, new_ihelp);
  else if (!str_cmp(arg, "xhelp"))
  {
    do_reload_help(ch, u""_s);
    ch->sendln("Done!");
  }
  else if (!str_cmp(arg, "greetings"))
  {
    file_to_string(GREETINGS1_FILE, greetings1);
    file_to_string(GREETINGS2_FILE, greetings2);
    file_to_string(GREETINGS3_FILE, greetings3);
    file_to_string(GREETINGS4_FILE, greetings4);
    ch->sendln("Done!");
  }
  else if (!str_cmp(arg, "vaults"))
  {
    DC::getInstance()->reload_vaults();
    ch->sendln("Done!");
  }
  else
  {
    ch->sendln("Unknown reload option. Try 'help reload'.");
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_listproc(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg, arg1, arg2;
  qint32 start, i, end, tot;
  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  bool mob;
  if (arg[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' ||
      !check_range_valid_and_convert(start, arg1, 0, 100000) ||
      !check_range_valid_and_convert(end, arg2, 0, 100000) ||
      start > end)
  {
    ch->sendln("$3Syntax:$n listproc <obj/mob> <low vnum> <high vnum>");
    return ReturnValue::eFAILURE;
  }
  mob = !str_cmp(arg, "mob"); // typoed mob means obj. who cares.
  QString buf;
  buf[0] = '\0';
  for (i = start, tot = 1; i <= end; i++)
  {
    if (mob && (real_mobile(i) < 0 || !DC::getInstance()->mob_index[real_mobile(i)].mobprogs))
      continue;
    else if (!mob && (real_object(i) < 0 || !DC::getInstance()->obj_index[real_object(i)].mobprogs))
      continue;
    if (tot++ > 100)
      break;
    if (mob)
    {
      ch->sendln(u"[%1] [%2] %3"_s.arg(tot, -3).arg(i, -3).arg(((CharacterPtr)DC::getInstance()->mob_index[real_mobile(i)].item)->name()));
    }
    else
    {
      ch->sendln(u"[%1] [%2] %3"_s.arg(tot, -3).arg(i, -3).arg((DC::getInstance()->obj_index[real_object(i)].item)->name()));
    }
  }
  return ReturnValue::eSUCCESS;
}
