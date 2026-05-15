
/****************************************************************************
 * file: act_info.c , Implementation of commands.	 Part of DIKUMUD    *
 * Usage : Informative commands. 					    *
 * Copyright (C) 1990, 1991 - see 'license.doc' for complete information.   *
 *                                                                          *
 * Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse    *
 * Performance optimization and bug fixes by MERC Industries.		    *
 * You can use our stuff in any way you like whatsoever so long as ths	    *
 * copyright notice remains intact.  If you like it please drop a line	    *
 * to mec@garnet.berkeley.edu.						    *
 * 									    *
 * This is free software and you are benefitting.	We hope that you    *
 * share your changes too.  What goes around, comes around. 		    *
 ****************************************************************************/
/* $Id: info.cpp,v 1.210 2015/06/14 02:38:12 pirahna Exp $ */
#include "DC/DC.h"

qint32 get_saves(CharacterPtr ch, qint32 savetype)
{
  qint32 save = ch->saves[savetype];
  switch (savetype)
  {
  case SAVE_TYPE_MAGIC:
    save += int_app[GET_INT(ch)].magic_resistance;
    break;
  case SAVE_TYPE_COLD:
    save += str_app[GET_STR(ch)].cold_resistance;
    break;
  case SAVE_TYPE_ENERGY:
    save += wis_app[GET_WIS(ch)].energy_resistance;
    break;
  case SAVE_TYPE_FIRE:
    save += dex_app[GET_DEX(ch)].fire_resistance;
    break;
  case SAVE_TYPE_POISON:
    save += con_app[GET_CON(ch)].poison_resistance;
    break;
  default:
    break;
  }
  return save;
}

/* Procedures related to 'look' */

void argument_split_3(QString argument, QString &first_arg, QString &second_arg, QString &third_arg)
{
  auto arguments = argument.trimmed().toLower().split(' ');
  first_arg = arguments.value(0);
  second_arg = arguments.value(1);
  third_arg = arguments.value(2);
}

ObjectPtr Character::get_object_in_equip_vis(QString arg, ObjectPtr equipment[], qint32 *j, bool blindfighting)
{
  qint32 k, num;
  QString tmpname;
  QString tmp;

  dc_strcpy(tmpname, arg);
  tmp = tmpname;
  if ((num = get_number(&tmp)) < 0)
    return {};

  for ((*j) = 0, k = 1; ((*j) < MAX_WEAR) && (k <= num); (*j)++)
    if (equipment[(*j)])
      if (CAN_SEE_OBJ(this, equipment[(*j)], blindfighting))
        if (isexact(tmp, equipment[(*j)]->name()))
        {
          if (k == num)
            return (equipment[(*j)]);
          k++;
        }

  return {};
}

QString find_ex_description(QString word, ExtraDescriptionPtr list)
{
  ExtraDescriptionPtr i;

  for (i = list; i; i = i->next)
    if (isexact(word, i->keyword_))
      return (i->description_);

  return {};
}

const QString item_condition(ObjectPtr object)
{
  qint32 percent = 100 - (qint32)(100 * ((qreal)eq_current_damage(object) / (qreal)eq_max_damage(object)));

  if (percent >= 100)
    return " [$B$2Excellent$R]";
  else if (percent >= 80)
    return " [$2Good$R]";
  else if (percent >= 60)
    return " [$3Decent$R]";
  else if (percent >= 40)
    return " [$B$5Damaged$R]";
  else if (percent >= 20)
    return " [$4Quite Damaged$R]";
  else if (percent >= 0)
    return " [$B$4Falling Apart$R]";
  else
    return " [$5Pile of Scraps$R]";
}

void Character::show_obj_to_char(ObjectPtr object, qint32 mode)
{
  QString buffer;
  QString flagbuf;
  qint32 found = {};
  //   qint32 percent;

  // Don't show NO_NOTICE items in a room with "look" unless they have holylite
  if (mode == 0 && isSet(object->flags_.more_flags, ITEM_NONOTICE) &&
      (player && !player->holyLite))
    return;

  buffer[0] = '\0';
  if ((mode == 0) && !object->long_description().isEmpty())
    dc_strcpy(buffer, qPrintable(object->long_description()));
  else if (!object->short_description().isEmpty() && ((mode == 1) ||
                                                      (mode == 2) || (mode == 3) || (mode == 4)))
    dc_strcpy(buffer, qPrintable(object->short_description()));
  else if (mode == 5)
  {
    if (object->flags_.type_flag == ITEM_NOTE)
    {
      if (!object->ActionDescription().isEmpty())
      {
        dc_strncpy(buffer, "There is something written upon it:\r\n\r\n", sizeof(buffer) - 1);
        dc_strncat(buffer, qPrintable(object->ActionDescription()), sizeof(buffer) - 1);
        page_string(conn_, buffer, 1);
      }
      else
        act_to_character("It's blank.", this, 0, 0, 0);
      return;
    }
    else if ((object->flags_.type_flag != ITEM_DRINKCON))
    {
      dc_strcpy(buffer, "You see nothing special.");
    }
    else /* ITEM_TYPE == ITEM_DRINKCON */
    {
      dc_strcpy(buffer, "It looks like a drink container.");
    }
  }

  if (mode != 3)
  {
    if (mode == 0)             // 'look'
      dc_strcat(buffer, "$R"); // setup color background

    dc_strcpy(flagbuf, " $B($R");

    if (IS_OBJ_STAT(object, ITEM_INVISIBLE))
    {
      dc_strcat(flagbuf, "Invisible");
      found++;
    }
    if (IS_OBJ_STAT(object, ITEM_MAGIC) && IS_AFFECTED(this, AFF_DETECT_MAGIC))
    {
      if (found)
        dc_strcat(flagbuf, "$B/$R");
      dc_strcat(flagbuf, "Blue Glow");
      found++;
    }
    if (IS_OBJ_STAT(object, ITEM_GLOW))
    {
      if (found)
        dc_strcat(flagbuf, "$B/$R");
      dc_strcat(flagbuf, "Glowing");
      found++;
    }
    if (IS_OBJ_STAT(object, ITEM_HUM))
    {
      if (found)
        dc_strcat(flagbuf, "$B/$R");
      dc_strcat(flagbuf, "Humming");
      found++;
    }
    if (mode == 0 && isSet(object->flags_.more_flags, ITEM_NONOTICE))
    {
      if (found)
        dc_strcat(flagbuf, "$B/$R");
      dc_strcat(flagbuf, "NO_NOTICE");
      found++;
    }
    if (mode == 0 && isSet(object->flags_.more_flags, ITEM_NOSEE))
    {
      if (found)
        dc_strcat(flagbuf, "$B/$R");
      dc_strcat(flagbuf, "NO_SEE");
      found++;
    }
    if (found)
    {
      dc_strcat(flagbuf, "$B)$R");
      dc_strcat(buffer, flagbuf);
    }

    /* show object's condition if is an armor...  */
    if (object->flags_.type_flag == ITEM_ARMOR ||
        object->flags_.type_flag == ITEM_WEAPON ||
        object->flags_.type_flag == ITEM_FIREWEAPON ||
        object->flags_.type_flag == ITEM_CONTAINER ||
        IS_KEYRING(object) ||
        object->flags_.type_flag == ITEM_INSTRUMENT ||
        object->flags_.type_flag == ITEM_WAND ||
        object->flags_.type_flag == ITEM_STAFF ||
        object->flags_.type_flag == ITEM_LIGHT)
    {
      dc_strcat(buffer, item_condition(object)); /*
            percent = 100 - (qint32)(100 * ((qreal)eq_current_damage(object) / (qreal)eq_max_damage(object)));

            if (percent >= 100)
               dc_strcat(buffer, " [$B$2Excellent$R]");
            else if (percent >= 80)
               dc_strcat(buffer, " [$2Good$R]");
            else if (percent >= 60)
               dc_strcat(buffer, " [$3Decent$R]");
            else if (percent >= 40)
               dc_strcat(buffer, " [$B$5Damaged$R]");
            else if (percent >= 20)
               dc_strcat(buffer, " [$4Quite Damaged$R]");
            else if (percent >= 0)
               dc_strcat(buffer, " [$B$4Falling Apart$R]");
            else dc_strcat(buffer, " [$5Pile of Scraps$R]");
   */
    }
    if (isSet(object->flags_.more_flags, ITEM_24H_SAVE) && !isSet(object->flags_.extra_flags, ITEM_NOSAVE))
    {
      time_t now = time(nullptr);
      time_t expires = object->save_expiration;
      if (expires == 0)
      {
        dc_strcat(buffer, " $R($B$0unsaved$R)");
      }
      else if (now >= expires)
      {
        dc_strcat(buffer, " $R($B$0expired$R)");
      }
      else
      {
        QString timebuffer;
        dc_snprintf(timebuffer, 100, " $R($B$0%lu secs left$R)", expires - now);
        dc_strcat(buffer, timebuffer);
      }
    }

    if (isSet(object->flags_.more_flags, ITEM_24H_NO_SELL))
    {
      time_t now = time(nullptr);
      time_t expires = object->no_sell_expiration;
      if (now >= expires || expires == 0)
      {
        dc_strcat(buffer, " $R($B$0sellable$R)");
      }
      else
      {
        QString timebuffer = {};

        dc_snprintf(timebuffer, 100, " $R($B$0No sell for %lu secs$R)", expires - now);
        dc_strcat(buffer, timebuffer);
      }
    }

    if (isSet(object->flags_.more_flags, ITEM_POOF_AFTER_24H))
    {
      if (object->flags_.timer)
      {
        QString timebuffer = {};
        dc_snprintf(timebuffer, 100, " $R($B$0%hd ticks left$R)", object->flags_.timer);
        dc_strcat(buffer, timebuffer);
      }
    }

    if (mode == 0)               // 'look'
      dc_strcat(buffer, "$B$1"); // setup color background
  }

  dc_strcat(buffer, "\r\n");
  page_string(conn_, buffer, 1);
}

void Character::list_obj_to_char(ObjectPtr list, qint32 mode, bool show)
{
  ObjectPtr i;
  bool found = false;
  qint32 number = 1;
  qint32 can_see;
  QString buf;

  for (i = list; i; i = i->next_content)
  {
    if ((can_see = CAN_SEE_OBJ(this, i)) && i->next_content &&
        i->next_content->item_number == i->item_number && i->item_number != -1 && !isSet(i->flags_.more_flags, ITEM_NONOTICE))
    {
      number++;
      continue;
    }
    if (can_see || number > 1)
    {
      if (number > 1)
      {
        dc_sprintf(buf, "[%d] ", number);
        send(buf);
      }
      show_obj_to_char(i, mode);
      found = true;
      number = 1;
    }
  }

  if ((!found) && (show))
    sendln(u"Nothing"_s);
}

