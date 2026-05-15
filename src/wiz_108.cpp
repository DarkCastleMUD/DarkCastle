/********************************
| Level 108 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "DC/DC.h"

qint32 get_number(QString *name);

ReturnValues do_zoneexits(CharacterPtr ch, QString argument, cmd_t cmd)
{
  //  try
  // {
  QString buf;
  QString output = "";
  RoomDirectionPtr curExits;
  qint32 curZone = dc_->world[(ch)->in_room]->zone;
  qint32 curRoom = ch->in_room;
  ObjectPtr portal;
  qint32 i, dir;
  qint32 low, high;
  qint32 last_good = curRoom;

  if (!can_modify_room(ch, ch->in_room))
  {
    ch->sendln(u"You are unable to do this outside of your range."_s);
    return ReturnValue::eFAILURE;
  }

  ch->send(u"Searching Zone: %1 - %2\r\n"_s.arg(curZone).arg(dc_->zones.value(dc_->world[curRoom].zone).Name()));
  for (low = curRoom; low > 0; low--)
  {
    if (!dc_->rooms.contains(low - 1))
      continue;
    last_good = low;
    if (dc_->world[low - 1].zone != curZone)
      break;
  }
  low = last_good;
  last_good = curRoom;
  for (high = curRoom; high < dc_->top_of_world; high++)
  {
    if (!dc_->rooms.contains(high + 1))
      continue;
    last_good = high;
    if (dc_->world[high + 1].zone != curZone)
      break;
  }
  high = last_good;

  for (i = low; i < high; i++)
  {
    if (!dc_->rooms.contains(i))
      continue;
    for (dir = {}; dir < 6; dir++)
    {
      if ((curExits = dc_->world[i].dir_option[dir]) != 0)
      {
        if (curExits->to_room > 0 && dc_->world[curExits->to_room].zone != curZone)
        {
          dc_sprintf(buf, "Room %5d - %5s to Room %5d, zone %3lu (%s)\r\n", i, dirs[dir], curExits->to_room, dc_->world[curExits->to_room].zone, dc_->zones.value(dc_->world[curExits->to_room].zone).NameC());

          output += buf;
        }
      }
    }
    for (portal = dc_->world[i].contents; portal; portal = portal->next_content)
    {
      if (portal->flags_.type_flag == ITEM_CLIMBABLE)
      {
        if (portal->flags_.value[0] < 0)
        {
          dc_sprintf(buf, "Room %5d - climb to Room %5lu (ERROR)\r\n",
                     i, real_room(portal->flags_.value[0]));

          output += buf;
        }
        else if (!dc_->rooms.contains(portal->flags_.value[0]))
        {
          dc_sprintf(buf, "Room %5d - climb to Room %5lu (DOES NOT EXIST)\r\n",
                     i, real_room(portal->flags_.value[0]));

          output += buf;
        }
        else if (dc_->world[real_room(portal->flags_.value[0])].zone != curZone)
        {
          dc_sprintf(buf, "Room %5d - climb to Room %5lu, zone %3lu (%s)\r\n", i, real_room(portal->flags_.value[0]), dc_->world[real_room(portal->flags_.value[0])].zone, dc_->zones.value(dc_->world[real_room(portal->flags_.value[0])].zone).NameC());

          output += buf;
        }
      }

      if (portal->isPortal() && !portal->hasPortalFlagNoEnter() && (portal->isPortalTypePermanent() || portal->isPortalTypeTemp()))
      {
        if (real_room(portal->getPortalDestinationRoom()) == INVALID_ROOM)
        {
          dc_sprintf(buf, "Room %5d - enter to Room %5lu (ERROR)\r\n", i, real_room(portal->getPortalDestinationRoom()));

          output += buf;
        }
        else if (dc_->world[real_room(portal->getPortalDestinationRoom())].zone != curZone)
        {
          dc_sprintf(buf, "Room %5d - enter to Room %5lu, zone %3lu (%s)\r\n", i, real_room(portal->getPortalDestinationRoom()), dc_->world[real_room(portal->getPortalDestinationRoom())].zone, dc_->zones.value(dc_->world[real_room(portal->getPortalDestinationRoom())].zone).NameC());

          output += buf;
        }
      }
    }

    for (portal = dc_->object_list; portal; portal = portal->next)
    {
      if ((portal->isPortal()) && (portal->isPortalTypePermanent() || (portal->isPortalTypeTemp())) && (portal->in_room != INVALID_ROOM) && !portal->hasPortalFlagNoLeave())
      {
        if ((portal->flags_.value[0] == dc_->world[i]->number_) || (portal->flags_.value[2] == dc_->world[i].zone))
        {
          if (dc_->world[real_room(portal->in_room)].zone != curZone)
          {
            dc_sprintf(buf, "Room %5d - leave to Room %5lu, zone %3lu (%s)\r\n", i, real_room(portal->in_room), dc_->world[real_room(portal->in_room)].zone, dc_->zones.value(dc_->world[real_room(portal->in_room)].zone).NameC());

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
  //   ch->send(u"Error encountered while finding zone exits:\r\n%1\r\n"_s.arg(errmsg));
  //   ch->sendln(u"Ask Rubicon if it needs fixed..."_s);
  //   return ReturnValue::eFAILURE;
  //}

  return ReturnValue::eSUCCESS;
}

ReturnValues do_purloin(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString bufName, *pBuf;
  ObjectPtr k;
  qint32 j, nIndex = {};

  if (!ch->has_skill(COMMAND_PURLOIN))
  {
    ch->sendln(u"Huh?"_s);
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
    ch->sendln(u"get_number failed.  bad input?"_s);
    return ReturnValue::eFAILURE;
  }

  for (k = dc_->object_list, j = 1; k && (j <= nIndex); k = k->next)
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
        ch->send(u"You purloin %s from %s.\r\n"_s.arg(k->short_description).arg(qPrintable(vict->name())));
        dc_->logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s purloins %s from %s",
                  qPrintable(ch->name()), k->short_description, qPrintable(vict->name()));
      }
      else
      {
        ch->send(u"You purloin %1.\r\n"_s.arg(k->short_description()));
      }
      move_obj(k, ch);
      return ReturnValue::eSUCCESS;
    }
    j++;
  }

  ch->sendln(u"Sorry, couldn't find it or something."_s);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_set(CharacterPtr ch, QString argument, cmd_t cmd)
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
  QString name, buf2, buf, help;
  qint32 skill, value, i, x;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (!ch->has_skill(COMMAND_SET))
  {
    ch->sendln(u"Huh?"_s);
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, name);
  if (name.isEmpty()) // no arguments. print an informative text
  {
    ch->sendln(u"Usage:\r\nset <name> <field> <value>"_s);

    dc_strcpy(help, "\r\nField being one of the following:\r\n");
    ch->display_string_list(values);
    /*        for (i = 1; *values[i] != '\n'; i++)
            {
                dc_sprintf(help + dc_strlen(help), "%18s", values[i]);
                if (!(i % 4))
                {
                    dc_strcat(help, "\r\n");
                    ch->send(help);
                    *help = '\0';
                }
            }
            if (!help.isEmpty())
                ch->send(help);
            ch->sendln(u""_s);*/
    return ReturnValue::eFAILURE;
  }
  if (!(vict = get_char_vis(ch, name)))
  {
    ch->sendln(u"No living thing by that name."_s);
    return ReturnValue::eFAILURE;
  }

  if (ch->getLevel() < vict->getLevel())
  {
    ch->sendln(u"Get real! You ain't that big."_s);
    if (vict->isPlayer())
    {
      dc_sprintf(buf2, "%s just tried to set: %s\r\n", qPrintable(ch->name()), buf);
      send_to_char(buf2, vict);
    }
    return ReturnValue::eFAILURE;
  }

  if (vict->isPlayer() && (vict->getLevel() == IMPLEMENTER) && (qPrintable(vict->name()) != qPrintable(ch->name())))
  {
    ch->sendln(u"Forget it dweeb."_s);
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, buf);
  if (buf.isEmpty())
  {
    ch->sendln(u"A field was expected."_s);
    return ReturnValue::eFAILURE;
  }

  skill = values.indexOf(buf, Qt::CaseInsensitive);
  if (skill < 0)
  {
    ch->sendln(u"That value not recognized."_s);
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
      ch->sendln(u"Can't set a mob's age."_s);
      return ReturnValue::eFAILURE;
    }
    value = dc_atoi(buf);
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set age of victim */
    vict->player->time.birth =
        time(0) - (qint32)value * (qint32)SECS_PER_MUD_YEAR;
  };
  break;
  case 1: /* sex */
  {
    if (str_cmp(buf, "m") && str_cmp(buf, "f") && str_cmp(buf, "n"))
    {
      ch->sendln(u"Sex must be 'm','f' or 'n'."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set sex of victim */
    switch (*buf)
    {
    case 'm':
      vict->sex = Character::sex_t::MALE;
      break;
    case 'f':
      vict->sex = Character::sex_t::FEMALE;
      break;
    case 'n':
      vict->sex = Character::sex_t::NEUTRAL;
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
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
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
    value = dc_atoi(buf);
    if (value > DC::MAX_MORTAL_LEVEL && value < MIN_GOD)
    {
      ch->sendln(u"That level doesn't exist!"_s);
      return ReturnValue::eFAILURE;
    }
    if (((value < 0) || (value > DC::MAX_MORTAL_LEVEL)) && ch->getLevel() < OVERSEER)
    {
      ch->sendln(u"Level must be between 0 and 101."_s);
      return ReturnValue::eFAILURE;
    }
    /* why the fuck was ths missing? -Sadus */
    if (vict->isPlayer() && value > ch->getLevel())
    {
      ch->sendln(u"That level is higher than you!"_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set level of victim */
    vict->setLevel(value);
    dc_->update_wizlist(vict);
  }
  break;
  case 4: /* height */
  {
    value = dc_atoi(buf);
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set height of victim */
    vict->height = value;
  }
  break;
  case 5: /* weight */
  {
    value = dc_atoi(buf);
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set weight of victim */
    vict->weight = value;
  }
  break;
  case 6: /* str */
  {
    value = dc_atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln(u"Strength must be more than 0"_s);
      ch->sendln(u"and less than 26."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original strength of victim */
    vict->raw_str = value;
  }
  break;
  case 7: /* stradd */
  {
    ch->sendln(u"Strength addition not supported."_s);
  }
  break;
  case 8: /* qint32 */
  {
    value = dc_atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln(u"Intelligence must be more than 0"_s);
      ch->sendln(u"and less than 26."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original INT of victim */
    vict->raw_intel = value;
    redo_mana(vict);
    affect_total(vict);
  }
  break;
  case 9: /* wis */
  {
    value = dc_atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln(u"Wisdom must be more than 0"_s);
      ch->sendln(u"and less than 26."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original WIS of victim */
    vict->raw_wis = value;
  }
  break;
  case 10: /* dex */
  {
    value = dc_atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln(u"Dexterity must be more than 0"_s);
      ch->sendln(u"and less than 26."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original DEX of victim */
    vict->raw_dex = value;
  }
  break;
  case 11: /* con */
  {
    value = dc_atoi(buf);
    if ((value <= 0) || (value > 30))
    {
      ch->sendln(u"Constitution must be more than 0"_s);
      ch->sendln(u"and less than 26."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original CON of victim */
    vict->raw_con = value;
    redo_hitpoints(vict);
  }
  break;
  case 12: /* gold */
  {
    value = dc_atoi(buf);
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
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
    dc_->logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s sets %s's exp from %ld to %ld.",
              qPrintable(ch->name()), qPrintable(vict->name()), before_exp, vict->exp);
  }
  break;
  case 14: /* mana */
  {
    value = dc_atoi(buf);
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original mana of victim */
    vict->raw_mana = value;
    redo_mana(vict);
  }
  break;
  case 15: /* hit */
  {
    value = dc_atoi(buf);
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original hit of victim */
    vict->raw_hit = value;
    redo_hitpoints(vict);
  }
  break;
  case 16: /* move */
  {
    value = dc_atoi(buf);
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original move of victim */
    vict->raw_move = value;
  }
  break;
  case 17: /* sessions */
  {
    if (vict->isNonPlayer())
    {
      ch->sendln(u"Can't set a mob's pracs..."_s);
      return ReturnValue::eFAILURE;
    }
    value = dc_atoi(buf);
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original sessions of victim */
    vict->player->practices = value;
  }
  break;
  case 18: /* alignment */
  {
    value = dc_atoi(buf);
    if ((value < -1000) || (value > 1000))
    {
      ch->sendln(u"Alignment must be more than -1000"_s);
      ch->sendln(u"and less than 1000."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original alignment of victim */
    vict->alignment = value;
  }
  break;
  case 19: /* thirst */
  {
    value = dc_atoi(buf);
    if ((value < -1) || (value > 100))
    {
      ch->sendln(u"Thirst must be more than -2"_s);
      ch->sendln(u"and less than 101."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original thirst of victim */
    vict->conditions[THIRST] = value;
  }
  break;
  case 20: /* drunk */
  {
    value = dc_atoi(buf);
    if ((value < -1) || (value > 100))
    {
      ch->sendln(u"Drunk must be more than -2"_s);
      ch->sendln(u"and less than 101."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
    /* set original drunk of victim */
    vict->conditions[DRUNK] = value;
  }
  break;
  case 21: /* full */
  {
    value = dc_atoi(buf);
    if ((value < -1) || (value > 100))
    {
      ch->sendln(u"Full must be more than -2"_s);
      ch->sendln(u"and less than 101."_s);
      return ReturnValue::eFAILURE;
    }
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
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
        ch->sendln(u"No such race."_s);
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
    dc_->logentry(buf2, IMPLEMENTER, DC::LogChannel::LOG_GOD);
  }
  break;
  case 23: /* bank */
  {
    GET_BANK(vict) = dc_atoi(buf);
    dc_->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 24: /* platinum */
  {
    if (ch->getLevel() == IMPLEMENTER)
    {
      quint32 before_plat = GET_PLATINUM(vict);
      GET_PLATINUM(vict) = dc_atoi(buf);
      dc_->logf(IMPLEMENTER, DC::LogChannel::LOG_GOD, "%s sets %s's platinum from %u to %u.",
                qPrintable(ch->name()), qPrintable(vict->name()), before_plat, GET_PLATINUM(vict));
    }
  }
  break;
  case 25: /* ki */
  {
    vict->raw_ki = dc_atoi(buf);
    dc_->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 26: /* clan number */
  {
    vict->clan = dc_atoi(buf);
    dc_->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_BUG);
  }
  break;
  case 27: // saves
  {
    one_argument(argument, buf2);
    if (buf.isEmpty() || buf.isEmpty() 2)
    {
      ch->sendln(u"Syntax: set <vict> saves <0-5> <num>"_s);
      return ReturnValue::eFAILURE;
    }

    if (vict->isNonPlayer())
    {
      ch->sendln(u"You cannot set saves_bases on mobs."_s);
      return ReturnValue::eFAILURE;
    }

    if (!check_range_valid_and_convert(i, buf, 0, 5))
    {
      ch->sendln(u"Save type be from 0 to 5."_s);
      return ReturnValue::eFAILURE;
    }

    if (!check_range_valid_and_convert(value, buf2, 0, 5))
    {
      ch->sendln(u"Value must be from -10 to 100."_s);
      return ReturnValue::eFAILURE;
    }
    vict->player->saves_mods[i] = value;
  }
  break;
  case 28:
  {
    GET_HP_METAS(vict) = dc_atoi(buf);
    dc_->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 29:
  {
    GET_MANA_METAS(vict) = dc_atoi(buf);
    dc_->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 30:
  {
    GET_MOVE_METAS(vict) = dc_atoi(buf);
    dc_->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 31:
  {
    vict->armor = dc_atoi(buf);
    dc_->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  case 32:
  {
    if (buf.isEmpty())
    {
      ch->send(u"Syntax: set <vict> profession <0-%1>\r\n"_s.arg(MAX_PROFESSIONS));
      return ReturnValue::eFAILURE;
    }

    if (vict->isNonPlayer())
    {
      ch->sendln(u"You cannot set profession on mobs."_s);
      return ReturnValue::eFAILURE;
    }

    if (!check_range_valid_and_convert(value, buf, 0, MAX_PROFESSIONS))
    {
      ch->send(u"Save type be from 0 to %1.\r\n"_s.arg(MAX_PROFESSIONS));
      return ReturnValue::eFAILURE;
    }

    vict->player->profession = value;
    dc_->logentry(buf2, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  break;
  }

  ch->sendln(u"Ok."_s);
  affect_total(vict);
  return ReturnValue::eSUCCESS;
}
