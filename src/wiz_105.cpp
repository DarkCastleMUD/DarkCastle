/********************************
| Level 105 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/wizard.h"
#include "DC/spells.h" // Character::PLAYER_CANTQUIT
#include "DC/mobile.h"
#include "DC/handler.h"
#include "DC/room.h"
#include "DC/player.h"
#include "DC/fight.h"
#include "DC/utility.h"

#include "DC/interp.h"
#include "DC/returnvals.h"
#include "DC/innate.h"
#include "DC/fileinfo.h"
#include "DC/const.h"
#include "DC/Timer.h"
#include "DC/common.h"

int do_clearaff(Character *ch, char *argument, cmd_t cmd)
{
  bool found = false;
  char buf[MAX_INPUT_LENGTH];
  Character *victim;
  struct affected_type *af, *afpk;
  class Object *dummy;

  one_argument(argument, buf);

  if (!*buf)
    victim = ch;
  else if (!generic_find(argument, FIND_CHAR_ROOM | FIND_CHAR_WORLD, ch, &victim, &dummy, true))
    ch->send(QStringLiteral("Couldn't find '%1' anywhere.\r\n").arg(argument));
  if (victim)
  {
    for (af = victim->affected; af; af = afpk)
    {
      found = true;
      afpk = af->next;
      QString aff_name = get_skill_name(af->type);
      if (!aff_name.isEmpty())
      {
        ch->send(QStringLiteral("Removing %1 affect.\r\n").arg(aff_name));
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
    return eSUCCESS;
  }
  return eFAILURE;
}

int do_reloadhelp(Character *ch, char *argument, cmd_t cmd)
{
  extern FILE *help_fl;
  extern struct help_index_element *help_index;
  extern struct help_index_element *build_help_index(FILE * fl, int *num);
  DC::getInstance()->free_help_from_memory();
  fclose(help_fl);
  if (!(help_fl = fopen(HELP_KWRD_FILE, "r")))
  {
    perror(HELP_KWRD_FILE);
    abort();
  }
  help_index = build_help_index(help_fl, &DC::getInstance()->top_of_helpt);
  ch->sendln("Reloaded.");
  return eSUCCESS;
}

int do_log(Character *ch, char *argument, cmd_t cmd)
{
  Character *vict;
  class Object *dummy;
  char buf[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];

  if (IS_NPC(ch) || !ch->has_skill(COMMAND_LOG))
  {
    ch->sendln("Huh?");
    return eFAILURE;
  }

  one_argument(argument, buf);

  if (!*buf)
  {
    ch->sendln("Log who?");
  }
  else if (!(vict = get_pc_vis(ch, buf)))
    ch->sendln("Couldn't find any such creature.");
  else if (IS_NPC(vict))
    ch->sendln("Can't do that to a beast.");
  else if (vict->getLevel() > ch->getLevel())
    act("$E might object to that.. better not.", ch, 0, vict, TO_CHAR, 0);
  else if (isSet(vict->player->punish, PUNISH_LOG))
  {
    ch->sendln("LOG removed.");
    REMOVE_BIT(vict->player->punish, PUNISH_LOG);
    sprintf(buf2, "%s removed log on %s.", GET_NAME(ch), GET_NAME(vict));
    logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  else
  {
    ch->sendln("LOG set.");
    SET_BIT(vict->player->punish, PUNISH_LOG);
    sprintf(buf2, "%s just logged %s.", GET_NAME(ch), GET_NAME(vict));
    logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  return eSUCCESS;
}

int do_showbits(Character *ch, char *argument, cmd_t cmd)
{
  char person[MAX_INPUT_LENGTH];
  Character *victim;
  one_argument(argument, person);

  if (!*person)
  {
    char buf[MAX_STRING_LENGTH];
    const auto &character_list = DC::getInstance()->character_list;
    for (const auto &victim : character_list)
    {
      if (IS_NPC(victim))
        continue;
      sprintf(buf, "0.%s", victim->getNameC());
      do_showbits(ch, buf, cmd);
    }
    return eSUCCESS;
  }
  if (!(victim = get_char(person)))
  {
    ch->sendln("They aren't here.");
    return eFAILURE;
  }

  csendf(ch, "Player: %s\n\r", victim->getNameC());

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

  ch->sendln("--------------------\n\r");

  return eSUCCESS;
}

int do_debug(Character *ch, char *args, cmd_t cmd)
{
  std::string arg1, arg2, arg3;
  std::string remainder;

  std::tie(arg1, remainder) = half_chop(args);
  if (arg1 == "perf")
  {
    std::tie(arg2, remainder) = half_chop(remainder);
    if (arg2 == "list")
    {
      for (const auto &pt : PerfTimers)
      {
        csendf(ch, "%s\n\r", pt.first.c_str());
      }
    }
    else if (arg2 == "show")
    {
      std::tie(arg3, remainder) = half_chop(remainder);
      if (arg3 == "all")
      {
        for (const auto &pt : PerfTimers)
        {
          std::string key = pt.first;
          Timer t = pt.second;
          csendf(ch, "%15s: "
                     "cur:%lus %luμs"
                     "\tmin:%lus %luμs"
                     "\tmax:%lus %luμs"
                     "\tavg:%lus %luμs\n\r",
                 key.c_str(),
                 t.getDiff().tv_sec, t.getDiff().tv_usec,
                 t.getDiffMin().tv_sec, t.getDiffMin().tv_usec,
                 t.getDiffMax().tv_sec, t.getDiffMax().tv_usec,
                 t.getDiffAvg().tv_sec, t.getDiffAvg().tv_usec);
        }
      }
      else if (arg3 != "")
      {
        std::map<std::string, Timer>::iterator i = PerfTimers.find(arg3);
        if (i != PerfTimers.end())
        {
          std::string key = i->first;
          Timer t = i->second;
          csendf(ch, "%15s: "
                     "cur:%lus %luμs"
                     "\tmin:%lus %luμs"
                     "\tmax:%lus %luμs"
                     "\tavg:%lus %luμs\n\r",
                 key.c_str(),
                 t.getDiff().tv_sec, t.getDiff().tv_usec,
                 t.getDiffMin().tv_sec, t.getDiffMin().tv_usec,
                 t.getDiffMax().tv_sec, t.getDiffMax().tv_usec,
                 t.getDiffAvg().tv_sec, t.getDiffAvg().tv_usec);
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
      return eFAILURE;
    }
    victim->setDebug(!victim->getDebug());
    ch->sendln(QStringLiteral("Debug for %1 toggled %2").arg(GET_NAME(victim)).arg(victim->getDebug() ? "on" : "off"));
    return eSUCCESS;
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
        uint64_t change_count{};
        bool first_npc_found = false;
        bool first_npc_debug_state = false;
        for (const auto &c : DC::getInstance()->character_list)
        {
          if (IS_NPC(c) && c->mobdata && DC::getInstance()->mob_index[c->mobdata->nr].virt == vnum)
          {
            if (!first_npc_found)
            {
              first_npc_found = true;
              first_npc_debug_state = c->getDebug();
            }
            c->setDebug(!first_npc_debug_state);
            ch->sendln(QStringLiteral("Vnum %1 Rnum %2 debug turned %3.").arg(vnum).arg(c->mobdata->nr).arg(c->getDebug() ? "on" : "off"));
            change_count++;
          }
        }
        ch->sendln(QStringLiteral("%1 mobiles changed.").arg(change_count));
        return eSUCCESS;
      }
    }

    ch->sendln("Invalid vnum. Valid example: 1 or v1");
    return eFAILURE;
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

  return eSUCCESS;
}

int do_pardon(Character *ch, char *argument, cmd_t cmd)
{
  char person[MAX_INPUT_LENGTH];
  char flag[MAX_INPUT_LENGTH];
  Character *victim;

  if (IS_NPC(ch))
    return eFAILURE;

  half_chop(argument, person, flag);

  if (!*person)
  {
    ch->sendln("Pardon whom?");
    return eFAILURE;
  }

  if (!(victim = get_pc_vis(ch, person)))
  {
    ch->sendln("They aren't here.");
    return eFAILURE;
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
      return eFAILURE;
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
      return eFAILURE;
    }
  }
  else
  {
    ch->sendln("No flag specified! (Flags are 'thief' & 'killer')");
    return eFAILURE;
  }

  ch->sendln("Done.");
  char log_buf[MAX_STRING_LENGTH] = {};
  sprintf(log_buf, "%s pardons %s for %s.",
          GET_NAME(ch), victim->getNameC(), flag);
  logentry(log_buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
  return eSUCCESS;
}

int do_dmg_eq(Character *ch, char *argument, cmd_t cmd)
{
  char buf[MAX_STRING_LENGTH];
  class Object *obj_object;
  int eqdam;

  one_argument(argument, buf);

  if (!*buf)
  {
    ch->sendln("Syntax: damage <item>");
    return eFAILURE;
  }

  obj_object = get_obj_in_list_vis(ch, buf, ch->carrying);

  if (!obj_object)
  {
    ch->sendln("You don't seem to have that item.");
    return eFAILURE;
  }
  eqdam = damage_eq_once(obj_object);

  if (eqdam >= eq_max_damage(obj_object))
    eq_destroyed(ch, obj_object, -1);
  else
  {
    act("$p is damaged.", ch, obj_object, 0, TO_CHAR, 0);
    act("$p carried by $n is damaged.", ch, obj_object, 0, TO_ROOM, 0);
  }

  return eSUCCESS;
}

char *print_classes(int bitv)
{
  static char buf[512];
  int i = 0;
  buf[0] = '\0';

  for (; *pc_clss_types2[i] != '\n'; i++)
  {
    if (isSet(bitv, 1 << (i - 1)))
      sprintf(buf, "%s %s", buf, pc_clss_types2[i]);
  }

  return buf;
}

// do_string is in modify.C

struct skill_quest *find_sq(char *testa)
{
  struct skill_quest *curr;
  for (curr = skill_list; curr; curr = curr->next)
    if (!str_nosp_cmp(get_skill_name(curr->num), testa))
      return curr;
  return nullptr;
}

struct skill_quest *find_sq(int sq)
{
  struct skill_quest *curr;
  for (curr = skill_list; curr; curr = curr->next)
    if (sq == curr->num)
      return curr;
  return nullptr;
}

int do_sqedit(Character *ch, char *argument, cmd_t cmd)
{
  char command[MAX_INPUT_LENGTH];
  argument = one_argument(argument, command);
  int clas = 1;

  const char *fields[] = {
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
    return eFAILURE;
  }
  //  if (argument && *argument && !is_number(argument))
  //   argument = one_argument(argument, skill+strlen(skill));
  /*  if (!argument || !*argument)
    {
       send_to_char("$3Syntax:$R sqedit <level/class> <skill> <value> OR\r\n"
                    "$3Syntax:$R sqedit message/new/delete <skillname>\r\n",ch);
       ch->sendln("$3Syntax:$R sqedit list.");
       return eFAILURE;
    }*/
  int i;
  for (i = 0;; i++)
  {
    if (!str_cmp((char *)fields[i], command) ||
        !str_cmp((char *)fields[i], "\n"))
      break;
  }

  if (!str_cmp((char *)fields[i], "\n"))
  {

    send_to_char("$3Syntax:$R sqedit <message/level/class> <skill> <value> OR\r\n"
                 "$3Syntax:$R sqedit <show/new/delete> <skillname> OR\r\n",
                 ch);
    ch->sendln("$3Syntax:$R sqedit list <class>.");
    ch->sendln("$3Syntax:$R sqedit save.");
    return eFAILURE;
  }
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH * 2];
  bool done = false;
  argument = one_argument(argument, arg1);
  struct skill_quest *skill = nullptr;

  if (argument && *argument)
  {
    argument = one_argument(argument, arg2);
    strcpy(arg3, arg1);
    strcat(arg3, " ");
    strcat(arg3, arg2);
    skill = find_sq(arg3);
  }

  if (skill == nullptr && (skill = find_sq(arg1)) == nullptr && i != 0 && i != 6 && i != 7)
  {
    ch->sendln("Unknown skill.");
    return eFAILURE;
  }
  struct skill_quest *curren, *last = nullptr;
  switch (i)
  {
  case 0:
    struct skill_quest *newOne;
    ///	int i;
    if (arg3[0] != '\0')
      i = find_skill_num(arg3);
    if (i <= 0)
      i = find_skill_num(arg1);
    if (i <= 0 || arg2[0] == '\0')
    {
      ch->sendln("Skill not found.");
      return eFAILURE;
    }
    if (find_sq(i))
    {
      ch->sendln("Skill quest already exists. Stop duping the damn sqs ;)");
      return eFAILURE;
    }
    //	argument = one_argument(argument,arg3);
    if (arg3[0] != '\0')
      for (int x = 0; *pc_clss_types2[x] != '\n'; x++)
      {
        if (!str_cmp(pc_clss_types2[x], arg2))
          clas = x;
      }

#ifdef LEAK_CHECK
    newOne = (struct skill_quest *)calloc(1, sizeof(struct skill_quest));
#else
    newOne = (struct skill_quest *)dc_alloc(1, sizeof(struct skill_quest));
#endif
    newOne->num = i;
    newOne->level = 1;
    newOne->message = str_dup("New skillquest.");
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
        dc_free(curren->message);
        dc_free(curren);
        return eSUCCESS;
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
      return eFAILURE;
    }
    break;
  case 4:
    int i;
    if (!is_number(arg2))
    {
      for (i = 0; *pc_clss_types2[i] != '\n'; i++)
      {
        if (!str_cmp(pc_clss_types2[i], arg2))
          break;
      }
      /*	if (*pc_clss_types[i] == '\n')
        {
          ch->sendln("Invalid class.");
          return eFAILURE;
        }*/
    }
    else
    {
      i = atoi(arg2);
    }
    if (i < 1 || i > 11)
    {
      ch->sendln("Invalid class.");
      return eFAILURE;
    }
    if (isSet(skill->clas, 1 << (i - 1)))
      REMOVE_BIT(skill->clas, 1 << (i - 1));
    // skill->clas = i;
    else
      SET_BIT(skill->clas, 1 << (i - 1));
    ch->sendln("Class modified.");
    break;
  case 5: // show
    csendf(ch, "$3Skill$R: %s\r\n$3Message$R: %s\r\n$3Class$R: %s\r\n$3Level$R: %d\r\n", get_skill_name(skill->num).toStdString().c_str(), skill->message, print_classes(skill->clas), skill->level);
    break;
  case 6:
    int l;
    for (l = 0; *pc_clss_types2[l] != '\n'; l++)
    {
      if (!str_cmp(pc_clss_types2[l], arg1))
        break;
    }
    if (*pc_clss_types2[l] == '\n')
    {
      ch->send("Unknown class.");
      return eFAILURE;
    }
    ch->sendln("These are the current sqs:");
    csendf(ch, "$3%s skillquests.$R\r\n", pc_clss_types2[l]);
    for (curren = skill_list; curren; curren = curren->next)
    {
      if (!isSet(curren->clas, 1 << (l - 1)))
        continue;
      csendf(ch, "$3%d$R. %s\r\n", curren->num, get_skill_name(curren->num).toStdString().c_str());
      done = true;
    }
    if (!done)
      ch->sendln("    No skill quests.");
    break;
  case 7: // save
    do_write_skillquest(ch, argument, cmd);
    break;
  default:
    logentry(QStringLiteral("Incorrect -i- in do_sqedit"), 0, DC::LogChannel::LOG_WORLD);
    return eFAILURE;
  }
  return eSUCCESS;
}
int max_aff(class Object *obj, int type)
{
  int a, b = -1;
  for (a = 0; a < obj->num_affects; a++)
  {
    if (obj->affected[a].location > 30)
      continue;
    if (type & (1 << obj->affected[a].location))
      b = b > obj->affected[a].modifier ? b : obj->affected[a].modifier;
  }
  return b;
}