void show_spells(CharacterPtr i, CharacterPtr ch)
{
  QString strbuf;

  if (IS_AFFECTED(i, AFF_SANCTUARY))
  {
    strbuf = strbuf + "$7aura! ";
  }

  if (i->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
  {
    strbuf = strbuf + "$R$6pulsing! ";
  }
  else if (i->affected_by_spell(SPELL_PROTECT_FROM_GOOD))
  {
    strbuf = strbuf + "$R$6$Bpulsing! ";
  }

  if (IS_AFFECTED(i, AFF_FIRESHIELD))
  {
    strbuf = strbuf + "$B$4flames! ";
  }

  if (IS_AFFECTED(i, AFF_FROSTSHIELD))
  {
    strbuf = strbuf + "$B$3frost! ";
  }

  if (IS_AFFECTED(i, AFF_ACID_SHIELD))
  {
    strbuf = strbuf + "$B$2acid! ";
  }

  if (i->affected_by_spell(SPELL_BARKSKIN))
  {
    strbuf = strbuf + "$R$5woody! ";
  }

  if (IS_AFFECTED(i, AFF_LIGHTNINGSHIELD))
  {
    strbuf = strbuf + "$B$5energy! ";
  }

  if (IS_AFFECTED(i, AFF_PARALYSIS))
  {
    strbuf = strbuf + "$R$2paralyze! ";
  }

  if (i->affected_by_spell(SPELL_STONE_SHIELD) || i->affected_by_spell(SPELL_GREATER_STONE_SHIELD))
  {
    strbuf = strbuf + "$B$0stones! ";
  }

  if (IS_AFFECTED(i, AFF_FLYING))
  {
    strbuf = strbuf + "$B$1flying!";
  }

  if (!strbuf.isEmpty())
  {
    if (i->isNonPlayer())
      strbuf = QString("$B$7-$1") + qPrintable(i->shortdesc_or_name()) + " has: " + strbuf + "$R\r\n";
    else
      strbuf = QString("$B$7-$1") + qPrintable(i->name()) + " has: " + strbuf + "$R\r\n";

    send_to_char(strbuf.c_str(), ch);
  }
}

void show_char_to_char(CharacterPtr i, CharacterPtr ch, qint32 mode)
{
  QString buffer;
  ObjectPtr tmp_obj{};
  ClanPtr clan{};
  qint32 j{}, found{}, percent{};

  if (mode == 0)
  {
    if (!CAN_SEE(ch, i))
    {
      if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
      {
        ch->sendln(u"$R$7You sense a hidden life form in the room."_s);
      }

      return;
    }
    ch->send(u"$B$3"_s);

    if (i->long_description().isEmpty() || (i->isNonPlayer() && (GET_POS(i) != i->mobdata->default_pos)))
    {
      /* A character without long descr, or not in default pos. */
      if (i->isPlayer())
      {
        if (!i->conn_)
          buffer = "*linkdead*  ";
        if (isSet(i->player->toggles, Player::PLR_GUIDE_TOG))
          buffer.append("$B$7(Guide)$B$3 ");

        buffer.append(qPrintable(i->shortdesc_or_name()));
        if ((i->getLevel() < OVERSEER) && i->clan && (clan = get_clan(i)))
        {
          if (ch->isPlayer() && !isSet(ch->player->toggles, Player::PLR_BRIEF))
          {
            buffer.append(u" %1 [%2]"_s.arg(i->title_).arg(clan->name()));
          }
          else
          {
            buffer.append(u" the %1 [%2]"_s.arg(races[(qint32)i->race].singular_name).arg(clan->name()));
          }
        }
        else
        {
          if (!ch->isNonPlayer() && !isSet(ch->player->toggles, Player::PLR_BRIEF))
          {
            buffer.append(" ");
            buffer.append(i->title_);
          }
          else
          {
            buffer.append(" the ");
            buffer.append(u"%1"_s.arg(races[(qint32)i->race].singular_name));
          }
        }
      }

      if (i->isNonPlayer())
      {
        buffer = i->short_description();
        buffer[0] = buffer[0].toUpper();
      }

      switch (GET_POS(i))
      {
      case position_t::STUNNED:
        buffer.append(" is on the ground, stunned.");
        break;
      case position_t::DEAD:
        buffer.append(" is lying here, dead.");
        break;
      case position_t::STANDING:
        buffer.append(" is here.");
        break;
      case position_t::SITTING:
        buffer.append(" is sitting here.");
        break;
      case position_t::RESTING:
        buffer.append(" is resting here.");
        break;
      case position_t::SLEEPING:
        buffer.append(" is sleeping here.");
        break;
      case position_t::FIGHTING:
        if (i->fighting)
        {
          buffer.append(" is here, fighting ");

          if (i->fighting == ch)
          {
            buffer.append("YOU!");
          }
          else
          {
            if (i->in_room == i->fighting->in_room)
            {
              buffer.append(qPrintable(i->fighting->shortdesc_or_name()));
            }
            else
            {
              buffer.append("someone who has already left.");
            }
          }
        }
        else
        { /* NIL fighting pointer */
          buffer.append(" is here struggling with thin air.");
        }
        break;
      default:
        buffer.append(" is floating here.");
        break;
      }

      if (IS_AFFECTED(i, AFF_INVISIBLE))
        buffer.append(" $1(invisible) ");
      if (IS_AFFECTED(i, AFF_HIDE) && ((IS_AFFECTED(ch, AFF_TRUE_SIGHT) && ch->has_skill(SPELL_TRUE_SIGHT) > 80) || ch->getLevel() > IMMORTAL || ARE_GROUPED(i, ch)))
        buffer.append(" $4(hidden) ");
      if ((IS_AFFECTED(ch, AFF_DETECT_EVIL) || IS_AFFECTED(ch, AFF_KNOW_ALIGN)) && IS_EVIL(i))
        buffer.append(" $B$4(red halo) ");
      if ((IS_AFFECTED(ch, AFF_DETECT_GOOD) || IS_AFFECTED(ch, AFF_KNOW_ALIGN)) && IS_GOOD(i))
        buffer.append(" $B$1(blue halo) ");
      if (IS_AFFECTED(ch, AFF_KNOW_ALIGN) && !IS_GOOD(i) && !IS_EVIL(i))
        buffer.append(" $B$5(yellow halo) ");
      if (IS_AFFECTED(i, AFF_CHAMPION))
        buffer.append(" $B$4(Champion) ");

      buffer.append("$R\r\n");
      send_to_char(buffer, ch);

      show_spells(i, ch);
    }
    else /* npc with long */
    {
      if (IS_AFFECTED(i, AFF_INVISIBLE))
      {
        buffer = "$B$7*$3";
      }
      else
      {
        buffer = "";
      }

      if (IS_AFFECTED(i, AFF_HIDE) && IS_AFFECTED(ch, AFF_TRUE_SIGHT) && ch->has_skill(SPELL_TRUE_SIGHT) > 80)
        buffer.append(" $4(hidden) $3");
      if ((IS_AFFECTED(ch, AFF_DETECT_EVIL) || IS_AFFECTED(ch, AFF_KNOW_ALIGN)) && IS_EVIL(i))
        buffer.append(" $B$4(red halo)$3 ");
      if ((IS_AFFECTED(ch, AFF_DETECT_GOOD) || IS_AFFECTED(ch, AFF_KNOW_ALIGN)) && IS_GOOD(i))
        buffer.append(" $B$1(blue halo)$3 ");
      if (IS_AFFECTED(ch, AFF_KNOW_ALIGN) && !IS_GOOD(i) && !IS_EVIL(i))
        buffer.append(" $B$5(yellow halo)$3 ");

      buffer.append(i->long_description());

      send_to_char(buffer, ch);

      show_spells(i, ch);
      ch->send(u"$R$7"_s);
    }
  }
  else if ((mode == 1) || (mode == 3))
  {
    if (mode == 1)
    {
      if (!i->description().isEmpty())
      {
        ch->send(i->description());
      }
      else
      {
        act_to_victim("You see nothing special about $m.", i, 0, ch, 0);
      }
    }

    /* Show a character to another */

    if (GET_MAX_HIT(i) > 0)
    {
      percent = (100 * i->getHP()) / GET_MAX_HIT(i);
    }
    else
    {
      percent = -1; /* How could MAX_HIT be < 1?? */
    }

    buffer = qPrintable(i->shortdesc_or_name());

    buffer.append(u" the %1"_s.arg(races[(qint32)i->race].singular_name));

    if (percent >= 100)
      buffer.append(" is in excellent condition.\r\n");
    else if (percent >= 90)
      buffer.append(" has a few scratches.\r\n");
    else if (percent >= 75)
      buffer.append(" is slightly hurt.\r\n");
    else if (percent >= 50)
      buffer.append(" is fairly fucked up.\r\n");
    else if (percent >= 30)
      buffer.append(" is bleeding freely.\r\n");
    else if (percent >= 15)
      buffer.append(" is covered in blood.\r\n");
    else if (percent >= 0)
      buffer.append(" is near death.\r\n");
    else
      buffer.append(" is suffering from a slow death.\r\n");

    send_to_char(buffer, ch);

    if (mode == 3)
    { // If it was a glance, show spells then get out
      show_spells(i, ch);
      return;
    }

    found = false;
    for (j = {}; j < MAX_WEAR; j++)
    {
      if (i->equipment[j])
      {
        if (CAN_SEE_OBJ(ch, i->equipment[j]))
        {
          found = true;
        }
      }
    }

    if (found)
    {
      act_to_victim("\r\n$n is using:", i, 0, ch, 0);
      act_to_victim("<    worn     > Item Description     (Flags) [Item Condition]\r\n", i, 0, ch, 0);

      for (j = {}; j < MAX_WEAR; j++)
      {
        if (i->equipment[j])
        {
          if (CAN_SEE_OBJ(ch, i->equipment[j]))
          {
            send_to_char(where[j], ch);
            ch->show_obj_to_char(i->equipment[j], 1);
          }
        }
      }
    }

    if ((GET_CLASS(ch) == CLASS_THIEF && ch != i) || ch->getLevel() > IMMORTAL)
    {
      found = false;
      ch->sendln(u"\r\nYou attempt to peek at the inventory:"_s);
      for (tmp_obj = i->carrying; tmp_obj;
           tmp_obj = tmp_obj->next_content)
      {
        if ((isSet(tmp_obj->flags_.extra_flags, ITEM_QUEST) == false ||
             ch->getLevel() > IMMORTAL) &&
            CAN_SEE_OBJ(ch, tmp_obj) &&
            dc_->number(0ULL, MORTAL) < ch->getLevel())
        {
          ch->show_obj_to_char(tmp_obj, 1);
          found = true;
        }
      }
      if (!found)
        ch->sendln(u"You can't see anything."_s);
    }
  }
  else if (mode == 2)
  {
    /* Lists inventory */
    act_to_victim("$n is carrying:", i, 0, ch, 0);
    ch->list_obj_to_char(i->carrying, 1, true);
  }
}

ReturnValues Character::do_botcheck(QStringList arguments, cmd_t cmd)
{
  QString name = arguments.value(0);
  if (name.isEmpty())
  {
    sendln(u"botcheck <player> or all\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  QString name2 = "0." + name;
  CharacterPtr victim = get_char(name2);

  if (!victim && name == "all")
  {
    ConnectionPtr conn;
    CharacterPtr i;

    for (auto &d : dc_->connections_)
    {
      if (conn->connected || !conn->character)
        continue;
      if (!(i = conn->original))
        i = conn->character;
      if (!CAN_SEE(this, i))
        continue;
      sendln(u"\r\n%1"_s.arg(i->name()));
      sendln(u"----------"_s);
      do_botcheck(i->name().split(' '));
    }
    return ReturnValue::eSUCCESS;
  }

  if (victim == nullptr)
  {
    sendln(u"Unable to find %1."_s.arg(name));
    return ReturnValue::eFAILURE;
  }

  if (victim->getLevel() > getLevel())
  {
    sendln(u"Unable to show information."_s);
    send(u"%s is a higher level than you.\r\n"_s.arg(qPrintable(victim->name())));
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer())
  {
    sendln(u"Unable to show information."_s);
    send(u"%s is a mob.\r\n"_s.arg(qPrintable(victim->name())));
    return ReturnValue::eFAILURE;
  }

  if (victim->player->lastseen == 0)
    victim->player->lastseen = new std::multimap<qint32, std::pair<timeval, timeval>>;

  if (victim->player->lastseen->size() == 0)
  {
    send(u"%s has not seen any mobs recently.\r\n"_s.arg(qPrintable(victim->name())));
    return ReturnValue::eFAILURE;
  }

  qint32 nr, ms;
  timeval seen, targeted;
  double ts1, ts2;
  for (std::multimap<qint32, std::pair<timeval, timeval>>::iterator i = victim->player->lastseen->begin(); i != victim->player->lastseen->end(); ++i)
  {
    nr = (*i).first;
    seen = (*i).second.first;
    targeted = (*i).second.second;

    ts1 = seen.tv_sec + ((double)seen.tv_usec / 1000000.0);
    ts2 = targeted.tv_sec + ((double)targeted.tv_usec / 1000000.0);

    if (ts2 > ts1)
    {
      ms = (qint32)((ts2 - ts1) * 1000.0);
    }
    else
    {
      ms = {};
    }

    if (nr >= 0)
    {
      send(u"[%4dms] [%5d] [%s]\r\n"_s.arg(ms).arg(dc_->mob_index_[nr]->vnum()).arg(qPrintable(((CharacterPtr)(dc_->mob_index_[nr]->item))->short_description())));
    }
  }

  return ReturnValue::eSUCCESS;
}

void Character::list_char_to_char(CharacterPtr list, qint32 mode)
{
  bool clear_lastseen = false;
  CharacterPtr i;
  qint32 known = has_skill(SKILL_BLINDFIGHTING);
  timeval tv, tv_zero = {0, 0};

  for (i = list; i; i = i->next_in_room)
  {
    if (this == i)
      continue;
    if (!i->isNonPlayer() && (i->player->wizinvis > getLevel()))
      if (!i->player->incognito || !(in_room == i->in_room))
        continue;
    if (IS_AFFECTED(this, AFF_SENSE_LIFE) || CAN_SEE(this, i))
    {
      show_char_to_char(i, this, 0);

      if (isPlayer() && i->isNonPlayer())
      {
        if (player->lastseen == 0)
          player->lastseen = new std::multimap<qint32, std::pair<timeval, timeval>>;

        if (clear_lastseen == false)
        {
          player->lastseen->clear();
          clear_lastseen = true;
        }

        gettimeofday(&tv, nullptr);
        player->lastseen->insert(std::pair<qint32, std::pair<timeval, timeval>>(i->mobdata->nr, std::pair<timeval, timeval>(tv, tv_zero)));
      }
    }
    else if (IS_DARK(in_room))
    {
      if (known && skill_success(nullptr, SKILL_BLINDFIGHTING))
        sendln(u"Your blindfighting awareness alerts you to a presense in the area."_s);
      else if (ch->dc_->number(1, 10) == 1)
        sendln(u"$B$4You see a pair of glowing red eyes looking your way.$R$7"_s);
    }
  }
}

void try_to_peek_into_container(CharacterPtr vict, CharacterPtr ch,
                                QString container)
{
  ObjectPtr obj = {};
  ObjectPtr cont = {};
  qint32 found = false;

  if (GET_CLASS(ch) != CLASS_THIEF && ch->getLevel() < DEITY)
  {
    ch->send(u"They might object to you trying to look in their pockets..."_s);
    return;
  }

  if (!(cont = get_obj_in_list_vis(ch, container, vict->carrying)) ||
      dc_->number(level_(0), MORTAL + 30) > ch->getLevel())
  {
    ch->sendln(u"You cannot see a container named that to peek into."_s);
    return;
  }

  if (NOT_CONTAINERS(cont))
  {
    ch->sendln(u"It's not a container...."_s);
    return;
  }

  QString buf;
  dc_sprintf(buf, "You attempt to peek into the %s.\r\n", qPrintable(cont->short_description()));
  ch->send(buf);

  if (isSet(cont->flags_.value[1], CONT_CLOSED))
  {
    ch->sendln(u"It is closed."_s);
    return;
  }

  for (obj = cont->contains; obj; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj) && dc_->number(level_(0), MORTAL + 30) < ch->getLevel())
    {
      ch->show_obj_to_char(obj, 1);
      found = true;
    }

  if (!found)
    ch->sendln(u"You don't see anything inside it."_s);
}

QString Character::getStatDiff(qint32 base, qint32 random, bool swapcolors)
{
  QString buf, buf2;
  QString color_good = "$2";
  QString color_bad = "$4";

  if (player)
  {
    color_good = getSettingAsColor("color.good");
    color_bad = getSettingAsColor("color.bad");
  }
  else
  {
    return QString();
  }

  // original value
  buf2 = u"%1"_s.arg(base);
  buf += buf2;

  if (random - base > 0)
  {
    // if postive show "+ difference"
    if (swapcolors)
    {
      buf2 = u"%1+%2$R"_s.arg(color_bad).arg(random - base);
    }
    else
    {
      buf2 = u"%1+%2$R"_s.arg(color_good).arg(random - base);
    }
    buf += buf2;
  }
  else if (random - base < 0)
  {
    // if negative show "- difference"
    if (swapcolors)
    {
      buf2 = u"%s%d$R"_s.arg(color_good).arg(random - base);
    }
    else
    {
      buf2 = u"%1%2$R"_s.arg(color_bad).arg(random - base);
    }
    buf += buf2;
  }
  buf += "$R";

  return buf;
}
void showStatDiff(CharacterPtr ch, qint32 base, qint32 random, bool swapcolors)
{
  ch->send(ch->getStatDiff(base, random, swapcolors));
}

bool identify(CharacterPtr ch, ObjectPtr obj)
{
  if (ch == nullptr || obj == nullptr)
  {
    return false;
  }

  QString buf, buf2;
  qint32 i = 0, value = 0, bits = {};
  bool found = false;

  if (obj->isDark() && !ch->isImmortalPlayer())
  {
    ch->sendln(u"A magical aura around the item attempts to conceal its secrets."_s);
    return false;
  }

  if (!obj->short_description().isEmpty())
  {
    ch->send(u"$3Short description: $R%1\r\n"_s.arg(obj->short_description()));
  }
  else
  {
    ch->sendln(u"$3Short description: $R"_s);
  }

  ch->send(u"$3Keywords: '$R%1$3'$R\r\n"_s.arg(obj->name()));

  sprinttype(GET_ITEM_TYPE(obj), item_types, buf2);
  ch->send(u"$3Item type: $R%s\r\n"_s.arg(buf2));

  sprintbit(obj->flags_.extra_flags, Object::extra_bits, buf);
  ch->send(u"$3Extra flags: $R%1\r\n"_s.arg(buf));

  sprintbit(obj->flags_.more_flags, Object::more_obj_bits, buf2);
  ch->send(u"$3More flags: $R%s\r\n"_s.arg(buf2));

  if (isSet(obj->flags_.more_flags, ITEM_NO_TRADE) || isSet(obj->flags_.more_flags, ITEM_NPC_CORPSE) || isSet(obj->flags_.more_flags, ITEM_PC_CORPSE) || isSet(obj->flags_.more_flags, ITEM_PC_CORPSE_LOOTED))
  {
    ch->send(u"$3Owner: $R%s\r\n"_s.arg(qPrintable(obj->getOwner())));
  }

  ch->send(u"$3Worn on: $R%1\r\n"_s.arg(QFlagsToStrings(obj->flags_.wear_flags)));

  sprintbit(obj->flags_.size, Object::size_bits, buf);
  ch->send(u"$3Worn by: $R%1\r\n"_s.arg(buf));

  ch->send(u"$3Level: $R%1\r\n"_s.arg(obj->flags_.eq_level));
  ch->send(u"$3Weight: $R%1\r\n"_s.arg(obj->flags_.weight));
  ch->send(u"$3Value: $R%1\r\n"_s.arg(obj->flags_.cost));

  const ObjectPtr vobj = {};
  if (obj->item_number >= 0)
  {
    const qint32 vnum = dc_->obj_index_[obj->item_number]->vnum();
    if (vnum >= 0)
    {
      const qint32 rn_of_vnum = real_object(vnum);
      if (rn_of_vnum >= 0)
      {
        vobj = dc_->obj_index_[rn_of_vnum]->item;
      }
    }
  }

  switch (GET_ITEM_TYPE(obj))
  {

  case ITEM_SCROLL:
  case ITEM_POTION:
    ch->send(u"$3Level $R%d "_s.arg(obj->flags_.value[0]));

    if (vobj != nullptr)
    {
      ch->send(u"("_s);
      showStatDiff(ch, vobj->flags_.value[0], obj->flags_.value[0]);
      ch->send(u") "_s);
    }
    ch->sendln(u"$3spells of:$R"_s);

    if (obj->flags_.value[1] >= 1)
    {
      sprinttype(obj->flags_.value[1] - 1, spells, buf);
      dc_strcat(buf, "\r\n");
      ch->send(buf);
    }
    if (obj->flags_.value[2] >= 1)
    {
      sprinttype(obj->flags_.value[2] - 1, spells, buf);
      dc_strcat(buf, "\r\n");
      ch->send(buf);
    }
    if (obj->flags_.value[3] >= 1)
    {
      sprinttype(obj->flags_.value[3] - 1, spells, buf);
      dc_strcat(buf, "\r\n");
      ch->send(buf);
    }
    break;

  case ITEM_WAND:
  case ITEM_STAFF:
    dc_sprintf(buf, "$3Has $R%d$3 charges, with $R%d$3 charges left.$R\r\n",
               obj->flags_.value[1],
               obj->flags_.value[2]);
    ch->send(buf);

    dc_sprintf(buf, "$3Level $R%d$3 spell of:$R\r\n", obj->flags_.value[0]);
    ch->send(buf);

    if (obj->flags_.value[3] >= 1)
    {
      sprinttype(obj->flags_.value[3] - 1, spells, buf);
      dc_strcat(buf, "\r\n");
      ch->send(buf);
    }
    break;

  case ITEM_WEAPON:
    ch->send(u"$3Damage Dice are '$R%dD%d$3'$R"_s.arg(obj->flags_.value[1]).arg(       obj->flags_.value[2]);

    if (vobj != nullptr)
    {
      ch->send(u" ("_s);
      showStatDiff(ch, vobj->flags_.value[1], obj->flags_.value[1]);
      ch->send(u"D"_s);
      showStatDiff(ch, vobj->flags_.value[2], obj->flags_.value[2]);
      ch->send(u")"_s);
    }
    ch->sendln(u""_s);

    qint32 get_weapon_damage_type(ObjectPtr  wielded);
    bits = get_weapon_damage_type(obj) - 1000;
    extern QStringList strs_damage_types;
    ch->send(u"$3Damage type$R: %s\r\n"_s.arg(strs_damage_types[bits]));
    break;

  case ITEM_INSTRUMENT:
    dc_sprintf(buf, "$3Affects non-combat singing by '$R%d$3'$R\r\n$3Affects combat singing by '$R%d$3'$R\r\n",
            obj->flags_.value[0],
            obj->flags_.value[1]);
    ch->send(buf);
    break;

  case ITEM_MISSILE:
    dc_sprintf(buf, "$3Damage Dice are '$R%dD%d$3'$R\r\nIt is +%d to arrow hit and +%d to arrow damage\r\n",
            obj->flags_.value[0],
            obj->flags_.value[1],
            obj->flags_.value[2],
            obj->flags_.value[3]);
    ch->send(buf);
    break;

  case ITEM_FIREWEAPON:
    dc_sprintf(buf, "$3Bow is +$R%d$3 to arrow hit and +$R%d$3 to arrow damage.$R\r\n",
            obj->flags_.value[0],
            obj->flags_.value[1]);
    ch->send(buf);
    break;

  case ITEM_ARMOR:

    if (isSet(obj->flags_.extra_flags, ITEM_ENCHANTED))
    {
      value = (obj->flags_.value[0]) - (obj->flags_.value[1]);
    }
    else
    {
      value = obj->flags_.value[0];
    }

    dc_sprintf(buf, "$3AC-apply is $R%d (", value);
    ch->send(buf);
    if (vobj != nullptr)
    {
      showStatDiff(ch, vobj->flags_.value[0], obj->flags_.value[0]);
    }
    if (isSet(obj->flags_.extra_flags, ITEM_ENCHANTED))
    {
      ch->send(u"-%d"_s.arg(obj->flags_.value[1]));
    }
    ch->send(u")$3     Resistance to damage is $R%d\r\n"_s.arg(obj->flags_.value[2]));
    break;
  }

  found = false;

  for (i = {}; i < obj->num_affects; i++)
  {
    if ((obj->affected[i].location != APPLY_NONE) &&
        (obj->affected[i].modifier != 0 ||
         (vobj != nullptr &&
          i < vobj->num_affects &&
          !vobj->affected.isEmpty() &&
          vobj->affected[i].location == obj->affected[i].location)))
    {
      if (!found)
      {
        ch->sendln(u"$3Can affect you as:$R"_s);
        found = true;
      }

      if (obj->affected[i].location < 1000)
        sprinttype(obj->affected[i].location, apply_types, buf2);
      else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
        dc_strcpy(buf2, get_skill_name(obj->affected[i].location / 1000).toStdString().c_str());
      else
        dc_strcpy(buf2, "Invalid");
      ch->send(u"    $3Affects : $R%s$3 By $R%d"_s.arg(buf2).arg(obj->affected[i].modifier));

      if (vobj != nullptr &&
          i < vobj->num_affects &&
          !vobj->affected.isEmpty() &&
          vobj->affected[i].location == obj->affected[i].location)
      {
        ch->send(u" ("_s);
        // Swap color for ARMOR so lower values use "good" color
        if (vobj->affected[i].location == 17)
        {
          showStatDiff(ch, vobj->affected[i].modifier, obj->affected[i].modifier, true);
        }
        else
        {
          showStatDiff(ch, vobj->affected[i].modifier, obj->affected[i].modifier);
        }

        ch->send(u")"_s);
      }

      ch->send(u"\r\n"_s.arg(buf));
    }
  }

  return true;
}

ReturnValues Character::do_identify(QStringList arguments, cmd_t cmd)
{
  if (arguments.isEmpty())
  {
    send(u"What object do you want to identify?\r\n"_s);
    return ReturnValue::eFAILURE;
  }
  QString arg1 = arguments.at(0);

  QRegularExpression re("^v(?<vnum>\\d+)$");
  QRegularExpressionMatch match = re.match(arg1);
  ObjectPtr obj = {};
  if (match.hasMatch())
  {
    QString buffer = match.captured("vnum");

    vnum_t vnum = buffer.toULongLong();
    vnum_t rnum = real_object(vnum);
    if (rnum == -1)
    {
      send(u"Invalid VNUM.\r\n"_s);
      return ReturnValue::eFAILURE;
    }
    obj = dc_->obj_index_[rnum]->item;

    if (obj->isDark() && !isImmortalPlayer())
    {
      send(u"This object cannot be identified by mortals.\r\n"_s);
      return ReturnValue::eFAILURE;
    }
    return identify(this, obj);
  }
  else
  {
    CharacterPtr tmp_char;
    qint32 bits = generic_find(arg1.toStdString().c_str(), FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, this, &tmp_char, &obj, true);
    if (bits && obj)
    {
      if (identify(this, obj))
      {
        return ReturnValue::eSUCCESS;
      }
    }
    else
    {
      send(u"You could not find %1 in your inventory, among your equipment or in this room.\r\n"_s.arg(arg1));
    }
  }

  return ReturnValue::eFAILURE;
}

ReturnValues do_look(CharacterPtr ch, const QString argument, cmd_t cmd)
{
  QString buffer;
  QString arg1;
  QString arg2;
  QString arg3;
  QString tmpbuf;
  qint32 keyword_no = {};
  qint32 j = 0, bits = 0, temp = {};
  qint32 door = 0, original_loc = {};
  bool found = {};
  ObjectPtr tmp_object = {}, found_object = {};
  CharacterPtr tmp_char = {};
  static const QStringList keywords = {
      "north",
      "east",
      "south",
      "west",
      "up",
      "down",
      "in",
      "at",
      "out",
      "through",
      "", /* Look at '' case */
  };

  qint32 weight_in(ObjectPtr obj);
  if (!ch->conn_)
    return 1;
  if (GET_POS(ch) < position_t::SLEEPING)
    ch->sendln(u"You can't see anything but stars!"_s);
  else if (GET_POS(ch) == position_t::SLEEPING)
    ch->sendln(u"You can't see anything, you're sleeping!"_s);
  else if (check_blind(ch))
  {
    ansi_color(GREY, ch);
    return ReturnValue::eSUCCESS;
  }
  else if (IS_DARK(ch->in_room) && (!ch->isNonPlayer() && !ch->player->holyLite))
  {
    ch->sendln(u"It is pitch black..."_s);
    ch->list_char_to_char(dc_->world[ch->in_room]->people_, 0);
    ch->send(u"$R"_s);
    // TODO - if have blindfighting, list some of the room exits sometimes
  }
  else
  {
    argument_split_3(argument, arg1, arg2, arg3);
    keyword_no = search_list(arg1, keywords); /* Partial Match */

    if ((keyword_no == -1) && *arg1)
    {
      keyword_no = 7;
      dc_strcpy(arg2, arg1); /* Let arg2 become the target object (arg1) */
    }

    found = false;
    tmp_object = {};
    tmp_char = {};

    original_loc = ch->in_room;
    switch (keyword_no)
    {
    /* look <dir> */
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    {
      /* Check if there is an extra-desc with "up"(or whatever) and use that instead */
      auto tmp_desc = find_ex_description(arg1, dc_->world[ch->in_room].ex_description);
      if (!tmp_desc.isEmpty())
      {
        page_string(ch->conn_, qPrintable(tmp_desc), 0);
        return ReturnValue::eSUCCESS;
      }

      if (EXIT(ch, keyword_no))
      {
        if (EXIT(ch, keyword_no)->general_description && dc_strlen(EXIT(ch, keyword_no)->general_description))
        {
          send_to_char(EXIT(ch, keyword_no)->general_description, ch);
        }
        else
        {
          ch->sendln(u"You see nothing special."_s);
        }

        if (isSet(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) && !isSet(EXIT(ch, keyword_no)->exit_info, EX_HIDDEN) && (EXIT(ch, keyword_no)->keyword))
        {
          dc_sprintf(buffer, "The %s is closed.\r\n", qPrintable(fname(EXIT(ch, keyword_no)->keyword)));
          ch->send(buffer);
        }
        else
        {
          if (isSet(EXIT(ch, keyword_no)->exit_info, EX_ISDOOR) && !isSet(EXIT(ch, keyword_no)->exit_info, EX_HIDDEN) &&
              EXIT(ch, keyword_no)->keyword)
          {
            dc_sprintf(buffer, "The %s is open.\r\n", qPrintable(fname(EXIT(ch, keyword_no)->keyword)));
            ch->send(buffer);
          }
        }
      }
      else
      {
        ch->sendln(u"You see nothing special."_s);
      }
    }
    break;

      /* look 'in'	 */
    case 6:
    {
      if (*arg2)
      {
        /* Item carried */

        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

        if (bits)
        { /* Found something */
          if (GET_ITEM_TYPE(tmp_object) == ITEM_DRINKCON)
          {
            if (tmp_object->flags_.value[1] <= 0)
            {
              act_to_character("It is empty.", ch, 0, 0, 0);
            }
            else
            {
              temp = ((tmp_object->flags_.value[1] * 3) / tmp_object->flags_.value[0]);
              if (temp > 3)
              {
                dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD,
                          "Bug in object %d. v2: %d > v1: %d. Resetting.",
                          dc_->obj_index_[tmp_object->item_number]->vnum(),
                          tmp_object->flags_.value[1],
                          tmp_object->flags_.value[0]);
                tmp_object->flags_.value[1] =
                    tmp_object->flags_.value[0];
                temp = 3;
              }

              dc_sprintf(buffer, "It's %sfull of a %s liquid.\r\n",
                         fullness[temp],
                         color_liquid[tmp_object->flags_.value[2]]);
              ch->send(buffer);
            }
          }
          else if (ARE_CONTAINERS(tmp_object))
          {
            if (!isSet(tmp_object->flags_.value[1], CONT_CLOSED))
            {
              send_to_char(fname(tmp_object->name()), ch);
              switch (bits)
              {
              case FIND_OBJ_INV:
                ch->send(u" (carried) "_s);
                break;
              case FIND_OBJ_ROOM:
                ch->send(u" (in room) "_s);
                break;
              case FIND_OBJ_EQUIP:
                ch->send(u" (equipped) "_s);
                break;
              }

              if (tmp_object->flags_.value[0] && tmp_object->flags_.weight)
              {

                qint32 weight_in(ObjectPtr obj);
                if (dc_->obj_index_[tmp_object->item_number]->vnum() == 536)
                  temp = (3 * weight_in(tmp_object)) / tmp_object->flags_.value[0];
                else
                  temp = ((tmp_object->flags_.weight * 3) / tmp_object->flags_.value[0]);
              }
              else
              {
                temp = 3;
              }

              if (temp < 0)
              {
                temp = {};
              }
              else if (temp > 3)
              {
                temp = 3;
                dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD,
                          "Bug in object %d. Weight: %d v1: %d",
                          dc_->obj_index_[tmp_object->item_number]->vnum(),
                          tmp_object->flags_.weight,
                          tmp_object->flags_.value[0]);
              }

              if (NOT_KEYRING(tmp_object))
              {
                ch->send(u"(%sfull) : \r\n"_s.arg(fullness[temp]));
              }
              else
              {
                ch->sendln(u": "_s);
              }

              ch->list_obj_to_char(tmp_object->contains, 2, true);
            }
            else
              ch->sendln(u"It is closed."_s);
          }
          else
          {
            ch->sendln(u"That is not a container."_s);
          }
        }
        else
        { /* wrong argument */
          ch->sendln(u"You do not see that item here."_s);
        }
      }
      else
      { /* no argument */
        ch->sendln(u"Look in what?!"_s);
      }
    }
    break;

      /* look 'at'	 */
    case 7:
    {
      if (*arg2)
      {

        bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &tmp_char, &found_object);

        if (tmp_char)
        {
          if (*arg3)
          {
            try_to_peek_into_container(tmp_char, ch, arg3);
            return ReturnValue::eSUCCESS;
          }
          if (cmd == cmd_t::GLANCE)
            show_char_to_char(tmp_char, ch, 3);
          else
            show_char_to_char(tmp_char, ch, 1);
          if (ch != tmp_char)
          {
            if (!ch->isNonPlayer() && (tmp_char->getLevel() < ch->player->wizinvis))
            {
              return ReturnValue::eSUCCESS;
            }
            if ((cmd == cmd_t::GLANCE) && !IS_AFFECTED(ch, AFF_HIDE))
            {
              act("$n glances at you.", ch, 0, tmp_char, TO_VICT, INVIS_NULL);
              act("$n glances at $N.", ch, 0, tmp_char, TO_ROOM, INVIS_NULL | NOTVICT);
            }
            else if (!IS_AFFECTED(ch, AFF_HIDE))
            {
              act("$n looks at you.", ch, 0, tmp_char, TO_VICT, INVIS_NULL);
              act("$n looks at $N.", ch, 0, tmp_char, TO_ROOM, INVIS_NULL | NOTVICT);
            }
          }
          return ReturnValue::eSUCCESS;
        }

        /* Search for Extra Descriptions in room and items */

        /* Extra description in room?? */
        if (!found)
        {
          auto tmp_desc = find_ex_description(arg2, dc_->world[ch->in_room].ex_description);
          if (!tmp_desc.isEmpty())
          {
            page_string(ch->conn_, qPrintable(tmp_desc), 0);
            return ReturnValue::eSUCCESS; /* RETURN SINCE IT WAS ROOM DESCRIPTION */
                                          /* Old system was: found = true; */
          }
        }

        /* Search for extra descriptions in items */

        /* Equipment Used */

        if (!found)
        {
          for (j = {}; j < MAX_WEAR && !found; j++)
          {
            if (ch->equipment[j])
            {
              if (CAN_SEE_OBJ(ch, ch->equipment[j]))
              {
                auto tmp_desc = find_ex_description(arg2, ch->equipment[j]->ex_description);
                if (!tmp_desc.isEmpty())
                {
                  page_string(ch->conn_, qPrintable(tmp_desc), 1);
                  return ReturnValue::eSUCCESS;
                  //                              found = true;
                }
              }
            }
          }
        }

        /* In inventory */

        if (!found)
        {
          for (tmp_object = ch->carrying; tmp_object && !found;
               tmp_object = tmp_object->next_content)
          {
            if (CAN_SEE_OBJ(ch, tmp_object))
            {
              auto tmp_desc = find_ex_description(arg2, tmp_object->ex_description);
              if (!tmp_desc.isEmpty())
              {
                page_string(ch->conn_, qPrintable(tmp_desc), 1);
                return ReturnValue::eSUCCESS;
                //                           found = true;
              }
            }
          }
        }

        /* Object In room */

        if (!found)
        {
          for (tmp_object = dc_->world[ch->in_room].contents;
               tmp_object && !found;
               tmp_object = tmp_object->next_content)
          {
            if (CAN_SEE_OBJ(ch, tmp_object))
            {
              auto tmp_desc = find_ex_description(arg2, tmp_object->ex_description);
              if (!tmp_desc.isEmpty())
              {
                page_string(ch->conn_, qPrintable(tmp_desc), 1);
                return ReturnValue::eSUCCESS;
                //                           found = true;
              }
            }
          }
        }
        /* wrong argument */

        if (bits)
        { /* If an object was found */
          if (!found)
            /* Show no-description */
            ch->show_obj_to_char(found_object, 5);
          else
            /* Find hum, glow etc */
            ch->show_obj_to_char(found_object, 6);
        }
        else if (!found)
        {
          ch->sendln(u"You do not see that here."_s);
        }
      }
      else
      {
        /* no argument */
        ch->sendln(u"Look at what?"_s);
      }
    }
    break;
    case 8:
    { // look out
      for (tmp_object = dc_->object_list; tmp_object;
           tmp_object = tmp_object->next)
      {
        if (tmp_object->isPortal() && tmp_object->getPortalDestinationRoom() == ch->in_room && tmp_object->in_room != INVALID_ROOM && tmp_object->isPortalTypePermanent())
        {
          ch->in_room = tmp_object->in_room;
          found = true;
          break;
        }
      }
      if (found != true)
      {
        ch->sendln(u"Nothing much to see there."_s);
        return ReturnValue::eFAILURE;
      }
    }
      /* no break */
    case 9: // look through
      if (found != true)
      {
        if (*arg2)
        {
          if ((tmp_object = get_obj_in_list_vis(ch, arg2,
                                                dc_->world[ch->in_room].contents)))
          {
            if (tmp_object->isPortal())
            {
              if (tmp_object->isPortalTypePlayer() || tmp_object->isPortalTypePermanentNoLook())
              {
                dc_sprintf(tmpbuf, "You look through %s but it seems to be opaque.\r\n", qPrintable(tmp_object->short_description()));
                ch->send(tmpbuf);
                return ReturnValue::eFAILURE;
              }
              if ((ch->in_room = real_room(tmp_object->getPortalDestinationRoom())) == INVALID_ROOM)
                ch->in_room = original_loc;
              else
                found = true;
            }
          }
          else
          {
            ch->sendln(u"Look through what?"_s);
            return ReturnValue::eFAILURE;
          }
        }
      }

      if (found != true)
      {
        ch->sendln(u"You can't seem to look through that."_s);
        return ReturnValue::eFAILURE;
      }
      /* no break */
      /* no break */
      /* look ''		*/
    case 10:
    {
      QString sector_buf;
      QString rflag_buf;
      QString tempflag_buf;

      ansi_color(GREY, ch);
      ansi_color(BOLD, ch);
      send_to_char(dc_->world[ch->in_room]->name_, ch);
      ansi_color(NTEXT, ch);
      ansi_color(GREY, ch);

      // PUT SECTOR AND ROOMFLAG STUFF HERE
      if (!ch->isNonPlayer() && ch->player->holyLite)
      {
        sprinttype(dc_->world[ch->in_room].sector_type, sector_types,
                   sector_buf);
        sprintbit((qint32)dc_->world[ch->in_room]->room_flags_, room_bits,
                  rflag_buf);
        ch->send(u"\r\nLight[%d] <%s> [ %s]"_s.arg(DARK_AMOUNT(ch->in_room)).arg(sector_buf).arg(rflag_buf));
        if (dc_->world[ch->in_room].temp_room_flags)
        {
          sprintbit((qint32)dc_->world[ch->in_room].temp_room_flags, temp_room_bits,
                    tempflag_buf);
          ch->send(u" [ %1]"_s.arg(tempflag_buf));
        }
      }

      ch->sendln(u""_s);

      if (!ch->isNonPlayer() && !isSet(ch->player->toggles, Player::PLR_BRIEF))
        send_to_char(dc_->world[ch->in_room].description, ch);

      ansi_color(BLUE, ch);
      ansi_color(BOLD, ch);
      ch->list_obj_to_char(dc_->world[ch->in_room].contents, 0, false);
      ch->list_char_to_char(dc_->world[ch->in_room]->people_, 0);

      dc_strcpy(buffer, "");
      *buffer = '\0';
      for (qint32 doorj = {}; doorj <= 5; doorj++)
      {

        bool is_closed;
        bool is_hidden;

        // cheesy way of making it list west before east in 'look'
        if (doorj == 1)
          door = 3;
        else if (doorj == 3)
          door = 1;
        else
          door = doorj;

        if (!EXIT(ch, door) || EXIT(ch, door)->to_room == INVALID_ROOM)
          continue;
        is_closed = isSet(EXIT(ch, door)->exit_info, EX_CLOSED);
        is_hidden = isSet(EXIT(ch, door)->exit_info, EX_HIDDEN);

        if (ch->isNonPlayer() || ch->player->holyLite)
        {
          if (is_closed && is_hidden)
            dc_sprintf(buffer + dc_strlen(buffer), "$B($R%s-closed$B)$R ",
                       keywords[door]);
          else
            dc_sprintf(buffer + dc_strlen(buffer), "%s%s ",
                       keywords[door], is_closed ? "-closed" : "");
        }
        else if (!(is_closed && is_hidden))
          dc_sprintf(buffer + dc_strlen(buffer), "%s%s ", keywords[door],
                     is_closed ? "-closed" : "");
      }
      ansi_color(NTEXT, ch);
      ch->send(u"Exits: "_s);
      if (!buffer.isEmpty())
        ch->send(buffer);
      else
        ch->send(u"None."_s);
      ch->sendln(u""_s);
      if (ch->isPlayer() && !ch->hunting.isEmpty())
        ch->do_track(QString(ch->hunting).split(' '), cmd_t::TRACK);
    }
      ch->in_room = original_loc;
      break;

      /* wrong arg 	*/
    case -1:
      ch->sendln(u"Sorry, I didn't understand that!"_s);
      break;
    }

    ansi_color(NTEXT, ch);
  }
  ansi_color(GREY, ch);
  return ReturnValue::eSUCCESS;
}

