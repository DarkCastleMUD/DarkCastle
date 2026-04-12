/********************************
| Level 108 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/DC.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include <QString>
#include "DC/const.h"

qint32 get_number(QString *name);

command_return_t do_zoneexits(CharacterPtr ch, QString argument, cmd_t cmd)
{
  //  try
  // {
  QString buf;
  QString output = "";
  room_direction_data *curExits;
  qint32 curZone = DC::getInstance()->world[(ch)->in_room].zone;
  qint32 curRoom = ch->in_room;
  ObjectPtr portal;
  qint32 i, dir;
  qint32 low, high;
  qint32 last_good = curRoom;

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You are unable to do this outside of your range.");
    return ReturnValue::eFAILURE;
  }

  ch->send(QStringLiteral("Searching Zone: %1 - %2\r\n").arg(curZone).arg(DC::getInstance()->zones.value(DC::getInstance()->world[curRoom].zone).Name()));
  for (low = curRoom; low > 0; low--)
  {
    if (!DC::getInstance()->rooms.contains(low - 1))
      continue;
    last_good = low;
    if (DC::getInstance()->world[low - 1].zone != curZone)
      break;
  }
  low = last_good;
  last_good = curRoom;
  for (high = curRoom; high < DC::getInstance()->top_of_world; high++)
  {
    if (!DC::getInstance()->rooms.contains(high + 1))
      continue;
    last_good = high;
    if (DC::getInstance()->world[high + 1].zone != curZone)
      break;
  }
  high = last_good;

  for (i = low; i < high; i++)
  {
    if (!DC::getInstance()->rooms.contains(i))
      continue;
    for (dir = {}; dir < 6; dir++)
    {
      if ((curExits = DC::getInstance()->world[i].dir_option[dir]) != 0)
      {
        if (curExits->to_room > 0 && DC::getInstance()->world[curExits->to_room].zone != curZone)
        {
          dc_sprintf(buf, "Room %5d - %5s to Room %5d, zone %3lu (%s)\r\n", i, dirs[dir], curExits->to_room, DC::getInstance()->world[curExits->to_room].zone, DC::getInstance()->zones.value(DC::getInstance()->world[curExits->to_room].zone).NameC());

          output += buf;
        }
      }
    }
    for (portal = DC::getInstance()->world[i].contents; portal; portal = portal->next_content)
    {
      if (portal->obj_flags.type_flag == ITEM_CLIMBABLE)
      {
        if (portal->obj_flags.value[0] < 0)
        {
          dc_sprintf(buf, "Room %5d - climb to Room %5lu (ERROR)\r\n",
                     i, real_room(portal->obj_flags.value[0]));

          output += buf;
        }
        else if (!DC::getInstance()->rooms.contains(portal->obj_flags.value[0]))
        {
          dc_sprintf(buf, "Room %5d - climb to Room %5lu (DOES NOT EXIST)\r\n",
                     i, real_room(portal->obj_flags.value[0]));

          output += buf;
        }
        else if (DC::getInstance()->world[real_room(portal->obj_flags.value[0])].zone != curZone)
        {
          dc_sprintf(buf, "Room %5d - climb to Room %5lu, zone %3lu (%s)\r\n", i, real_room(portal->obj_flags.value[0]), DC::getInstance()->world[real_room(portal->obj_flags.value[0])].zone, DC::getInstance()->zones.value(DC::getInstance()->world[real_room(portal->obj_flags.value[0])].zone).NameC());

          output += buf;
        }
      }

      if (portal->isPortal() && !portal->hasPortalFlagNoEnter() && (portal->isPortalTypePermanent() || portal->isPortalTypeTemp()))
      {
        if (real_room(portal->getPortalDestinationRoom()) == DC::NOWHERE)
        {
          dc_sprintf(buf, "Room %5d - enter to Room %5lu (ERROR)\r\n", i, real_room(portal->getPortalDestinationRoom()));

          output += buf;
        }
        else if (DC::getInstance()->world[real_room(portal->getPortalDestinationRoom())].zone != curZone)
        {
          dc_sprintf(buf, "Room %5d - enter to Room %5lu, zone %3lu (%s)\r\n", i, real_room(portal->getPortalDestinationRoom()), DC::getInstance()->world[real_room(portal->getPortalDestinationRoom())].zone, DC::getInstance()->zones.value(DC::getInstance()->world[real_room(portal->getPortalDestinationRoom())].zone).NameC());

          output += buf;
        }
      }
    }

    for (portal = DC::getInstance()->object_list; portal; portal = portal->next)
    {
      if ((portal->isPortal()) && (portal->isPortalTypePermanent() || (portal->isPortalTypeTemp())) && (portal->in_room != DC::NOWHERE) && !portal->hasPortalFlagNoLeave())
      {
        if ((portal->obj_flags.value[0] == DC::getInstance()->world[i].number) || (portal->obj_flags.value[2] == DC::getInstance()->world[i].zone))
        {
          if (DC::getInstance()->world[real_room(portal->in_room)].zone != curZone)
          {
            dc_sprintf(buf, "Room %5d - leave to Room %5lu, zone %3lu (%s)\r\n", i, real_room(portal->in_room), DC::getInstance()->world[real_room(portal->in_room)].zone, DC::getInstance()->zones.value(DC::getInstance()->world[real_room(portal->in_room)].zone).NameC());

            output += buf;
          }
        }
      }
    }
  }

  send_to_char(output.c_str(), ch);
  // }
  // catch(QString errmsg)
  // {
  //   ch->send(QStringLiteral("Error encountered while finding zone exits:\r\n%1\r\n").arg(errmsg));
  //   ch->sendln("Ask Rubicon if it needs fixed...");
  //   return ReturnValue::eFAILURE;
  //}

  return ReturnValue::eSUCCESS;
}

command_return_t do_purloin(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString bufName, *pBuf;
  ObjectPtr k;
  qint32 j, nIndex = {};

  if (!ch->has_skill(COMMAND_PURLOIN))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, bufName);

  // if the QString is nullptr, return.  Else assign pBuf to point to it.
  if (*(pBuf = bufName) == '\0')
  {
    send_to_char("Retrieves any item in the game and puts it in your inventory.\r\n"
                 "Works well in combination with the 'find obj' command.\r\n"
                 "Usage: purloin [number.]name\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  // pass pBuf's address to get_number.  It returns the index
  // specified by the player, or 1 (0 if an error).  It also
  // sets pBuf so it points to the name.
  if ((nIndex = get_number(&pBuf)) == 0)
  {
    ch->sendln("get_number failed.  bad input?");
    return ReturnValue::eFAILURE;
  }

  for (k = DC::getInstance()->object_list, j = 1; k && (j <= nIndex); k = k->next)
  {
    if (!(isexact(pBuf, k->name())))
      continue;
    if (!CAN_SEE_OBJ(ch, k))
      continue;

    if (k->carried_by && !CAN_SEE(ch, k->carried_by))
    {
      continue;
    }
    else if (k->equipped_by && !CAN_SEE(ch, k->equipped_by))
    {
      continue;
    }
    else if (k->in_obj)
    {
      if (k->in_obj->carried_by && !CAN_SEE(ch, k->in_obj->carried_by))
      {
        continue;
      }
      else if (k->in_obj->equipped_by && !CAN_SEE(ch, k->in_obj->equipped_by))
      {
        continue;
      }
    }

    if (j == nIndex)
    {
      CharacterPtr vict = {};
      if (k->carried_by)
      {
        vict = k->carried_by;
      }
      else if (k->equipped_by)
      {
        qint32 iEq;
        vict = k->equipped_by;
        for (iEq = {}; iEq < MAX_WEAR; iEq++)
        {
          if (vict->equipment[iEq] == k)
          {
            obj_to_char(vict->unequip_char(iEq), vict);
            break;
          }
        } // for
      }
      else if (k->in_obj)
      {
        if (k->in_obj->carried_by)
        {
          vict = k->in_obj->carried_by;
        }
        else if (k->in_obj->equipped_by)
        {
          vict = k->in_obj->equipped_by;
        }
      }
      if (vict != nullptr)
      {
        ch->send(QStringLiteral("You purloin %s from %s.\r\n").arg(k->short_description).arg(qPrintable(vict->name())));
        DC::getInstance()->logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s purloins %s from %s",
                                qPrintable(ch->name()), k->short_description, qPrintable(vict->name()));
      }
      else
      {
        ch->send(QStringLiteral("You purloin %1.\r\n").arg(k->short_description()));
      }
      move_obj(k, ch);
      return ReturnValue::eSUCCESS;
    }
    j++;
  }

  ch->sendln("Sorry, couldn't find it or something.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_set(CharacterPtr ch, QString argument, cmd_t cmd)
{
  //   renamed the command "setup" so don't need this anymore
  //    void do_mortal_set(CharacterPtr ch, QString argument, cmd_t cmd);
  //
  //    if(!ch->isImmortalPlayer() || ch->isNonPlayer()) {
  //      do_mortal_set(ch, argument, cmd);
  //      return;
  //    }

  /* from spell_parser.c */

  const QStringList values = {
      "age", "sex", "class", "level", "height", "weight", "str", "stradd",
      "qint32", "wis", "dex", "con", "gold", "exp", "mana", "hit", "move",
      "sessions", "alignment", "thirst", "drunk", "full", "race",
      "bank", "platinum", "ki", "clan", "saves_base", "hpmeta",
      "manameta", "movemeta", "armor", "profession", "\n"};
  CharacterPtr vict;
  QString name, buf2[100], buf[100], help;
  qint32 skill, value, i, x;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (!ch->has_skill(COMMAND_SET))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, name);
  if (!*name) // no arguments. print an informative text
  {
    ch->sendln("Usage:\r\nset <name> <field> <value>");

    dc_strcpy(help, "\r\nField being one of the following:\r\n");
    ch->display_string_list(values);
    /*        for (i = 1; *values[i] != '\n'; i++)
            {
                dc_sprintf(help + strlen(help), "%18s", values[i]);
                if (!(i % 4))
                {
                    dc_strcat(help, "\r\n");
                    ch->send(help);
                    *help = '\0';
                }
            }
            if (*help)
                ch->send(help);
            ch->sendln("");*/
    return ReturnValue::eFAILURE;
  }
  if (!(vict = get_char_vis(ch, name)))
  {
    ch->sendln("No living thing by that name.");
    return ReturnValue::eFAILURE;
  }

  if (ch->getLevel() < vict->getLevel())
  {
    ch->sendln("Get real! You ain't that big.");
    if (vict->isPlayer())
    {
      dc_sprintf(buf2, "%s just tried to set: %s\r\n", qPrintable(ch->name()), buf);
      send_to_char(buf2, vict);
    }
    return ReturnValue::eFAILURE;
  }

  if (vict->isPlayer() && (vict->getLevel() == IMPLEMENTER) && (qPrintable(vict->name()) != qPrintable(ch->name())))
  {
    ch->sendln("Forget it dweeb.");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, buf);
  if (!*buf)
  {
    ch->sendln("A field was expected.");
    return ReturnValue::eFAILURE;
  }

  skill = values.indexOf(buf, Qt::CaseInsensitive);
  if (skill < 0)
  {
    ch->sendln("That value not recognized.");
    return ReturnValue::eFAILURE;
  }
  argument = one_argument(argument, buf); /* update argument */

  dc_sprintf(buf2, "%s sets %s's %s to %s.", qPrintable(ch->name()), qPrintable(vict->name()), values[skill], buf);
  switch (skill)
  {
  case 0: /* age */
  {
    if (vict->isNonPlayer())
    {
      ch->sendln("Can't set a mob's age.");
      return ReturnValue::eFAILURE;
    }
    value = atoi(buf);
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set age of victim */
    vict->player->time.birth =
        time(0) - (qint32)value * (qint32)SECS_PER_MUD_YEAR;
  };
  break;
  case 1: /* sex */
  {
    if (str_cmp(buf, "m") && str_cmp(buf, "f") && str_cmp(buf, "n"))
    {
      ch->sendln("Sex must be 'm','f' or 'n'.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set sex of victim */
    switch (*buf)
    {
    case 'm':
      vict->sex = SEX_MALE;
      break;
    case 'f':
      vict->sex = SEX_FEMALE;
      break;
    case 'n':
      vict->sex = SEX_NEUTRAL;
      break;
    }
  }
  break;
  case 2: /* class */
  {
    if (str_cmp(buf, "m") && str_cmp(buf, "c") &&
        str_cmp(buf, "w") && str_cmp(buf, "t") &&
        str_cmp(buf, "a") && str_cmp(buf, "p") &&
        str_cmp(buf, "b") && str_cmp(buf, "k") &&
        str_cmp(buf, "r") && str_cmp(buf, "d") &&
        str_cmp(buf, "u"))

    {
      send_to_char("Class must be 'm','c','w','t','a','p','b',"
                   "'r', 'k'(monk), 'd'(bard), or 'u'(druid). \r\n",
                   ch);
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set class of victim */
    switch (*buf)
    {
    case 'm':
      vict->c_class = CLASS_MAGIC_USER;
      break;
    case 'c':
      vict->c_class = CLASS_CLERIC;
      break;
    case 'w':
      vict->c_class = CLASS_WARRIOR;
      break;
    case 't':
      vict->c_class = CLASS_THIEF;
      break;
    case 'a':
      vict->c_class = CLASS_ANTI_PAL;
      break;
    case 'p':
      vict->c_class = CLASS_PALADIN;
      break;
    case 'b':
      vict->c_class = CLASS_BARBARIAN;
      break;
    case 'k':
      vict->c_class = CLASS_MONK;
      break;
    case 'r':
      vict->c_class = CLASS_RANGER;
      break;
    case 'd':
      vict->c_class = CLASS_BARD;
      vict->add_to_bard_list();
      break;
    case 'u':
      vict->c_class = CLASS_DRUID;
      break;
    }
  }
  break;
  case 3: /* level */
  {
    value = atoi(buf);
    if (value > DC::MAX_MORTAL_LEVEL && value < MIN_GOD)
    {
      ch->sendln("That level doesn't exist!");
      return ReturnValue::eFAILURE;
    }
    if (((value < 0) || (value > DC::MAX_MORTAL_LEVEL)) && ch->getLevel() < OVERSEER)
    {
      ch->sendln("Level must be between 0 and 101.");
      return ReturnValue::eFAILURE;
    }
    /* why the fuck was ths missing? -Sadus */
    if (vict->isPlayer() && value > ch->getLevel())
    {
      ch->sendln("That level is higher than you!");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set level of victim */
    vict->setLevel(value);
    DC::getInstance()->update_wizlist(vict);
  }
  break;
  case 4: /* height */
  {
    value = atoi(buf);
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set height of victim */
    vict->height = value;
  }
  break;
  case 5: /* weight */
  {
    value = atoi(buf);
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set weight of victim */
    vict->weight = value;
  }
  break;
  case 6: /* str */
  {
    value = atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln("Strength must be more than 0");
      ch->sendln("and less than 26.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original strength of victim */
    vict->raw_str = value;
  }
  break;
  case 7: /* stradd */
  {
    ch->sendln("Strength addition not supported.");
  }
  break;
  case 8: /* qint32 */
  {
    value = atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln("Intelligence must be more than 0");
      ch->sendln("and less than 26.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original INT of victim */
    vict->raw_intel = value;
    redo_mana(vict);
    affect_total(vict);
  }
  break;
  case 9: /* wis */
  {
    value = atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln("Wisdom must be more than 0");
      ch->sendln("and less than 26.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original WIS of victim */
    vict->raw_wis = value;
  }
  break;
  case 10: /* dex */
  {
    value = atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln("Dexterity must be more than 0");
      ch->sendln("and less than 26.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original DEX of victim */
    vict->raw_dex = value;
  }
  break;
  case 11: /* con */
  {
    value = atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln("Constitution must be more than 0");
      ch->sendln("and less than 26.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original CON of victim */
    vict->raw_con = value;
    redo_hitpoints(vict);
  }
  break;
  case 12: /* gold */
  {
    value = atoi(buf);
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original gold of victim */
    vict->setGold(value);
  }
  break;
  case 13: /* exp */
  {
    qint64 val;
    val = atoll(buf);
    qint64 before_exp = vict->exp;
    vict->exp = val;
    DC::getInstance()->logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s sets %s's exp from %ld to %ld.",
                            qPrintable(ch->name()), qPrintable(vict->name()), before_exp, vict->exp);
  }
  break;
  case 14: /* mana */
  {
    value = atoi(buf);
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original mana of victim */
    vict->raw_mana = value;
    redo_mana(vict);
  }
  break;
  case 15: /* hit */
  {
    value = atoi(buf);
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original hit of victim */
    vict->raw_hit = value;
    redo_hitpoints(vict);
  }
  break;
  case 16: /* move */
  {
    value = atoi(buf);
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original move of victim */
    vict->raw_move = value;
  }
  break;
  case 17: /* sessions */
  {
    if (vict->isNonPlayer())
    {
      ch->sendln("Can't set a mob's pracs...");
      return ReturnValue::eFAILURE;
    }
    value = atoi(buf);
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original sessions of victim */
    vict->player->practices = value;
  }
  break;
  case 18: /* alignment */
  {
    value = atoi(buf);
    if ((value < -1000) || (value > 1000))
    {
      ch->sendln("Alignment must be more than -1000");
      ch->sendln("and less than 1000.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original alignment of victim */
    vict->alignment = value;
  }
  break;
  case 19: /* thirst */
  {
    value = atoi(buf);
    if ((value < -1) || (value > 100))
    {
      ch->sendln("Thirst must be more than -2");
      ch->sendln("and less than 101.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original thirst of victim */
    vict->conditions[THIRST] = value;
  }
  break;
  case 20: /* drunk */
  {
    value = atoi(buf);
    if ((value < -1) || (value > 100))
    {
      ch->sendln("Drunk must be more than -2");
      ch->sendln("and less than 101.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original drunk of victim */
    vict->conditions[DRUNK] = value;
  }
  break;
  case 21: /* full */
  {
    value = atoi(buf);
    if ((value < -1) || (value > 100))
    {
      ch->sendln("Full must be more than -2");
      ch->sendln("and less than 101.");
      return ReturnValue::eFAILURE;
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original full of victim */
    vict->conditions[FULL] = value;
  }
  break;
  case 22: /* race */
  {
    for (x = {}; x <= 31; x++)
    {
      if (x == 31)
      {
        ch->sendln("No such race.");
        return ReturnValue::eFAILURE;
      }
      if (isexact(races[x].singular_name, buf))
      {
        vict->race = x;
        vict->immune = races[(qint32)vict->race].immune;
        vict->suscept = races[(qint32)vict->race].suscept;
        vict->resist = races[(qint32)vict->race].resist;
        break;
      }
    }
    DC::getInstance()->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
  }
  break;
  case 23: /* bank */
  {
    GET_BANK(vict) = atoi(buf);
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 24: /* platinum */
  {
    if (ch->getLevel() == IMPLEMENTER)
    {
      quint32 before_plat = GET_PLATINUM(vict);
      GET_PLATINUM(vict) = atoi(buf);
      DC::getInstance()->logf(IMPLEMENTER, DC::LogChannel::LOG_GOD, "%s sets %s's platinum from %u to %u.",
                              qPrintable(ch->name()), qPrintable(vict->name()), before_plat, GET_PLATINUM(vict));
    }
  }
  break;
  case 25: /* ki */
  {
    vict->raw_ki = atoi(buf);
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 26: /* clan number */
  {
    vict->clan = atoi(buf);
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_BUG);
  }
  break;
  case 27: // saves
  {
    one_argument(argument, buf2);
    if (!*buf || !*buf2)
    {
      ch->sendln("Syntax: set <vict> saves <0-5> <num>");
      return ReturnValue::eFAILURE;
    }

    if (vict->isNonPlayer())
    {
      ch->sendln("You cannot set saves_bases on mobs.");
      return ReturnValue::eFAILURE;
    }

    if (!check_range_valid_and_convert(i, buf, 0, 5))
    {
      ch->sendln("Save type be from 0 to 5.");
      return ReturnValue::eFAILURE;
    }

    if (!check_range_valid_and_convert(value, buf2, 0, 5))
    {
      ch->sendln("Value must be from -10 to 100.");
      return ReturnValue::eFAILURE;
    }
    vict->player->saves_mods[i] = value;
  }
  break;
  case 28:
  {
    GET_HP_METAS(vict) = atoi(buf);
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 29:
  {
    GET_MANA_METAS(vict) = atoi(buf);
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 30:
  {
    GET_MOVE_METAS(vict) = atoi(buf);
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 31:
  {
    vict->armor = atoi(buf);
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 32:
  {
    if (!*buf)
    {
      ch->send(QStringLiteral("Syntax: set <vict> profession <0-%1>\r\n").arg(MAX_PROFESSIONS));
      return ReturnValue::eFAILURE;
    }

    if (vict->isNonPlayer())
    {
      ch->sendln("You cannot set profession on mobs.");
      return ReturnValue::eFAILURE;
    }

    if (!check_range_valid_and_convert(value, buf, 0, MAX_PROFESSIONS))
    {
      ch->send(QStringLiteral("Save type be from 0 to %1.\r\n").arg(MAX_PROFESSIONS));
      return ReturnValue::eFAILURE;
    }

    vict->player->profession = value;
    DC::getInstance()->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  }

  ch->sendln("Ok.");
  affect_total(vict);
  return ReturnValue::eSUCCESS;
}