int maxcheck(int &check, int max)
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

int wear_bitv[MAX_WEAR] = {
    65535, 2, 2, 4, 4, 8, 16, 32, 64, 128, 256, 512,
    1024, 2048, 4096, 4096, 8192, 8192, 16384, 16384, 131072,
    262144, 262144};

int do_eqmax(Character *ch, char *argument, cmd_t cmd)
{
  Character *vict;
  char arg[MAX_INPUT_LENGTH];
  int a = 0, o;
  argument = one_argument(argument, arg);
  extern int class_restricted(Character * ch, class Object * obj);
  extern int size_restricted(Character * ch, class Object * obj);

  if ((vict = get_pc_vis(ch, arg)) == nullptr)
  {
    ch->sendln("Who?");
    return eFAILURE;
  }
  argument = one_argument(argument, arg);
  int type;
  int last_max[MAX_WEAR] =
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int last_vnum[5][MAX_WEAR] = {
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
    return eFAILURE;
  }
  argument = one_argument(argument, arg);
  bool nodouble = false;
  if (!str_cmp(arg, "nodouble"))
    nodouble = true;
  int i = 1;
  class Object *obj;
  for (i = 1; i < 32000; i++)
  {
    if (real_object(i) < 0)
      continue;
    obj = (Object *)DC::getInstance()->obj_index[real_object(i)].item;
    if (!class_restricted(vict, obj) &&
        !size_restricted(vict, obj) &&
        CAN_WEAR(obj, ITEM_TAKE) &&
        !isSet(obj->obj_flags.extra_flags, ITEM_NOSAVE) &&
        obj->obj_flags.eq_level <= vict->getLevel() &&
        !isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      for (o = 0; o < MAX_WEAR; o++)
        if (CAN_WEAR(obj, wear_bitv[o]))
        {
          int dam = max_aff(obj, type);
          if ((a = maxcheck(last_max[o], dam)))
          {
            if (a == 1)
            {
              last_vnum[0][o] = DC::getInstance()->obj_index[obj->item_number].virt;
              last_vnum[1][o] = -1;
              last_vnum[2][o] = -1;
              last_vnum[3][o] = -1;
              last_vnum[4][o] = -1;
              if (nodouble)
                break;
            }
            else
            {
              int v;
              for (v = 0; v < 5; v++)
                if (last_vnum[v][o] == -1)
                {
                  last_vnum[v][o] = DC::getInstance()->obj_index[obj->item_number].virt;
                  break;
                }
            }
          }
        }
    }
  }
  char buf1[MAX_STRING_LENGTH];
  int tot = 0;
  for (i = 1; i < MAX_WEAR; i++)
  {
    buf1[0] = '\0';
    sprintf(buf1, "%d. ", i);
    if (last_vnum[a][0] != -1)
      tot += last_max[a];
    if (last_vnum[a][i] == -1)
      sprintf(buf1, "%s Nothing\r\n", buf1);
    else
      for (a = 0; a < 5; a++)
      {
        if (last_vnum[a][i] == -1)
          continue;
        sprintf(buf1, "%s %s(%d)   ", buf1, ((Object *)DC::getInstance()->obj_index[real_object(last_vnum[a][i])].item)->short_description, last_vnum[a][i]);
        //    else sprintf(buf1,"%s%d. %d\r\n",buf1,i,last_vnum[i]);
      }
    sprintf(buf1, "%s\n", buf1);
    send_to_char(buf1, ch);
  }
  sprintf(buf1, "Total %s: %d\r\n", arg, tot);
  send_to_char(buf1, ch);
  return eSUCCESS;
}