/* end of look */

ReturnValues do_read(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString buf;

  // This is just for now - To be changed later.!

  // yeah right.  -Sadus
  dc_sprintf(buf, "at %s", arg);
  do_look(ch, buf);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_examine(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name, buf;
  CharacterPtr tmp_char;
  ObjectPtr tmp_object;

  dc_sprintf(buf, "at %s", argument);
  do_look(ch, buf);

  one_argument(argument, name);

  if (name.isEmpty())
  {
    ch->sendln(u"Examine what?"_s);
    return ReturnValue::eFAILURE;
  }

  generic_find(name, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &tmp_char, &tmp_object, true);

  if (tmp_object)
  {
    if (GET_ITEM_TYPE(tmp_object) == ITEM_DRINKCON || ARE_CONTAINERS(tmp_object))
    {
      ch->sendln(u"When you look inside, you see:"_s);
      dc_sprintf(buf, "in %s", argument);
      do_look(ch, buf);
    }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_exits(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 door;
  QString buf;
  const QStringList exits = {
      "North",
      "East ",
      "South",
      "West ",
      "Up   ",
      "Down "};

  if (check_blind(ch))
    return ReturnValue::eFAILURE;

  for (door = {}; door <= 5; door++)
  {
    if (!EXIT(ch, door) || EXIT(ch, door)->to_room == INVALID_ROOM)
      continue;

    if (!ch->isNonPlayer() && ch->player->holyLite)
      dc_sprintf(buf + dc_strlen(buf), "%s - %s [%d]\r\n", exits[door],
                 dc_->world[EXIT(ch, door)->to_room].name,
                 dc_->world[EXIT(ch, door)->to_room]->number_);
    else if (isSet(EXIT(ch, door)->exit_info, EX_CLOSED))
    {
      if (isSet(EXIT(ch, door)->exit_info, EX_HIDDEN))
        continue;
      else
        dc_sprintf(buf + dc_strlen(buf), "%s - (Closed)\r\n", exits[door]);
    }
    else if (IS_DARK(EXIT(ch, door)->to_room))
      dc_sprintf(buf + dc_strlen(buf), "%s - Too dark to tell\r\n", exits[door]);
    else
      dc_sprintf(buf + dc_strlen(buf), "%s leads to %s.\r\n", exits[door],
                 dc_->world[EXIT(ch, door)->to_room].name);
  }

  ch->sendln(u"You scan around the exits to see where they lead."_s);

  if (buf[0])
    ch->send(buf);
  else
    ch->sendln(u"None."_s);

  return ReturnValue::eSUCCESS;
}

QList<QChar> frills = {
    'o',
    '/',
    '~',
    '\\'};

ReturnValues do_score(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString race;
  QString buf, scratch;
  qint32 level = {};
  qint32 to_dam, to_hit, spell_dam;
  // qint32 flying = {};
  bool affect_found[AFF_MAX + 1] = {false};
  bool modifyOutput;

  affected_typePtr aff;

  qint64 exp_needed;
  quint32 immune = 0, suscept = 0, resist = {};
  QString isrString;
  // qint32 i;

  dc_sprintf(race, "%s", races[(qint32)ch->race].singular_name);
  exp_needed = (exp_table[(qint32)ch->getLevel() + 1] - ch->exp);

  to_hit = GET_REAL_HITROLL(ch);
  to_dam = GET_REAL_DAMROLL(ch);
  spell_dam = getRealSpellDamage(ch);

  dc_sprintf(buf, "$7($5:$7)================================================="
                  "========================($5:$7)\r\n"
                  "|=| %-30s  -- Character Attributes (DarkCastleMUD) |=|\r\n"
                  "($5:$7)=============================($5:$7)================="
                  "========================($5:$7)\r\n",
             qPrintable(ch->shortdesc_or_name()));

  ch->send(buf);

  dc_sprintf(buf, "|\\| $4Strength$7:        %4d  (%2d) |/| $1Race$7:  %-10s  $1HitPts$7:%5d$1/$7(%5d) |~|\r\n"
                  "|~| $4Dexterity$7:       %4d  (%2d) |o| $1Class$7: %-11s $1Mana$7:   %4d$1/$7(%5d) |\\|\r\n"
                  "|/| $4Constitution$7:    %4d  (%2d) |\\| $1Level$7:  %3llu        $1Fatigue$7:%4d$1/$7(%5d) |o|\r\n"
                  "|o| $4Intelligence$7:    %4d  (%2d) |~| $1Height$7: %3d        $1Ki$7:     %4d$1/$7(%5d) |/|\r\n"
                  "|\\| $4Wisdom$7:          %4d  (%2d) |/| $1Weight$7: %3d        $1Rdeaths$7:   %-5d     |~|\r\n"
                  "|~| $3Rgn$7: $4H$7:%3d $4M$7:%3d $4V$7:%3d $4K$7:%2d |o| $1Age$7:    %3d yrs    $1Align$7: %+5d         |\\|\r\n",
             GET_STR(ch), GET_RAW_STR(ch), race, ch->getHP(), GET_MAX_HIT(ch),
             GET_DEX(ch), GET_RAW_DEX(ch), pc_clss_types[(qint32)GET_CLASS(ch)], GET_MANA(ch), GET_MAX_MANA(ch),
             GET_CON(ch), GET_RAW_CON(ch), ch->getLevel(), GET_MOVE(ch), GET_MAX_MOVE(ch),
             GET_INT(ch), GET_RAW_INT(ch), GET_HEIGHT(ch), GET_KI(ch), GET_MAX_KI(ch),
             GET_WIS(ch), GET_RAW_WIS(ch), GET_WEIGHT(ch), ch->isNonPlayer() ? 0 : GET_RDEATHS(ch), ch->hit_gain_lookup(),
             ch->mana_gain_lookup(), ch->move_gain_lookup(), ch->ki_gain_lookup(), GET_AGE(ch),
             GET_ALIGNMENT(ch));
  ch->send(buf);

  if (ch->isPlayer()) // mobs can't view this part
  {
    QString experience_needed;
    if (ch->isImmortalPlayer())
    {
      experience_needed = "0";
    }
    else
    {
      experience_needed = u"%L1"_s.arg(exp_needed);
    }

    qint32 instrument_combat{}, instrument_non_combat = {};
    get_instrument_bonus(ch, instrument_combat, instrument_non_combat);
    ch->sendln(u"($5:$7)=============================($5:$7)===($5:$7)===================================($5:$7)\r\n"_s
                              "|/| $2Combat Statistics:$7                |\\| $2Equipment and Valuables:$7          |o|\r\n"
                              "|o|  $3Armor$7:   %1   $3Pkills$7:  %2  |~|  $3Items Carried$7:  %3 |/|\r\n"
                              "|\\|  $3BonusHit$7: %4   $3PDeaths$7: %5  |/|  $3Weight Carried$7: %6 |~|\r\n"
                              "|~|  $3BonusDam$7: %7   $3SplDam$7:  %8  |o|  $3Experience$7:   %L9 |\\|\r\n"
                              "|/|  $3InstrCom$7: %10   $3InstrNon$7:%11  |\\|  $3ExpTillLevel$7: %L12 |/|\r\n"
                              "|o|  $B$4FIRE$R[%13]  $B$3COLD$R[%14]  $B$5NRGY$R[%15]  |\\|  $3Gold$7: %L16 |o|\r\n"
                              "|\\|  $B$2ACID$R[%17]  $B$7MAGK$R[%18]  $2POIS$7[%19]  |~|  $3Bank$7: %L20 |/|\r\n"
                              "|-|  $3MELE$R[%21]  $3SPEL$R[%22]   $3KI$R [%23]  |/|  $3QPoints$7: %L24 $3Platinum$7: %L25 |-|\r\n"
                              "|/|                                   |o|                                   |/|\r\n"
                              "($5:$7)===================================($5:$7)===================================($5:$7)")
                   .arg(GET_ARMOR(ch), 5)
                   .arg(GET_PKILLS(ch), 5)
                   .arg(u"%1/%2"_s.arg(IS_CARRYING_N(ch)).arg(CAN_CARRY_N(ch)), 16)
                   .arg(u"%1%2"_s.arg(to_hit > 0 ? "+" : "").arg(to_hit), 4)
                   .arg(QString::number(GET_PDEATHS(ch)), 5)
                   .arg(u"%1/%2"_s.arg(IS_CARRYING_W(ch)).arg(CAN_CARRY_W(ch)), 16)
                   .arg(u"%1%2"_s.arg(to_dam > 0 ? "+" : "").arg(to_dam), 4)                                                   // +4d
                   .arg(u"%1%2"_s.arg(spell_dam > 0 ? "+" : "").arg(spell_dam), 5)                                             //+5d
                   .arg(ch->exp, 18)                                                                                           // -18s
                   .arg(u"%1%2"_s.arg(instrument_combat > 0 ? "+" : "").arg(instrument_combat), 4)                             //+4
                   .arg(u"%1%2"_s.arg(instrument_non_combat > 0 ? "+" : "").arg(instrument_non_combat), 5)                     //+5
                   .arg(experience_needed, 18)                                                                                 // -18s
                   .arg(u"%1%2"_s.arg(get_saves(ch, SAVE_TYPE_FIRE) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_FIRE)), 3)     //+3
                   .arg(u"%1%2"_s.arg(get_saves(ch, SAVE_TYPE_COLD) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_COLD)), 3)     //+3
                   .arg(u"%1%2"_s.arg(get_saves(ch, SAVE_TYPE_ENERGY) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_ENERGY)), 3) //+3
                   .arg(ch->getGold(), 26)
                   .arg(u"%1%2"_s.arg(get_saves(ch, SAVE_TYPE_ACID) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_ACID)), 3)     //+3
                   .arg(u"%1%2"_s.arg(get_saves(ch, SAVE_TYPE_MAGIC) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_MAGIC)), 3)   //+3
                   .arg(u"%1%2"_s.arg(get_saves(ch, SAVE_TYPE_POISON) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_POISON)), 3) //+3
                   .arg(GET_BANK(ch), 26)                                                                                      // -25s
                   .arg(u"%1%2"_s.arg(ch->melee_mitigation > 0 ? "+" : "").arg(ch->melee_mitigation), 3)                       // +3
                   .arg(u"%1%2"_s.arg(ch->spell_mitigation > 0 ? "+" : "").arg(ch->spell_mitigation), 3)                       // +3
                   .arg(u"%1%2"_s.arg(ch->song_mitigation > 0 ? "+" : "").arg(ch->song_mitigation), 3)                         // +3
                   .arg(GET_QPOINTS(ch), 5)                                                                                    // -25s
                   .arg(GET_PLATINUM(ch), 7));                                                                                 // -6d
  }
  else
    ch->sendln(u"($5:$7)===================================($5:$7)==================================($5:$7)"_s);
  qint32 found = false;

  if ((immune = ch->immune))
  {
    for (qint32 i = {}; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(immune, i);
      if (!isrString.isEmpty())
      {
        scratch = frills[level];
        dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
                   scratch, "Immunity", isrString.c_str(), scratch);
        ch->send(buf);
        found = true;
        isrString = QString();
        if (++level == 4)
          level = {};
      }
    }
  }
  if ((suscept = ch->suscept))
  {
    for (qint32 i = {}; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(suscept, i);
      if (!isrString.isEmpty())
      {
        scratch = frills[level];
        dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
                   scratch, "Susceptibility", isrString.c_str(), scratch);
        ch->send(buf);
        found = true;
        isrString = QString();
        if (++level == 4)
          level = {};
      }
    }
  }
  if ((resist = ch->resist))
  {
    for (qint32 i = {}; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(resist, i);
      if (!isrString.isEmpty())
      {
        scratch = frills[level];
        dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
                   scratch, "Resistibility", isrString.c_str(), scratch);
        ch->send(buf);
        found = true;
        isrString = QString();
        if (++level == 4)
          level = {};
      }
    }
  }

  if ((aff = ch->affected))
  {

    for (; aff; aff = aff->next)
    {
      if (aff->bitvector >= 0 && aff->bitvector <= AFF_MAX)
      {
        assert(aff->bitvector >= 0 && aff->bitvector <= AFF_MAX);
        affect_found[aff->bitvector] = true;
      }
      scratch = frills[level];
      modifyOutput = false;

      // figure out the name of the affect (if any)
      QString aff_name = get_skill_name(aff->type);
      //	 if (aff_name)
      //      if (*aff_name && !str_cmp(aff_name, "fly")) flying = 1;
      switch (aff->type)
      {
      case BASE_SETS + SET_RAGER:
        if (aff->location == 0)
        {
          aff_name = u"Battlerager's Fury"_s;
        }
        break;
      case BASE_SETS + SET_RAGER2:
        if (aff->location == 0)
        {
          aff_name = u"Battlerager's Fury"_s;
        }
        break;
      case BASE_SETS + SET_MOSS:
        if (aff->location == 0)
          aff_name = u"infravision"_s;
        break;
      case Character::PLAYER_CANTQUIT:
        aff_name = u"CANTQUIT"_s;
        break;
      case Character::PLAYER_OBJECT_THIEF:
        aff_name = u"DIRTY_THIEF/CANT_QUIT"_s;
        break;
      case Character::PLAYER_GOLD_THIEF:
        aff_name = u"GOLD_THIEF/CANT_QUIT"_s;
        break;
      case SKILL_HARM_TOUCH:
        aff_name = u"harmtouch reuse timer"_s;
        break;
      case SKILL_LAY_HANDS:
        aff_name = u"layhands reuse timer"_s;
        break;
      case SKILL_QUIVERING_PALM:
        aff_name = u"quiver reuse timer"_s;
        break;
      case SKILL_BLOOD_FURY:
        aff_name = u"blood fury reuse timer"_s;
        break;
      case SKILL_FEROCITY_TIMER:
        aff_name = u"ferocity reuse timer"_s;
        break;
      case SKILL_DECEIT_TIMER:
        aff_name = u"deceit reuse timer"_s;
        break;
      case SKILL_TACTICS_TIMER:
        aff_name = u"tactics reuse timer"_s;
        break;
      case SKILL_CLANAREA_CLAIM:
        aff_name = u"clanarea claim timer"_s;
        break;
      case SKILL_CLANAREA_CHALLENGE:
        aff_name = u"clanarea challenge timer"_s;
        break;
      case SKILL_CRAZED_ASSAULT:
        if (dc_strcmp(apply_types[(qint32)aff->location], "HITROLL"))
          aff_name = u"crazed assault reuse timer"_s;
        break;
      case SPELL_IMMUNITY:
        aff_name = u"immunity"_s;
        modifyOutput = true;
        break;
      case SKILL_NAT_SELECT:
        aff_name = u"natural selection"_s;
        modifyOutput = true;
        break;
      case SKILL_BREW_TIMER:
        aff_name = u"brew timer"_s;
        break;
      case SKILL_SCRIBE_TIMER:
        aff_name = u"scribe timer"_s;
        break;
      case CONC_LOSS_FIXER:
        aff_name = {}; // We don't want this showing up in score
        break;
      default:
        break;
      }
      if (aff_name.isEmpty()) // not one we want displayed
        continue;

      QString fading;
      if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
      {
        if (aff->duration < 3)
        {
          fading = "$2(fading)$7";
        }
      }

      QString modified = apply_types[(qint32)aff->location];
      if (modifyOutput)
      {
        if (ch->affected_by_spell(SKILL_NAT_SELECT))
        {
          modified = races[aff->modifier].singular_name;
        }
        else if (ch->affected_by_spell(SPELL_IMMUNITY))
        {
          modified = spells[aff->modifier];
        }
      }

      QString format;
      if (aff->type == Character::PLAYER_CANTQUIT)
      {
        format = "|%1| Affected by %2 from %3%4 Modifier %5 |%1|\r\n";
        format = format.arg(scratch);
        format = format.arg(QString::fromStdString("CANTQUIT"), 8);
        format = format.arg(QString::fromStdString(aff->caster), -12);
        format = format.arg(QString::fromStdString(fading), 8);
        format = format.arg(QString::fromStdString(modified), -15);
      }
      else
      {
        format = "|%1| Affected by %2 %3 Modifier %4 |%1|\r\n";
        format = format.arg(scratch);
        format = format.arg(aff_name, -25);
        format = format.arg(QString::fromStdString(fading), 8);
        format = format.arg(QString::fromStdString(modified), -15);
      }
      ch->send(format.toStdString());

      found = true;
      if (++level == 4)
        level = {};
    }
  }
  /*  if (flying == 0 && IS_AFFECTED(ch, AFF_FLYING)) {
      scratch = frills[level];
      dc_sprintf(buf, "|%c| Affected by fly                                Modifier NONE            |%c|\r\n",
              scratch, scratch);
      ch->send(buf);
      found = true;
      if(++level == 4)
        level = {};
    }*/
  extern bool elemental_score(CharacterPtr ch, qint32 level);
  if (!found)
    found = elemental_score(ch, level);
  else
    elemental_score(ch, level);

  if (found)
    ch->sendln(u"($5:$7)=========================================================================($5:$7)"_s);

  found = false;

  for (qint32 aff_idx = 1; aff_idx < (AFF_MAX + 1); aff_idx++)
  {
    if ((!affect_found[aff_idx]) && IS_AFFECTED(ch, aff_idx))
    {
      if (aff_idx == AFF_HIDE || aff_idx == AFF_GROUP)
        continue;

      found = true;
      if (++level == 4)
        level = {};
      scratch = frills[level];

      if (aff_idx != AFF_REFLECT)
        dc_sprintf(buf, "|%c| Affected by %-25s          Modifier NONE            |%c|\r\n",
                   scratch, affected_bits[aff_idx - 1], scratch);
      else
        dc_sprintf(buf, "|%c| Affected by %-25s          Modifier (%3d)           |%c|\r\n",
                   scratch, affected_bits[aff_idx - 1], ch->spell_reflect, scratch);
      ch->send(buf);
    }
  }

  if (found)
    ch->sendln(u"($5:$7)=========================================================================($5:$7)"_s);

  if (ch->isPlayer()) // mob can't view this part
  {
    if (ch->getLevel() > IMMORTAL && ch->player->buildLowVnum && ch->player->buildHighVnum)
    {
      if (ch->player->buildLowVnum == ch->player->buildOLowVnum &&
          ch->player->buildLowVnum == ch->player->buildMLowVnum)
      {
        dc_sprintf(buf, "CREATION RANGE: %d-%d\r\n", ch->player->buildLowVnum, ch->player->buildHighVnum);
        ch->send(buf);
      }
      else
      {
        dc_sprintf(buf, "ROOM RANGE: %d-%d\r\n", ch->player->buildLowVnum, ch->player->buildHighVnum);
        ch->send(buf);
        dc_sprintf(buf, "MOB RANGE: %d-%d\r\n", ch->player->buildMLowVnum, ch->player->buildMHighVnum);
        ch->send(buf);
        dc_sprintf(buf, "OBJ RANGE: %d-%d\r\n", ch->player->buildOLowVnum, ch->player->buildOHighVnum);
        ch->send(buf);
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_time(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  QString suf;
  qint32 weekday, day;
  time_t timep;
  qint32 h, m;
  // qint32 s;
  extern time_info_data time_info;
  extern QStringList weekdays;
  extern QStringList month_name;
  tm *pTime = {};

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  dc_sprintf(buf, "It is %d o'clock %s, on %s.\r\n",
             ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
             ((time_info.hours >= 12) ? "pm" : "am"),
             weekdays[weekday]);

  ch->send(buf);

  day = time_info.day + 1; /* day in [1..35] */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  dc_sprintf(buf, "The %d%s Day of the %s, Year %d.  (game time)\r\n",
             day,
             suf,
             month_name[time_info.month],
             time_info.year);

  ch->send(buf);

  // Changed to the below code without seconds in an attempt to stop
  // the timing of bingos... - pir 2/7/1999
  timep = time(0);
  if (ch->getLevel() > IMMORTAL)
  {
    dc_sprintf(buf, "The system time is %ld.\r\n", timep);

    ch->send(buf);
  }

  pTime = localtime(&timep);
  if (!pTime)
    return ReturnValue::eFAILURE;

  dc_sprintf(buf, "The system time is %d/%d/%d (%d:%02d) %s\r\n",
             pTime->tm_mon + 1,
             pTime->tm_mday,
             pTime->tm_year + 1900,
             pTime->tm_hour,
             pTime->tm_min,
             pTime->tm_zone);
  ch->send(buf);

  timep -= start_time;
  h = timep / 3600;
  m = (timep % 3600) / 60;
  // 	s = timep % 60;
  // 	dc_sprintf (buf, "The mud has been running for: %02li:%02li:%02li \r\n",
  // 			h,m,s);
  dc_sprintf(buf, "The mud has been running for: %02i:%02i \r\n", h, m);
  ch->send(buf);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_weather(CharacterPtr ch, QString argument, cmd_t cmd)
{
  extern weather_data weather_info;
  QString buf;

  if (GET_POS(ch) <= position_t::SLEEPING)
  {
    ch->sendln(u"You dream of being on a tropical island surrounded by beautiful members of the attractive sex."_s);
    return ReturnValue::eSUCCESS;
  }
  if (OUTSIDE(ch))
  {
    dc_sprintf(buf, "The sky is %s and %s.\r\n",
               sky_look[weather_info.sky],
               (weather_info.change >= 0 ? "you feel a warm wind from south" : "your foot tells you bad weather is due"));
    act_to_character(buf, ch, 0, 0, 0);
  }
  else
    ch->sendln(u"You have no feeling about the weather at all."_s);

  if (ch->isImmortalPlayer())
  {
    ch->send(u"Pressure: %4d  Change: %d (- = worse)\r\n"_s.arg(weather_info.pressure).arg(weather_info.change));
    ch->send(u"Sky: %9s  Sunlight: %d\r\n"_s.arg(sky_look[weather_info.sky]).arg(weather_info.sunlight));
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_help(CharacterPtr ch, QString argument, cmd_t cmd)
{
  extern QTextStream help_fl;
  extern QString help;

  qint32 chk, bot, top, mid;
  QString buf, buffer;

  if (!ch->conn_)
    return ReturnValue::eFAILURE;

  for (; isspace(*argument); argument++)
    ;

  if (!argument.isEmpty())
  {
    if (!dc_->help_index_)
    {
      ch->sendln(u"No help available."_s);
      return ReturnValue::eSUCCESS;
    }
    bot = {};
    top = dc_->top_of_helpt;

    for (;;)
    {
      mid = (bot + top) / 2;

      if (!(chk = str_cmp(argument, help_index[mid].keyword)))
      {
        fseek(help_fl, help_index[mid].pos, 0);
        *buffer = '\0';
        for (;;)
        {
          fgets(buf, 80, help_fl);
          if (*buf == '#')
            break;
          buf[80] = {};
          if ((dc_strlen(buffer) + dc_strlen(buf)) >= MAX_STRING_LENGTH)
            break;
          dc_strcat(buffer, buf);
          dc_strcat(buffer, "\r");
        }
        page_string(ch->conn_, buffer, 1);
        return ReturnValue::eSUCCESS;
      }
      else if (bot >= top)
      {
        ch->sendln(u"There is no help on that word."_s);
        return 1;
      }
      else if (chk > 0)
        bot = ++mid;
      else
        top = --mid;
    }
  }

  ch->send(help);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_count(CharacterPtr ch, QString arg, cmd_t cmd)
{
  ConnectionPtr conn;
  CharacterPtr i;
  qint32 clss[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  qint32 race[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  qint32 immortal = {};
  qint32 total = {};

  for (auto &d : dc_->connections_)
  {
    if (conn->connected || !conn->character)
      continue;
    if (!(i = conn->original))
      i = conn->character;
    if (!CAN_SEE(ch, i))
      continue;
    if (i->getLevel() > MORTAL)
    {
      immortal++;
      total++;
      continue;
    }
    clss[(qint32)GET_CLASS(i)]++;
    race[(qint32)i->race]++;
    total++;
  }

  ch->send(u"There are %d visible players connected, %d of which are immortals.\r\n"_s.arg(total).arg(immortal));
  ch->send(u"%d warriors, %d clerics, %d mages, %d thieves, %d barbarians, %d monks,\r\n"_s.arg(clss[CLASS_WARRIOR], clss[CLASS_CLERIC], clss[CLASS_MAGIC_USER]).arg(clss[CLASS_THIEF]).arg(clss[CLASS_BARBARIAN]).arg(clss[CLASS_MONK]));
  ch->send(u"%d paladins, %d antipaladins, %d bards, %d druids, and %d rangers.\r\n"_s.arg(clss[CLASS_PALADIN]).arg(clss[CLASS_ANTI_PAL]).arg(clss[CLASS_BARD]).arg(clss[CLASS_DRUID]).arg(clss[CLASS_RANGER]));
  ch->send(u"%d humans, %d elves, %d dwarves, %d hobbits, %d pixies,\r\n"_s.arg(race[RACE_HUMAN], race[RACE_ELVEN]).arg(race[RACE_DWARVEN]).arg(race[RACE_HOBBIT]).arg(race[RACE_PIXIE]));
  ch->send(u"%d ogres, %d gnomes, %d orcs, %d trolls.\r\n"_s.arg(race[RACE_GIANT]).arg(race[RACE_GNOME]).arg(race[RACE_ORC]).arg(race[RACE_TROLL]));
  ch->send(u"The maximum number of players since last reboot was %d."_s.arg(max_who));
  return ReturnValue::eSUCCESS;
}

ReturnValues do_inventory(CharacterPtr ch, QString argument, cmd_t cmd)
{
  ch->sendln(u"You are carrying:"_s);
  ch->list_obj_to_char(ch->carrying, 1, true);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_equipment(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 j;
  bool found;

  ch->sendln(u"You are using:"_s);
  found = false;
  for (j = {}; j < MAX_WEAR; j++)
  {
    if (ch->equipment[j])
    {
      if (CAN_SEE_OBJ(ch, ch->equipment[j]))
      {
        if (!found)
        {
          act_to_character("<    worn     > Item Description     (Flags) [Item Condition]\r\n", ch, 0, 0, 0);
          found = true;
        }
        send_to_char(where[j], ch);
        ch->show_obj_to_char(ch->equipment[j], 1);
      }
      else
      {
        send_to_char(where[j], ch);
        ch->sendln(u"something"_s);
        found = true;
      }
    }
  }
  if (!found)
  {
    ch->sendln(u"Nothing."_s);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_credits(CharacterPtr ch, QString argument, cmd_t cmd)
{
  page_string(ch->conn_, credits, 0);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_story(CharacterPtr ch, QString argument, cmd_t cmd)
{
  page_string(ch->conn_, story, 0);
  return ReturnValue::eSUCCESS;
}
/*
ReturnValues do_news(CharacterPtr ch, QString argument, cmd_t cmd)
{
   page_string(ch->conn_, news, 0);
   return ReturnValue::eSUCCESS;
}

*/
ReturnValues do_info(CharacterPtr ch, QString argument, cmd_t cmd)
{
  page_string(ch->conn_, info, 0);
  return ReturnValue::eSUCCESS;
}

/*********------------ locate objects -----------------***************/
ReturnValues do_olocate(CharacterPtr ch, QString name, cmd_t cmd)
{
  QString buf, buf2;
  ObjectPtr k;
  qint32 in_room = 0, count = {};
  qint32 vnum = {};
  qint32 searchnum = {};

  buf2[0] = '\0';
  if (isdigit(*name))
  {
    vnum = dc_atoi(name);
    searchnum = real_object(vnum);
  }

  ch->sendln(u"-#-- Short Description ------- Room Number\n"_s);

  for (k = dc_->object_list; k; k = k->next)
  {

    // allow search by vnum
    if (vnum)
    {
      if (k->item_number != searchnum)
        continue;
    }
    else if (!(isexact(name, k->name())))
      continue;

    if (!CAN_SEE_OBJ(ch, k))
      continue;

    buf[0] = '\0';

    if (k->in_obj)
    {
      if (k->in_obj->in_room > INVALID_ROOM)
        in_room = dc_->world[k->in_obj->in_room]->number_;
      else if (k->in_obj->carried_by)
      {
        if (!CAN_SEE(ch, k->in_obj->carried_by))
          continue;
        in_room = dc_->world[k->in_obj->carried_by->in_room]->number_;
      }
      else if (k->in_obj->equipped_by)
      {
        if (!CAN_SEE(ch, k->in_obj->equipped_by))
          continue;
        in_room = dc_->world[k->in_obj->equipped_by->in_room]->number_;
      }
    }
    else if (k->carried_by)
    {
      if (!CAN_SEE(ch, k->carried_by))
        continue;
      in_room = dc_->world[k->carried_by->in_room]->number_;
    }
    else if (k->equipped_by)
    {
      if (!CAN_SEE(ch, k->equipped_by))
        continue;
      in_room = dc_->world[k->equipped_by->in_room]->number_;
    }
    else if (k->in_room > 0)
      in_room = dc_->world[k->in_room]->number_;
    else
      in_room = {};

    count++;

    if (in_room != INVALID_ROOM)
      dc_sprintf(buf, "[%2d] %-26s %d", count, qPrintable(k->short_description()), in_room);
    else
      dc_sprintf(buf, "[%2d] %-26s %s", count, qPrintable(k->short_description()), "(Item at INVALID_ROOM.)");

    if (k->in_obj)
    {
      dc_strcat(buf, " in ");
      dc_strcat(buf, qPrintable(k->in_obj->short_description()));
      if (k->in_obj->carried_by)
      {
        dc_strcat(buf, " carried by ");
        dc_strcat(buf, qPrintable(k->in_obj->carried_by->name()));
      }
      else if (k->in_obj->equipped_by)
      {
        dc_strcat(buf, " equipped by ");
        dc_strcat(buf, qPrintable(k->in_obj->equipped_by->name()));
      }
    }
    if (k->carried_by)
    {
      dc_strcat(buf, " carried by ");
      dc_strcat(buf, qPrintable(k->carried_by->name()));
    }
    else if (k->equipped_by)
    {
      dc_strcat(buf, " equipped by ");
      dc_strcat(buf, qPrintable(k->equipped_by->name()));
    }
    if (dc_strlen(buf2) + dc_strlen(buf) + 3 >= MAX_STRING_LENGTH)
    {
      ch->sendln(u"LIST TRUNCATED...TOO LONG"_s);
      break;
    }
    dc_strcat(buf2, buf);
    dc_strcat(buf2, "\r\n");
  }

  if (buf.isEmpty() 2)
    ch->sendln(u"Couldn't find any such OBJECT."_s);
  else
    page_string(ch->conn_, buf2, 1);
  return ReturnValue::eSUCCESS;
}
/*********--------- end of locate objects -----------------************/

/* -----------------   MOB LOCATE FUNCTION ---------------------------- */
// locates ONLY mobiles.  If cmd == 18, it locates pc's AND mobiles
ReturnValues do_mlocate(CharacterPtr ch, QString name, cmd_t cmd)
{
  QString buf, buf2;
  qint32 count = {};
  qint32 vnum = {};
  qint32 searchnum = {};

  if (isdigit(*name))
  {
    vnum = dc_atoi(name);
    searchnum = real_mobile(vnum);
  }

  *buf2 = '\0';
  ch->sendln(u" #   Short description          Room Number\n"_s);

  const auto &character_list = dc_->character_list;
  for (const auto &i : character_list)
  {

    if ((i->isPlayer() &&
         (cmd != cmd_t::MLOCATE_CHARACTER || !CAN_SEE(ch, i))))
      continue;

    // allow find by vnum
    if (vnum)
    {
      if (searchnum != i->mobdata->nr)
        continue;
    }
    else if (!(isexact(name, i->name())))
      continue;

    if (i->in_room == INVALID_ROOM)
    {
      continue;
    }

    count++;

    dc_sprintf(buf, "[%2d] %-26s %d\r\n", count, qPrintable(i->short_description()), dc_->world[i->in_room]->number_);
    if (dc_strlen(buf) + dc_strlen(buf2) + 3 >= MAX_STRING_LENGTH)
    {
      ch->sendln(u"LIST TRUNCATED...TOO LONG"_s);
      break;
    }
    dc_strcat(buf2, buf);
  }

  if (buf.isEmpty() 2)
    ch->sendln(u"Couldn't find any MOBS by that NAME."_s);
  else
    page_string(ch->conn_, buf2, 1);
  return ReturnValue::eSUCCESS;
}
/* --------------------- End of Mob locate function -------------------- */

ReturnValues do_consider(CharacterPtr ch, QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString name;
  qint32 mod = {};
  qint32 percent, x, y;
  qint32 Learned;

  const QStringList level_messages = {
      "You can kill %s naked and weaponless.\r\n",
      "%s is no match for you.\r\n",
      "%s looks like an easy kill.\r\n",
      "%s wouldn't be all that hard.\r\n",
      "%s is perfect for you!\r\n",
      "You would need some luck and good equipment to kill %s.\r\n",
      "%s says 'Do you feel lucky, punk?'.\r\n",
      "%s laughs at you mercilessly.\r\n",
      "%s will tear your head off and piss on your dead skull.\r\n"};

  const QStringList ac_messages = {
      "looks impenetrable.",
      "is heavily armored.",
      "is very well armored.",
      "looks quite durable.",
      "looks pretty durable.",
      "is well protected.",
      "is protected.",
      "is pretty well protected.",
      "is slightly protected.",
      "is enticingly dressed.",
      "is pretty much naked."};

  const QStringList hplow_messages = {
      "wouldn't be worth your time",
      "wouldn't stand a snowball's chance in hell against you",
      "definitely wouldn't last too long against you",
      "probably wouldn't last too long against you",
      "can handle almost half the damage you can",
      "can take half the damage you can",
      "can handle just over half the damage you can",
      "can take two-thirds the damage you can",
      "can handle almost as much damage as you",
      "can handle nearly as much damage as you",
      "can handle just as much damage as you"};

  const QStringList hphigh_messages = {
      "can definitely take anything you can dish out",
      "can probably take anything you can dish out",
      "takes a licking and keeps on ticking",
      "can take some punishment",
      "can handle more than twice the damage that you can",
      "can handle twice as much damage as you",
      "can handle quite a bit more damage than you",
      "can handle a lot more damage than you",
      "can handle more damage than you",
      "can handle a bit more damage than you",
      "can handle just as much damage as you"};

  const QStringList dam_messages = {
      "hits like my grandmother",
      "will probably graze you pretty good",
      "can hit pretty hard",
      "can pack a pretty damn good punch",
      "can massacre on a good day",
      "can massacre even on a bad day",
      "could make Darth Vader cry like a baby",
      "can eat the Terminator for lunch and not have to burp",
      "will beat the living shit out of you",
      "will pound the fuck out of you",
      "could make Sylvester Stallone cry for his mommy",
      "is a *very* tough mobile.  Be careful"};

  const QStringList thief_messages = {
      "At least they'll hang you quickly.",
      "Bards will sing of your bravery, rogues will snicker at\r\nyour stupidity.",
      "Don't plan on sending your kids to college.",
      "I'd bet against you.",
      "The odds aren't quite in your favor.",
      "I'd give you about 50-50.",
      "The odds are slightly in your favor.",
      "I'd place my money on you. (Not ALL of my money.)",
      "Pretty damn good...80-90 percent.",
      "If you fail THIS steal, you're a loser.",
      "You can't miss."};

  one_argument(argument, name);

  if (!(victim = ch->get_char_room_vis(name)))
  {
    ch->sendln(u"Who was that you're scoping out?"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln(u"Looks like a WIMP! (Used to be \"Looks like a PUSSY!\" but we got complaints.)"_s);
    return ReturnValue::eFAILURE;
  }

  if (!ch->decrementMove(5, "You are too tired to consider much of anything at the moment."))
  {
    return ReturnValue::eFAILURE;
  }

  if (!skill_success(ch, nullptr, SKILL_CONSIDER))
  {
    ch->sendln(u"You try really hard, but you really have no idea about their capabilties!"_s);
    return ReturnValue::eFAILURE;
  }

  Learned = ch->has_skill(SKILL_CONSIDER);

  if (Learned > 20)
  {
    /* ARMOR CLASS */
    x = GET_ARMOR(victim) / 20 + 5;
    if (x > 10)
      x = 10;
    if (x < 0)
      x = {};

    percent = dc_->number(1, 101);
    if (percent > Learned)
    {
      if (ch->dc_->number(0, 1) == 0)
      {
        x -= dc_->number(1, 3);
        if (x < 0)
          x = {};
      }
      else
      {
        x += dc_->number(1, 3);
        if (x > 10)
          x = 10;
      }
    }

    ch->send(u"As far as armor goes, %s %s\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())).arg(ac_messages[x]));
  }

  /* HIT POINTS */

  if (Learned > 40)
  {

    if (victim->isPlayer() && victim->getLevel() > IMMORTAL)
    {
      ch->send(u"Compared to your hps, %s can definitely take anything you can dish out.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())));
    }
    else
    {

      if (ch->getHP() >= victim->getHP() || ch->getLevel() > MORTAL)
      {
        x = victim->getHP() / ch->getHP() * 100;
        x /= 10;
        if (x < 0)
          x = {};
        if (x > 10)
          x = 10;
        percent = dc_->number(1, 101);
        if (percent > Learned)
        {
          if (ch->dc_->number(0, 1) == 0)
          {
            x -= dc_->number(1, 3);
            if (x < 0)
              x = {};
          }
          else
          {
            x += dc_->number(1, 3);
            if (x > 10)
              x = 10;
          }
        }

        ch->send(u"Compared to your hps, %s %s.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())).arg(hplow_messages[x]));
      }
      else
      {
        x = ch->getHP() / victim->getHP() * 100;
        x /= 10;
        if (x < 0)
          x = {};
        if (x > 10)
          x = 10;
        percent = dc_->number(1, 101);
        if (percent > Learned)
        {
          if (ch->dc_->number(0, 1) == 0)
          {
            x -= dc_->number(1, 3);
            if (x < 0)
              x = {};
          }
          else
          {
            x += dc_->number(1, 3);
            if (x > 10)
              x = 10;
          }
        }

        ch->send(u"Compared to your hps, %s %s.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())).arg(hphigh_messages[x]));
      }
    }

    if (Learned > 60)
    {

      /* Average Damage */

      if (victim->equipment[WEAR_WIELD])
      {
        x = victim->equipment[WEAR_WIELD]->flags_.value[1];
        y = victim->equipment[WEAR_WIELD]->flags_.value[2];
        x = (((x * y - x) / 2) + x);
      }
      else
      {
        if (victim->isNonPlayer())
        {
          x = victim->mobdata->damnodice;
          y = victim->mobdata->damsizedice;
          x = (((x * y - x) / 2) + x);
        }
        else
          x = dc_->number(0, 2);
      }
      x += GET_DAMROLL(victim);

      if (x <= 5)
        x = {};
      else if (x <= 10)
        x = 1;
      else if (x <= 15)
        x = 2;
      else if (x <= 23)
        x = 3;
      else if (x <= 30)
        x = 4;
      else if (x <= 40)
        x = 5;
      else if (x <= 50)
        x = 6;
      else if (x <= 75)
        x = 7;
      else if (x <= 100)
        x = 8;
      else if (x <= 125)
        x = 9;
      else if (x <= 150)
        x = 10;
      else
        x = 11;
      percent = dc_->number(1, 101);
      if (percent > Learned)
      {
        if (ch->dc_->number(0, 1) == 0)
        {
          x -= dc_->number(1, 4);
          if (x < 0)
            x = {};
        }
        else
        {
          x += dc_->number(1, 4);
          if (x > 11)
            x = 11;
        }
      }

      ch->send(u"Average damage: %s %s.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())).arg(dam_messages[x]));
    }
  }

  if (Learned > 80)
  {
    /* CHANCES TO STEAL */
    if ((GET_CLASS(ch) == CLASS_THIEF) || (ch->getLevel() > IMMORTAL))
    {

      percent = Learned;

      mod += AWAKE(victim) ? 10 : -50;
      level_diff_t level_difference = victim->getLevel() - ch->getLevel();

      mod += level_difference / 2;
      mod += 5; /* average item is 5 lbs, steal takes ths into acct */
      if (GET_DEX(ch) < 10)
        mod += ((10 - GET_DEX(ch)) * 5);
      else if (GET_DEX(ch) > 15)
        mod -= ((GET_DEX(ch) - 10) * 2);

      percent -= mod;

      if (GET_POS(victim) <= position_t::SLEEPING)
        percent = 100;
      if (victim->getLevel() > IMMORTAL)
        percent = {};
      if (percent < 0)
        percent = {};
      else if (percent > 100)
        percent = 100;
      percent /= 10;
      x = percent;

      percent = dc_->number(1, 101);
      if (percent > Learned)
      {
        if (ch->dc_->number(0, 1) == 0)
        {
          x -= dc_->number(1, 3);
          if (x < 0)
            x = {};
        }
        else
        {
          x += dc_->number(1, 3);
          if (x > 10)
            x = 10;
        }
      }

      ch->send(u"Chances of stealing: %s\r\n"_s.arg(thief_messages[x]));
    }
  }
  /* Level Comparison */
  level_diff_t level_difference = victim->getLevel() - ch->getLevel();
  if (level_difference <= -15)
    y = {};
  else if (level_difference <= -10)
    y = 1;
  else if (level_difference <= -5)
    y = 2;
  else if (level_difference <= -2)
    y = 3;
  else if (level_difference <= 1)
    y = 4;
  else if (level_difference <= 2)
    y = 5;
  else if (level_difference <= 4)
    y = 6;
  else if (level_difference <= 9)
    y = 7;
  else
    y = 8;

  ch->send(u"Level comparison: "_s);
  ch->send(level_messages[y]).arg(qPrintable(victim->shortdesc_or_name()));

  if (Learned > 89)
  {
    ch->send(u"Training: "_s);

    if (GET_CLASS(victim) == CLASS_WARRIOR ||
        GET_CLASS(victim) == CLASS_THIEF ||
        GET_CLASS(victim) == CLASS_BARBARIAN ||
        GET_CLASS(victim) == CLASS_MONK ||
        GET_CLASS(victim) == CLASS_BARD)
      ch->send(u"%s appears to be a trained fighter.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())));
    else if (GET_CLASS(victim) == CLASS_MAGIC_USER ||
             GET_CLASS(victim) == CLASS_CLERIC ||
             GET_CLASS(victim) == CLASS_DRUID ||
             GET_CLASS(victim) == CLASS_PSIONIC ||
             GET_CLASS(victim) == CLASS_NECROMANCER)
      ch->send(u"%s appears to be trained in mystical arts.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())));
    else if (GET_CLASS(victim) == CLASS_ANTI_PAL ||
             GET_CLASS(victim) == CLASS_PALADIN ||
             GET_CLASS(victim) == CLASS_RANGER)
      ch->send(u"%s appears to have training in both combat and magic.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())));
    else if (GET_CLASS(victim))
      ch->send(u"%s appears to have training, but you are unfamiliar with what.\r\n"_s.arg(qPrintable(victim->shortdesc_or_name())));
    else
      ch->sendln(u"You've seen stray dogs that were better trained."_s);
  }

  return ReturnValue::eSUCCESS;
}

/* Shows characters in adjacent rooms -- Sadus */
ReturnValues do_scan(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 i;
  CharacterPtr vict;
  Room *room;
  qint32 was_in;

  const QStringList possibilities =
      {
          "to the North",
          "to the East",
          "to the South",
          "to the West",
          "above you",
          "below you",
          "\n",
      };

  if (!ch->decrementMove(2, "You are to tired to scan right now."))
  {
    return ReturnValue::eFAILURE;
  }

  act_to_room(u"$n carefully searches the surroundings..."_s, 0, 0, INVIS_NULL | STAYHIDE);
  ch->sendln(u"You carefully search the surroundings...\r\n"_s);

  for (vict = dc_->world[ch->in_room]->people_; vict; vict = vict->next_in_room)
  {
    if (CAN_SEE(ch, vict) && ch != vict)
    {
      ch->send(u"%35s -- %s\r\n"_s.arg(qPrintable(vict->shortdesc_or_name())).arg("Right Here"));
    }
  }

  for (i = {}; i < 6; i++)
  {
    if (CAN_GO(ch, i))
    {
      room = &dc_->world[dc_->world[ch->in_room].dir_option[i]->to_room];
      if (room == &dc_->world[ch->in_room])
        continue;
      if (isSet(room->room_flags_, NO_SCAN))
      {
        ch->send(u"%35s -- a little bit %s\r\n", "It's too hard to see!"_s.arg(possibilities[i]));
      }
      else
        for (vict = room->people; vict; vict = vict->next_in_room)
        {
          if (CAN_SEE(ch, vict))
          {
            if (IS_AFFECTED(vict, AFF_CAMOUFLAGUE) &&
                dc_->world[vict->in_room].sector_type != SECT_INSIDE &&
                dc_->world[vict->in_room].sector_type != SECT_CITY &&
                dc_->world[vict->in_room].sector_type != SECT_AIR)
              continue;

            if (skill_success(ch, nullptr, SKILL_SCAN))
            {
              ch->send(u"%35s -- a little bit %s\r\n"_s.arg(qPrintable(vict->shortdesc_or_name())).arg(possibilities[i]));
            }
          }
        }

      // Now we go one room further (reach out and touch someone)

      was_in = ch->in_room;
      ch->in_room = dc_->world[ch->in_room].dir_option[i]->to_room;

      if (CAN_GO(ch, i))
      {
        room = &dc_->world[dc_->world[ch->in_room].dir_option[i]->to_room];
        if (isSet(room->room_flags_, NO_SCAN))
        {
          ch->send(u"%35s -- a ways off %s\r\n", "It's too hard to see!"_s.arg(possibilities[i]));
        }
        else
          for (vict = room->people; vict; vict = vict->next_in_room)
          {
            if (CAN_SEE(ch, vict))
            {
              if (IS_AFFECTED(vict, AFF_CAMOUFLAGUE) &&
                  dc_->world[vict->in_room].sector_type != SECT_INSIDE &&
                  dc_->world[vict->in_room].sector_type != SECT_CITY &&
                  dc_->world[vict->in_room].sector_type != SECT_AIR)
                continue;

              if (skill_success(ch, nullptr, SKILL_SCAN, -10))
              {
                ch->sendln(u"%35s -- a ways off %s"_s.arg(qPrintable(vict->shortdesc_or_name())).arg(possibilities[i]));
              }
            }
          }
        // Now if we have the farsight spell we go another room out
        if (IS_AFFECTED(ch, AFF_FARSIGHT))
        {
          ch->in_room = dc_->world[ch->in_room].dir_option[i]->to_room;
          if (CAN_GO(ch, i))
          {
            room = &dc_->world[dc_->world[ch->in_room].dir_option[i]->to_room];
            if (isSet(room->room_flags_, NO_SCAN))
            {
              ch->send(u"%35s -- extremely far off %s\r\n", "It's too hard to see!"_s.arg(possibilities[i]));
            }
            else
              for (vict = room->people; vict; vict = vict->next_in_room)
              {
                if (CAN_SEE(ch, vict))
                {
                  if (IS_AFFECTED(vict, AFF_CAMOUFLAGUE) &&
                      dc_->world[vict->in_room].sector_type != SECT_INSIDE &&
                      dc_->world[vict->in_room].sector_type != SECT_CITY &&
                      dc_->world[vict->in_room].sector_type != SECT_AIR)
                    continue;

                  if (skill_success(ch, nullptr, SKILL_SCAN, -20))
                  {
                    ch->sendln(u"%35s -- extremely far off %s"_s.arg(qPrintable(vict->shortdesc_or_name())).arg(possibilities[i]));
                  }
                }
              }
          }
        }
      }
      ch->in_room = was_in;
    }
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues do_tick(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 ntick;
  QString buf;

  if (isSet(dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return 1;
  }

  if (ch->isNonPlayer())
  {
    ch->sendln(u"Monsters don't wait for anything."_s);
    return ReturnValue::eFAILURE;
  }

  if (ch->conn_ == nullptr)
    return ReturnValue::eFAILURE;

  while (*argument == ' ')
    argument++;

  if (*argument == '\0')
    ntick = 1;
  else
    ntick = dc_atoi(argument);

  if (ntick == 1)
    dc_sprintf(buf, "$n is waiting for one tick.");
  else
    dc_sprintf(buf, "$n is waiting for %d ticks.", ntick);

  act_to_character(buf, ch, 0, 0, INVIS_NULL);
  act_to_room(buf, ch, 0, 0, INVIS_NULL);

  // TODO - figure out if this ever had any purpose.  It's still fun though:)
  ch->conn_->tick_wait = ntick;
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_experience(QStringList arguments, cmd_t cmd)
{
  if (level_ >= IMMORTAL)
  {
    send(u"Immortals cannot gain levels by gaining experience.\r\n"_s);
    return ReturnValue::eSUCCESS;
  }

  level_ next_level = level_;
  qint64 experience_remaining = {};

  do
  {
    next_level += 1;
    quint64 experience_next_level = exp_table[next_level];
    quint64 current_experience = exp;
    experience_remaining = experience_next_level - current_experience;

    if (experience_remaining < 0)
    {
      send(u"You have enough experience to advance to level %L1.\r\n"_s.arg(next_level));
    }
    else
    {
      send(u"You require %L1 experience to advance to level %L2.\r\n"_s.arg(experience_remaining).arg(next_level));
    }
  } while (experience_remaining < 0);

  return ReturnValue::eSUCCESS;
}

void check_champion_and_website_who_list()
{
  ObjectPtr obj;
  std::stringstream buf, buf2;
  qint32 addminute = {};
  QString name;

  const auto &character_list = dc_->character_list;
  for (const auto &ch : character_list)
  {

    if (ch->isPlayer() && ch->conn_ && ch->player && ch->player->wizinvis <= 0)
    {
      buf << qPrintable(ch->shortdesc_or_name()) << std::endl;
    }

    if ((ch->isNonPlayer() || !ch->conn_) && (obj = get_obj_in_list_num(real_object(CHAMPION_ITEM), ch->carrying)))
    {
      obj_from_char(obj);
      obj_to_room(obj, CFLAG_HOME);
    }

    if (IS_AFFECTED(ch, AFF_CHAMPION) && !(obj = get_obj_in_list_num(real_object(CHAMPION_ITEM), ch->carrying)))
    {
      REMBIT(ch->affected_by, AFF_CHAMPION);
    }
  }

  buf << "endminutenobodywillhavethisnameever" << std::endl;
  addminute++;

  if (!(obj = get_obj_num(real_object(CHAMPION_ITEM))))
  {
    if ((obj = clone_object(real_object(CHAMPION_ITEM))))
    {
      obj_to_room(obj, CFLAG_HOME);
    }
    else
    {
      dc_->logentry(u"CHAMPION_ITEM obj not found. Please create one."_s, 0, DC::LogChannel::LOG_MISC);
    }
  }

  std::ifstream stream(LOCAL_WHO_FILE);

  while (getline(stream, name))
  {
    if (addminute <= 9)
      buf << name << std::endl;
    else
      buf2 << name << std::endl;
    if (name == "endminutenobodywillhavethisnameever")
      addminute++;
  }

  stream.close();

  std::ofstream flo(LOCAL_WHO_FILE);
  flo << buf.str();
  flo.close();

  std::ofstream flwo(WEB_WHO_FILE);
  flwo << buf2.str();
  flwo.close();
}

ReturnValues do_sector(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString art = "a";

  if (ch->conn_ && ch->in_room)
  {
    qint32 sector = dc_->world[ch->in_room].sector_type;
    switch (sector)
    {
    case SECT_INSIDE:
    case SECT_UNDERWATER:
    case SECT_AIR:
    case SECT_ARCTIC:
      art = "an";
      break;
    }

    ch->send(u"You are currently in %s %s area.\r\n"_s.arg(art.c_str()).arg(sector_types[sector]));
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues do_version(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (ch)
  {
    ch->sendln(u"Version: %1 Build time: %2"_s.arg(DC::getBuildVersion()).arg(DC::getBuildTime()));
  }
  return ReturnValue::eSUCCESS;
}

class Search
{
public:
  enum types
  {
    O_NAME, // name=
    O_DESCRIPTION,
    O_SHORT_DESCRIPTION,
    O_ACTION_DESCRIPTION,
    O_TYPE_FLAG,   // type=
    O_WEAR_FLAGS,  // wear=
    O_SIZE,        // size=
    O_EXTRA_FLAGS, // extra=
    O_WEIGHT,
    O_COST,
    O_MORE_FLAGS, // more=
    O_EQ_LEVEL,   // level
    O_V1,         // v1
    O_V2,         // v2
    O_V3,         // v3
    O_V4,         // v4
    O_AFFECTED,
    O_EDD_KEYWORD,
    O_EDD_DESCRIPTION,
    O_CARRIED_BY,
    O_EQUIPPED_BY,
    LIMIT
  };
  enum locations
  {
    in_inventory,
    in_equipment,
    in_room,
    in_vault,
    in_clan_vault,
    in_object_database
  };
  bool operator==(const ObjectPtr obj);
  void setType(types type) { type_ = type; }
  types getType(void) const { return type_; }

  // name=
  void setObjectName(QString name) { o_name_ = name; }

  // description
  // short description
  // action description

  // type=
  void setObjectType(object_type_t object_type) { obj_flags_.type_flag = object_type; }

  void setObjectWearLocation(ObjectPositions location) { obj_flags_.wear_flags = location; }

  // size=
  void setObjectSize(quint16 size) { obj_flags_.size = size; }

  // extra=
  void setObjectExtra(quint32 flags) { obj_flags_.extra_flags = flags; }

  // weight
  void setObjectMinimumWeight(quint64 weight) { o_min_weight_ = weight; }
  void setObjectMaximumWeight(quint64 weight) { o_max_weight_ = weight; }

  // cost
  void setObjectMinimumCost(quint64 cost) { o_min_cost_ = cost; }
  void setObjectMaximumCost(quint64 cost) { o_max_cost_ = cost; }

  // more flags
  void setObjectMore(quint32 flags) { obj_flags_.more_flags = flags; }

  // level=
  void setObjectMinimumLevel(quint64 level) { o_min_level_ = level; }
  void setObjectMaximumLevel(quint64 level) { o_max_level_ = level; }

  // v1
  // v2
  // v3
  // v4
  // affected

  // edd keyword
  void setObjectEDDKeyword(QString keyword) { o_edd_keyword_ = keyword; }

  // edd description

  // carried by
  void setObjectCarriedBy(QString name) { o_carried_by_ = name; }

  // equired by
  void setObjectEquippedBy(QString name) { o_equipped_by_ = name; }

  // limit=
  void setLimitOutput(quint64 limit_output) { limit_output_ = limit_output; }
  quint64 getLimitOutput(void) const { return limit_output_; }

  void enableShowRange(void) { show_range_ = true; }
  bool isShowRange(void) const { return show_range_; }

private:
  types type_ = {};

  quint64 o_min_level_ = {};
  quint64 o_max_level_ = {};
  quint64 o_min_cost_ = {};
  quint64 o_max_cost_ = {};
  quint64 o_min_weight_ = {};
  quint64 o_max_weight_ = {};

  quint64 o_value[4] = {};

  quint64 o_item_number_ = {}; /* Where in data-base               */
  quint64 o_in_room_ = {};     /* In what room -1 when conta/carr  */
  quint64 o_vroo_ = {};        /* for corpse saving */
  ObjectFlags obj_flags_ = {}; /* Object information               */
  qint16 o_num_affects_ = {};
  obj_affected_type o_affected_ = {}; /* Which abilities in PC to change  */

  QString o_name_ = {};               /* Title of object :get etc.        */
  QString o_description_ = {};        /* When in room                     */
  QString o_short_description_ = {};  /* when worn/carry/in cont.         */
  QString o_action_description_ = {}; /* What to write when used          */

  QString o_edd_keyword_ = {};     /* Keyword in look/examine          */
  QString o_edd_description_ = {}; /* What to see                      */
  QString o_carried_by_ = {};
  QString o_equipped_by_ = {};
  quint64 limit_output_ = {};
  bool show_range_ = false;
};

class Result
{
  Search::locations location_ = {};
  ObjectPtr object_ = {};
  QString name_;

public:
  Result(Search::locations location, ObjectPtr object)
      : location_(location), object_(object)
  {
  }

  Result(Search::locations location, ObjectPtr object, QString name)
      : location_(location), object_(object), name_(name)
  {
  }

  Search::locations getLocation() const { return location_; }
  ObjectPtr getObject() const { return object_; }
  QString getName() const { return name_; }
};

bool Search::operator==(const ObjectPtr obj)
{
  if (obj == nullptr)
  {
    return false;
  }

  if (show_range_)
  {
    return true;
  }

  switch (type_)
  {
  case O_NAME:
    if (o_name_ == obj->name() || isexact(o_name_, obj->name()))
    {
      return true;
    }
    break;

  case O_DESCRIPTION:
    break;

  case O_SHORT_DESCRIPTION:
    break;

  case O_ACTION_DESCRIPTION:
    break;

  case O_TYPE_FLAG:
    if (obj->flags_.type_flag == obj_flags_.type_flag)
    {
      return true;
    }
    else
    {
      return false;
    }
    break;

  case O_WEAR_FLAGS:
    if (obj->flags_.wear_flags == obj_flags_.wear_flags || isSet(obj->flags_.wear_flags, obj_flags_.wear_flags))
    {
      return true;
    }
    else
    {
      return false;
    }
    break;
  case O_SIZE:
    if (obj->flags_.size == obj_flags_.size || isSet(obj->flags_.size, obj_flags_.size))
    {
      return true;
    }
    else
    {
      return false;
    }
    break;

  case O_EDD_KEYWORD:
    break;

  case O_EDD_DESCRIPTION:
    break;

  case O_WEIGHT:
    if (o_max_weight_ == -1 && obj->flags_.weight >= o_min_weight_)
    {
      return true;
    }
    else if (obj->flags_.weight >= o_min_weight_ && obj->flags_.weight <= o_max_weight_)
    {
      return true;
    }
    break;

  case O_COST:
    if (o_max_cost_ == -1 && obj->flags_.cost >= o_min_cost_)
    {
      return true;
    }
    else if (obj->flags_.cost >= o_min_cost_ && obj->flags_.cost <= o_max_cost_)
    {
      return true;
    }
    break;

  case O_MORE_FLAGS:
    if (obj_flags_.more_flags == obj->flags_.more_flags || isSet(obj->flags_.more_flags, obj_flags_.more_flags))
    {
      return true;
    }
    else
    {
      return false;
    }
    break;
  case O_EXTRA_FLAGS:
    if (obj->flags_.extra_flags == obj_flags_.extra_flags || isSet(obj->flags_.extra_flags, obj_flags_.extra_flags))
    {
      return true;
    }
    else
    {
      return false;
    }
    break;
  case O_EQ_LEVEL:
    if (o_max_level_ == -1 && obj->flags_.eq_level >= o_min_level_)
    {
      return true;
    }
    else if (obj->flags_.eq_level >= o_min_level_ && obj->flags_.eq_level <= o_max_level_)
    {
      return true;
    }
    break;

  case O_V1:
    if (o_value[0] == obj->flags_.value[0])
    {
      return true;
    }
    break;

  case O_V2:
    if (o_value[1] == obj->flags_.value[1])
    {
      return true;
    }
    break;

  case O_V3:
    if (o_value[2] == obj->flags_.value[2])
    {
      return true;
    }
    break;

  case O_V4:
    if (o_value[3] == obj->flags_.value[3])
    {
      return true;
    }
    break;

  case O_AFFECTED:
    break;
  case O_CARRIED_BY:
  case O_EQUIPPED_BY:
  case LIMIT:
    break;
  }
  return false;
}

bool search_object(ObjectPtr obj, QList<Search> sl)
{
  bool matches = false;
  for (qsizetype i = {}; i < sl.size(); ++i)
  {
    if (sl[i].getType() == Search::types::LIMIT)
    {
      continue;
    }

    if (sl[i] == obj)
    {
      matches = true;
    }
    else
    {
      matches = false;
      break;
    }
  }

  return matches;
}

ReturnValues Character::do_search(QStringList arguments, cmd_t cmd)
{
  if (arguments.isEmpty())
  {
    arguments.append("help");
  }

  // Create search object based on parameters

  QList<Search> sl;
  QString arg1, arg2;
  bool equals = false, greater = false, greater_equals = false, lesser = false, lesser_equals = false, search_world = false, show_affects = false, show_details = false;
  for (qsizetype i = {}; i < arguments.size(); ++i)
  {
    Search so = {};
    // ch->send(fmt::format("{} [{}]\r\n", i, parsables[i]));
    if (arguments[i].contains('=') && !arguments[i].contains(">=") && !arguments[i].contains("<="))
    {
      QStringList equal_buffer = arguments[i].split('=');
      arg1 = equal_buffer.at(0);
      arg2 = equal_buffer.at(1);
      equals = true;
    }
    else if (arguments[i].contains('>') && !arguments[i].contains(">="))
    {
      QStringList equal_buffer = arguments[i].split('>');
      arg1 = equal_buffer.at(0);
      arg2 = equal_buffer.at(1);
      greater = true;
    }
    else if (arguments[i].contains(">="))
    {
      QStringList equal_buffer = arguments[i].split(">=");
      arg1 = equal_buffer.at(0);
      arg2 = equal_buffer.at(1);
      greater_equals = true;
    }
    else if (arguments[i].contains('<') && !arguments[i].contains("<="))
    {
      QStringList equal_buffer = arguments[i].split('<');
      arg1 = equal_buffer.at(0);
      arg2 = equal_buffer.at(1);
      lesser = true;
    }
    else if (arguments[i].contains("<="))
    {
      QStringList equal_buffer = arguments[i].split("<=");
      arg1 = equal_buffer.at(0);
      arg2 = equal_buffer.at(1);
      lesser_equals = true;
    }
    else
    {
      arg1 = arguments[i];
    }

    if (arg1 == "help" || arg1 == "?")
    {
      send(u"Usage: search <search terms> <other options>\r\n"_s);
      send(u"       Searches object database.\r\n\r\n"_s);
      send(u"Usage: search world <search terms> <other options>\r\n"_s);
      send(u"       Searches your reachable world.\r\n\r\n"_s);
      send(u"Search terms:\r\n"_s);
      send(u"level=50      show objects that are level 50.\r\n"_s);
      send(u"level=50-60   show objects with level including and between 50 to 60.\r\n"_s);
      send(u"level=?       show objects with level including and between 50 to 60.\r\n"_s);
      send(u"type=weapon   show objects of type weapon.\r\n"_s);
      send(u"type=?        show available object types.\r\n"_s);
      send(u"wear=neck     show objects that can be worn on the neck.\r\n"_s);
      send(u"wear=?        show available wear locations.\r\n"_s);
      send(u"size=small    show objects that can be worn on the neck.\r\n"_s);
      send(u"size=?        show available sizes.\r\n"_s);
      send(u"extra=mage    show objects that have the extra flag for mage set.\r\n"_s);
      send(u"extra=?       show available extra flags_.\r\n"_s);
      send(u"more=unique   show objects that have the extra flag for mage set.\r\n"_s);
      send(u"more=?        show available extra flags_.\r\n"_s);
      send(u"name=moss     show objects matching keyword moss.\r\n"_s);
      send(u"xyz           show objects matching keyword xyz.\r\n"_s);
      send(u"Search terms can be combined.\r\n"_s);
      send(u"Example: search level=1-10 type=ARMOR golden\r\n"_s);
      send(u"\r\n"_s);
      send(u"Other options:\r\n"_s);
      send(u"limit=10       limit output to only 10 results.\r\n"_s);
      send(u"affects        shows affects.\r\n"_s);
      send(u"details        shows certain details depending on object type.\r\n"_s);
      return ReturnValue::eSUCCESS;
    }
    else if (arg1 == "world")
    {
      search_world = true;
    }
    else if (arg1 == "affects")
    {
      show_affects = true;
    }
    else if (arg1 == "details")
    {
      show_details = true;
    }
    else if (arg1 == "level")
    {
      so.setType(Search::types::O_EQ_LEVEL);

      if (arg2.isEmpty() || arg2 == "?")
      {
        so.enableShowRange();
        sl.push_back(so);
      }
      else
      {

        // #-#
        // #-
        if (arg2.contains('-'))
        {
          QStringList equal_buffer = arg2.split('-');
          arg1 = equal_buffer.at(0);
          arg2 = equal_buffer.at(1);
          if (!arg1.isEmpty())
          {
            so.setObjectMinimumLevel(arg1.toULongLong());
            if (!arg2.isEmpty())
            {
              so.setObjectMaximumLevel(arg2.toULongLong());
            }
            else
            {
              so.setObjectMaximumLevel(-1);
            }

            sl.push_back(so);
          }
        }
        else // #
        {
          if (!arg2.isEmpty())
          {
            if (greater)
            {
              so.setObjectMinimumLevel(arg2.toULongLong() + 1);
              so.setObjectMaximumLevel(-1);
            }
            else if (greater_equals)
            {
              so.setObjectMinimumLevel(arg2.toULongLong());
              so.setObjectMaximumLevel(-1);
            }
            else if (lesser)
            {
              so.setObjectMinimumLevel(0);
              so.setObjectMaximumLevel(arg2.toULongLong() - 1);
            }
            else if (lesser_equals)
            {
              so.setObjectMinimumLevel(0);
              so.setObjectMaximumLevel(arg2.toULongLong());
            }
            else
            {
              so.setObjectMinimumLevel(arg2.toULongLong());
              so.setObjectMaximumLevel(arg2.toULongLong());
            }

            sl.push_back(so);
          }
        }
      }
    }
    else if (arg1 == "name")
    {
      if (arg2.isEmpty())
      {
        send(u"What name are you searching for? You can use name=value multiple times.\r\n"_s);
        return ReturnValue::eFAILURE;
      }
      else
      {
        qsizetype quote;
        while ((quote = arg2.indexOf('"')) != -1)
        {
          arg2.remove(quote, 1);
        }

        // Ex. name=woodbey
        so.setObjectName(arg2);
        so.setType(Search::types::O_NAME);
        sl.push_back(so);
      }
    }
    else if (arg1 == "type")
    {
      bool found = false;
      if (!arg2.isEmpty())
      {
        arg2 = arg2.toUpper();

        for (qsizetype i = {}; i < item_types.size(); ++i)
        {
          if (item_types[i].indexOf(arg2) == 0)
          {
            found = true;
            so.setObjectType(i);
            so.setType(Search::types::O_TYPE_FLAG);
            sl.push_back(so);
            break;
          }
        }
      }

      if (!found)
      {
        send(u"What type are you searching for?\r\n"_s);
        send(u"Here are some valid types:\r\n"_s);
        for (const auto &i : item_types)
        {
          send(i + "\r\n");
        }
        return ReturnValue::eFAILURE;
      }
    }
    else if (arg1 == "wear")
    {
      bool found = false;
      if (!arg2.isEmpty())
      {
        auto location = parse_bitstrings<ObjectPositions>(arg2);
        if (location)
        {
          found = true;
          so.setObjectWearLocation(location);
          so.setType(Search::types::O_WEAR_FLAGS);
          sl.push_back(so);
        }
      }
      if (!found)
      {
        send(u"What type are you searching for?\r\n"_s);
        send(u"Here are some valid wear locations:\r\n"_s);
        for (const auto &w : QFlagsToStrings<ObjectPositions>())
        {
          send(w + "\r\n");
        }
        return ReturnValue::eFAILURE;
      }
    }
    else if (arg1 == "size")
    {
      bool found = false;
      if (!arg2.isEmpty())
      {
        quint32 size = {};
        parse_bitstrings_into_int(Object::size_bits, arg2, nullptr, size);
        if (size)
        {
          found = true;
          so.setObjectSize(size);
          so.setType(Search::types::O_SIZE);
          sl.push_back(so);
        }
      }
      if (!found)
      {
        send(u"What size are you searching for?\r\n"_s);
        send(u"Here are some valid sizes:\r\n"_s);
        for (const auto &s : Object::size_bits)
        {
          send(s + "\r\n");
        }
        return ReturnValue::eFAILURE;
      }
    }
    else if (arg1 == "more")
    {
      bool found = false;
      if (!arg2.isEmpty())
      {
        quint32 more_flag = {};
        parse_bitstrings_into_int(Object::more_obj_bits, arg2, nullptr, more_flag);
        if (more_flag)
        {
          found = true;
          so.setObjectMore(more_flag);
          so.setType(Search::types::O_MORE_FLAGS);
          sl.push_back(so);
        }
      }
      if (!found)
      {
        send(u"What more flag are you searching for?\r\n"_s);
        send(u"Here are some valid more flags:\r\n"_s);
        for (const auto &s : Object::more_obj_bits)
        {
          send(s + "\r\n");
        }
        return ReturnValue::eFAILURE;
      }
    }
    else if (arg1 == "extra")
    {
      bool found = false;
      if (!arg2.isEmpty())
      {
        quint32 extra_flag = {};
        parse_bitstrings_into_int(Object::extra_bits, arg2, nullptr, extra_flag);
        if (extra_flag)
        {
          found = true;
          so.setObjectExtra(extra_flag);
          so.setType(Search::types::O_EXTRA_FLAGS);
          sl.push_back(so);
        }
      }
      if (!found)
      {
        send(u"What extra flag are you searching for?\r\n"_s);
        send(u"Here are some valid extra flags:\r\n"_s);
        for (const auto &s : Object::extra_bits)
        {
          send(s + "\r\n");
        }
        return ReturnValue::eFAILURE;
      }
    }
    else if (arg1 == "limit")
    {
      if (!arg2.isEmpty())
      {
        so.setType(Search::types::LIMIT);
        so.setLimitOutput(arg2.toULongLong());
        sl.push_back(so);
      }
    }
    else
    {
      so.setObjectName(arg1);
      so.setType(Search::types::O_NAME);
      sl.push_back(so);
    }
  }

  bool header_shown = false;
  size_t objects_found = {};

  QList<Result> obj_results;
  quint64 limit_output = {};

  if (search_world)
  {
    quint64 old_count = {};

    // search inventory
    for (auto obj = carrying; obj; obj = obj->next_content)
    {
      if (CAN_SEE_OBJ(this, obj))
      {
        if (search_object(obj, sl))
        {
          obj_results.push_back({Search::locations::in_inventory, obj});
        }

        // containers must be open to search them
        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !isSet(obj->flags_.value[1], CONT_CLOSED))
        {
          // search inventory containers
          for (auto obj_in_container = obj->contains; obj_in_container != nullptr; obj_in_container = obj_in_container->next_content)
          {
            if (CAN_SEE_OBJ(this, obj_in_container))
            {
              if (search_object(obj_in_container, sl))
              {
                obj_results.push_back({Search::locations::in_inventory, obj_in_container});
              }
            }
          }
        }
      }
    }
    send(u"%1 matches found in inventory.\r\n"_s.arg(obj_results.size() - old_count));
    old_count = obj_results.size();

    // Search equipment
    for (auto k = 0U; k < MAX_WEAR; k++)
    {
      auto obj = equipment[k];
      if (obj != nullptr)
      {
        if (CAN_SEE_OBJ(this, obj))
        {
          if (search_object(obj, sl))
          {
            obj_results.push_back({Search::locations::in_equipment, obj});
          }

          if (GET_ITEM_TYPE(obj) == ITEM_CONTAINER && !isSet(obj->flags_.value[1], CONT_CLOSED))
          {
            for (auto obj_in_container = obj->contains; obj_in_container != nullptr; obj_in_container = obj_in_container->next_content)
            {
              if (CAN_SEE_OBJ(this, obj_in_container))
              {
                if (search_object(obj_in_container, sl))
                {
                  obj_results.push_back({Search::locations::in_equipment, obj_in_container});
                }
              }
            }
          }
        }
      }
    }
    send(u"%1 matches found among worn equipment.\r\n"_s.arg(obj_results.size() - old_count));
    old_count = obj_results.size();

    // search room
    for (auto obj = dc_->world[in_room].contents; obj; obj = obj->next_content)
    {
      if (CAN_SEE_OBJ(this, obj))
      {
        if (search_object(obj, sl))
        {
          obj_results.push_back({Search::locations::in_room, obj});
        }

        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !isSet(obj->flags_.value[1], CONT_CLOSED))
        {
          // search inventory containers
          for (auto obj_in_container = obj->contains; obj_in_container != nullptr; obj_in_container = obj_in_container->next_content)
          {
            if (CAN_SEE_OBJ(this, obj_in_container))
            {
              if (search_object(obj_in_container, sl))
              {
                obj_results.push_back({Search::locations::in_room, obj_in_container});
              }
            }
          }
        }
      }
    }
    send(u"%1 matches found in room.\r\n"_s.arg(obj_results.size() - old_count));
    old_count = obj_results.size();

    // Search all vaults player has access to
    quint64 vaults_searched = {};
    extern VaultPtr vault_table;
    for (auto vault = vault_table; vault; vault = vault->next)
    {
      if (vault && !vault->owner.isEmpty() && dc_->has_vault_access(qPrintable(name()), vault))
      {
        vaults_searched++;
        vault_items_dataPtr items;
        sorted_vault sv;
        sort_vault(*vault, sv);
        if (!sv.vault_contents.isEmpty())
        {
          for (const auto &o_short_description : sv.vault_contents)
          {
            const auto &o = sv.vault_content_qty[o_short_description];
            const auto &obj = o.first;
            const auto &count = o.second;

            bool matches = false;

            for (qsizetype i = {}; i < sl.size(); ++i)
            {
              if (sl[i].getType() == Search::types::LIMIT)
              {
                limit_output = sl[i].getLimitOutput();
                continue;
              }

              if (sl[i] == obj)
              {
                matches = true;
              }
              else
              {
                matches = false;
                break;
              }
            }

            if (matches)
            {
              for (auto counter = 0U; counter < count; counter++)
              {
                obj_results.push_back({Search::locations::in_vault, obj, vault->owner});
              }
            }
          }
        }
      }
    }

    if (vaults_searched == 1)
    {
      send(u"%1 matches found within 1 vault.\r\n"_s.arg(obj_results.size() - old_count));
    }
    else
    {
      send(u"%1 matches found within %2 vaults.\r\n"_s.arg(obj_results.size() - old_count).arg(vaults_searched));
    }
    old_count = obj_results.size();

    if (clan)
    {
      // search clan vault is able

      QString vault_name = u"clan%1"_s.arg(clan);
      auto vault = dc_->vaults_->has_vault(qPrintable(vault_name));
      // search vault if able
      if (vault)
      {
        vault_items_dataPtr items;
        sorted_vault sv;
        sort_vault(*vault, sv);
        if (!sv.vault_contents.isEmpty())
        {
          for (const auto &o_short_description : sv.vault_contents)
          {
            const auto &o = sv.vault_content_qty[o_short_description];
            const auto &obj = o.first;
            const auto &count = o.second;

            bool matches = false;

            for (qsizetype i = {}; i < sl.size(); ++i)
            {
              if (sl[i].getType() == Search::types::LIMIT)
              {
                limit_output = sl[i].getLimitOutput();
                continue;
              }

              if (sl[i] == obj)
              {
                matches = true;
              }
              else
              {
                matches = false;
                break;
              }
            }

            if (matches)
            {
              for (auto counter = 0U; counter < count; counter++)
              {
                obj_results.push_back({Search::locations::in_clan_vault, obj, vault_name});
              }
            }
          }
        }
      }
    }

    bool showed_ranges = false;

    for (const auto &s : sl)
    {
      if (s.isShowRange())
      {
        if (s.getType() == Search::types::O_EQ_LEVEL)
        {
          quint64 min_level = 100, max_level = {};
          for (const auto &result : obj_results)
          {
            auto obj = result.getObject();
            if (obj->getLevel() > max_level)
            {
              max_level = obj->getLevel();
            }
            else if (obj->getLevel() < min_level)
            {
              min_level = obj->getLevel();
            }
          }

          send(u"Within %1 results the levels were %2-%3\r\n"_s.arg(obj_results.size()).arg(min_level).arg(max_level));

          showed_ranges = true;
        }
      }
    }

    if (showed_ranges)
    {
      return ReturnValue::eSUCCESS;
    }
  }
  else
  {
    for (qint32 vnum = {}; vnum < dc_->obj_index_[top_of_objt]->vnum(); ++vnum)
    {
      qint32 rnum = {};
      // real_object returns -1 for missing VNUMs
      if ((rnum = real_object(vnum)) < 0)
      {
        continue;
      }
      ObjectPtr obj = static_cast<ObjectPtr>(dc_->obj_index_[rnum]->item);
      if (obj == nullptr)
      {
        continue;
      }

      if (obj->isDark() && !isImmortalPlayer())
      {
        continue;
      }

      bool matches = false;

      for (qsizetype i = {}; i < sl.size(); ++i)
      {
        if (sl[i].getType() == Search::types::LIMIT)
        {
          limit_output = sl[i].getLimitOutput();
          continue;
        }

        if (sl[i] == obj)
        {
          matches = true;
        }
        else
        {
          matches = false;
          break;
        }
      }

      if (matches)
      {
        obj_results.push_back({Search::locations::in_object_database, obj});
      }
    }
    if (obj_results.isEmpty())
    {
      send(u"Searching $B%1$R objects...No results found.\r\n"_s.arg(top_of_objt));
      return ReturnValue::eFAILURE;
    }

    bool showed_ranges = false;

    for (const auto &s : sl)
    {
      if (s.isShowRange())
      {
        if (s.getType() == Search::types::O_EQ_LEVEL)
        {
          quint64 min_level = 100, max_level = {};
          for (const auto &result : obj_results)
          {
            auto obj = result.getObject();
            if (obj->getLevel() > max_level)
            {
              max_level = obj->getLevel();
            }
            else if (obj->getLevel() < min_level)
            {
              min_level = obj->getLevel();
            }
          }

          send(u"Within %1 results the levels were %2-%3\r\n"_s.arg(obj_results.size()).arg(min_level).arg(max_level));

          showed_ranges = true;
        }
      }
    }

    if (showed_ranges)
    {
      return ReturnValue::eSUCCESS;
    }
  }

  if (!search_world)
  {
    if (limit_output)
    {
      send(u"Searching %1 objects...%2 matches found. Limiting output to %3 matches.\r\n"_s.arg(top_of_objt).arg(obj_results.size()).arg(limit_output));
    }
    else
    {
      send(u"Searching %1 objects...%2 matches found.\r\n"_s.arg(top_of_objt).arg(obj_results.size()));
    }
  }

  QString header;
  qsizetype max_keyword_size = 0, max_short_description_size = {};

  quint64 result_nr = {};
  for (const auto &result : obj_results)
  {
    auto obj = result.getObject();
    if (limit_output > 0 && ++result_nr > limit_output)
    {
      break;
    }
    if (obj->name().size() > max_keyword_size)
    {
      max_keyword_size = obj->name().size();
    }

    if (nocolor_strlen(qPrintable(obj->short_description())) > max_short_description_size)
    {
      max_short_description_size = nocolor_strlen(qPrintable(obj->short_description()));
      max_short_description_size = MAX(20, max_short_description_size);
    }
  }

  if (std::count_if(sl.begin(), sl.end(), [](Search search_item)
                    { return (search_item.getType() == Search::types::O_NAME); }))
  {
    header += u" [%1]"_s.arg("Keywords", -max_keyword_size);
  }

  if (search_world)
  {
    header += u" [%1]"_s.arg("Location", 19);
  }

  if (true || std::count_if(sl.begin(), sl.end(), [](Search search_item)
                            { return (search_item.getType() == Search::types::O_SHORT_DESCRIPTION); }))
  {
    header += u" [%1]"_s.arg(u"Short Description"_s, -max_short_description_size);
  }

  if (show_details)
  {
    if (search_world)
    {
      header += u" [%1]"_s.arg(u"Details"_s, -21);
    }
    else
    {
      header += u" [%1]"_s.arg(u"Details"_s);
    }
  }

  if (show_affects)
  {
    header += u" [%1]"_s.arg(u"Affects"_s);
  }

  send(u"$7$B[ VNUM] [ LV]%1$R\r\n"_s.arg(header));

  result_nr = {};
  for (const auto &result : obj_results)
  {
    auto obj = result.getObject();
    if (limit_output > 0 && ++result_nr > limit_output)
    {
      break;
    }
    QString custom_columns;

    if (std::count_if(sl.begin(), sl.end(), [](Search search_item)
                      { return (search_item.getType() == Search::types::O_NAME); }))
    {
      custom_columns += u" [%1]"_s.arg(obj->name(), -max_keyword_size);
    }

    if (search_world)
    {
      switch (result.getLocation())
      {
      case Search::locations::in_inventory:
        custom_columns += u" [%1]"_s.arg("inventory", 19);
        break;
      case Search::locations::in_equipment:
        custom_columns += u" [%1]"_s.arg("equipped", 19);
        break;
      case Search::locations::in_room:
        custom_columns += u" [%1]"_s.arg("in room", 19);
        break;
      case Search::locations::in_vault:
      case Search::locations::in_clan_vault:
        custom_columns += u" [%1 vault]"_s.arg(result.getName(), 13);
        break;
      case Search::in_object_database:
        break;
      }
    }

    // For now short description is always shown
    if (true ||
        std::count_if(sl.begin(), sl.end(), [](Search search_item)
                      { return (search_item.getType() == Search::types::O_SHORT_DESCRIPTION); }))
    {
      // Because the color codes make the QString longer then it visually appears, we calculate that color code difference and add it to our max_short_description_size to get alignment right
      custom_columns += u" [%1]"_s.arg(obj->short_description(), -(dc_strlen(qPrintable(obj->short_description())) - nocolor_strlen(obj->short_description()) + max_short_description_size));
    }

    // Needed to show details or affects below
    const ObjectPtr vobj = {};
    if (obj->item_number >= 0)
    {
      const qint32 vnum = dc_->obj_index_[obj->item_number]->vnum();
      if (vnum >= 0)
      {
        const qint32 rn_of_vnum = real_object(vnum);
        if (rn_of_vnum >= 0)
        {
          vobj = dc_->obj_index_[rn_of_vnum]->item;
        }
      }
    }

    QString buffer;
    if (show_details)
    {
      switch (GET_ITEM_TYPE(obj))
      {
      case ITEM_WEAPON:
        buffer = u"%1D%2"_s.arg(obj->flags_.value[1]).arg(obj->flags_.value[2]);

        if (search_world && vobj != nullptr)
        {
          buffer += " (";
          buffer += getStatDiff(vobj->flags_.value[1], obj->flags_.value[1]);
          buffer += "D";
          buffer += getStatDiff(vobj->flags_.value[2], obj->flags_.value[2]);
          buffer += ")";
        }
        break;
      default:
        buffer = "";
        break;
      }
      custom_columns += u" [%1]"_s.arg(buffer, -21);
      /*
               qint32 get_weapon_damage_type(ObjectPtr  wielded);
               bits = get_weapon_damage_type(obj) - 1000;
               extern QStringList strs_damage_types;
               ch->send(u"$3Damage type$R: %s\r\n"_s.arg(strs_damage_types[bits]));
      */
    }

    if (show_affects)
    {
      quint64 affects_found = {};
      for (qint16 i = {}; i < obj->num_affects; i++)
      {
        if ((obj->affected[i].location != APPLY_NONE) &&
            (obj->affected[i].modifier != 0 ||
             (vobj != nullptr &&
              i < vobj->num_affects &&
              !vobj->affected.isEmpty() &&
              vobj->affected[i].location == obj->affected[i].location)))
        {
          QString buffer;
          if (obj->affected[i].location < 1000)
          {
            buffer = sprinttype(obj->affected[i].location, Object::apply_types);
          }
          else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
          {
            buffer = get_skill_name(obj->affected[i].location / 1000);
          }
          else
          {
            buffer = "Invalid";
          }

          if (affects_found++ == 0)
          {
            custom_columns += u" ["_s;
          }
          else
          {
            custom_columns += u","_s);
          }

          if (obj->affected[i].modifier > 0)
          {
            custom_columns += u"$R%1%2+%3$R"_s.arg(buffer).arg(getSettingAsColor("color.good")).arg(obj->affected[i].modifier);
          }
          else
          {
            custom_columns += u"$R%1%2-%3$R").arg(buffer).arg(getSettingAsColor("color.bad"_s.arg(obj->affected[i].modifier);
          }
        }
      }
      if (affects_found)
      {
        custom_columns += u"]"_s;
      }
    }

    send(u"[%1] [%2]%3\r\n"_s.arg(GET_OBJ_VNUM(obj), 5).arg(obj->flags_.eq_level, 3).arg(custom_columns));
  }
  send(u"\r\nIdentify a virtual object with the command: identify v####\r\n"_s);

  return ReturnValue::eSUCCESS;
}

// search type = armor woodbey level = ?