int do_reload(Character *ch, char *argument, cmd_t cmd)
{
  extern char motd[MAX_STRING_LENGTH];
  extern char imotd[MAX_STRING_LENGTH];
  extern char new_help[MAX_STRING_LENGTH];
  extern char new_ihelp[MAX_STRING_LENGTH];
  extern char credits[MAX_STRING_LENGTH];
  extern char story[MAX_STRING_LENGTH];
  extern char webpage[MAX_STRING_LENGTH];
  extern char info[MAX_STRING_LENGTH];
  extern char greetings1[MAX_STRING_LENGTH];
  extern char greetings2[MAX_STRING_LENGTH];
  extern char greetings3[MAX_STRING_LENGTH];
  extern char greetings4[MAX_STRING_LENGTH];

  char arg[256];

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
    do_reload_help(ch, str_hsh(""));
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
    return eFAILURE;
  }

  return eSUCCESS;
}

int do_listproc(Character *ch, char *argument, cmd_t cmd)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int start, i, end, tot;
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
    return eFAILURE;
  }
  mob = !str_cmp(arg, "mob"); // typoed mob means obj. who cares.
  char buf[MAX_STRING_LENGTH];
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
      sprintf(buf, "%s[%-3d] [%-3d] %s\r\n", buf, tot, i, ((Character *)DC::getInstance()->mob_index[real_mobile(i)].item)->getNameC());
    }
    else
    {
      sprintf(buf, "%s[%-3d] [%-3d] %s\r\n", buf, tot, i, ((Object *)DC::getInstance()->obj_index[real_object(i)].item)->name);
    }
  }
  ch->send(buf);
  return eSUCCESS;
}
