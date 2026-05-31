/************************************************************************
| $Id: non_off.cpp,v 1.62 2011/08/28 18:24:54 jhhudso Exp $
| non_off.C
| Description:  Implementation of generic, non-offensive commands.
*/
/*****************************************************************************/
/* Revision History                                                          */
/* 12/08/2003   Onager   Revised do_tap() to prevent sacrifices in donations */
/*****************************************************************************/

#include "DC/DC.h"

// decay variable means it's from a decaying corpse, not a player
void log_sacrifice(CharacterPtr ch, ObjectPtr obj, bool decay = false)
{

  if (GET_OBJ_RNUM(obj) == INVALID_ROOM)
    return;

  if (!decay)
  {
    dc_->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "%s just sacrificed %s[%d] in room %d\n", qPrintable(ch->name()), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), GET_ROOM_VNUM(ch->in_room));
  }
  else
  {
    dc_->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "%s just poofed from decaying corpse %s[%d] in room %d\n", GET_OBJ_SHORT(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), GET_ROOM_VNUM(obj->in_room));
  }

  for (ObjectPtr loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
  {
    dc_->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]\n",
              GET_OBJ_SHORT(obj),
              GET_OBJ_SHORT(loop_obj),
              GET_OBJ_VNUM(loop_obj));
  }
}

ReturnValues do_sacrifice(CharacterPtr ch, QString argument, cmd_t cmd)
{
  ObjectPtr obj;
  QString name;

  if (isSet(dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  if (name.isEmpty() || !str_cmp(name, qPrintable(ch->name())))
  {
    act_to_room("$n offers $mself to $s god, who graciously declines.", ch, 0, 0, 0);
    act_to_character("Your god appreciates your offer and may accept it later.", ch, 0, 0, 0);
    return ReturnValue::eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, name, ch->carrying);

  /* Ok, lets see if it's a corpse on the ground then */
  if (obj == nullptr)
  {
    obj = get_obj_in_list_vis(ch, name, dc_->world[ch->in_room]->contents_);
    if (obj == nullptr || GET_ITEM_TYPE(obj) != ITEM_CONTAINER || !isexact("corpse", obj->name()) || isexact("pc", obj->name()))
    {
      act_to_character("You don't seem to be holding that object.", ch, 0, 0, 0);
      return ReturnValue::eFAILURE;
    }
  }

  if (isSet(obj->flags_.extra_flags, ITEM_NODROP))
  {
    if (ch->isMortalPlayer())
    {
      ch->sendln(u"You are unable to destroy this item, it must be CURSED!"_s);
      return ReturnValue::eFAILURE;
    }
    else
      ch->sendln(u"(This item is cursed, BTW.)"_s);
  }

  if (obj->flags_.value[3] == 1 && isexact("pc", obj->name()))
  {
    ch->sendln(u"You probably don't *really* want to do that."_s);
    return ReturnValue::eFAILURE;
  }

  if (isSet(obj->flags_.extra_flags, ITEM_SPECIAL) && ch->getLevel() < ANGEL)
  {
    ch->sendln(u"God, what a stupid fucking thing for you to do."_s);
    return ReturnValue::eFAILURE;
  }

  if (dc_->obj_index_[obj->item_number]->vnum() == CHAMPION_ITEM)
  {
    ch->sendln(u"In soviet russia, champion flag sacrifice YOU!"_s);
    return ReturnValue::eFAILURE;
  }

  if (ch->isPlayerCantQuit() && !ch->isNonPlayer() && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    ch->sendln(u"Your criminal acts prohibit it."_s);
    return ReturnValue::eFAILURE;
  }

  /* don't let people sac stuff in donations */
  if (ch->in_room == 3099)
  {
    ch->sendln(u"Not in the donation room."_s);
    return (ReturnValue::eFAILURE);
  }

  if (isSet(obj->flags_.more_flags, ITEM_LIMIT_SACRIFICE) && obj->contains)
  {
    act_to_character("You attempt to sacrifice $p to the gods but they refuse your foolish gift. Empty it first.", ch, obj, 0, 0);
    act_to_room("$n attempts to foolishly sacrifices $p to $s god.", ch, obj, 0, 0);
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n sacrifices $p to $s god.", ch, obj, 0, 0);
  act_to_character("You sacrifice $p to the gods and receive one $B$5gold$R coin.", ch, obj, 0, 0);
  ch->addGold(1);
  log_sacrifice(ch, obj);
  extract_obj(obj);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_visible(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (ch->affected_by_spell(SPELL_INVISIBLE))
  {
    affect_from_char(ch, SPELL_INVISIBLE);
    ch->sendln(u"You drop your invisiblity spell."_s);
    if (!IS_AFFECTED(ch, AFF_INVISIBLE))
      act_to_room("$n slowly fades into existence.", ch, 0, 0, 0);
    else
      ch->sendln(u"You must remove the equipment making you invis to become visible."_s);
    return ReturnValue::eSUCCESS;
  }

  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    ch->sendln(u"You must remove the equipment making you invis to become visible."_s);
  else
    ch->sendln(u"You aren't invisible."_s);

  return ReturnValue::eSUCCESS;
}

ReturnValues do_donate(CharacterPtr ch, QString argument, cmd_t cmd)
{
  ObjectPtr obj = {};
  QString name;
  QString buf = {};
  qint32 location = {};
  qint32 room = 3099;
  qint32 origin = {};

  if (isSet(dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  if (ch->fighting)
  {
    ch->sendln(u"Aren't we a little to busy for that right now?"_s);
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, name);

  if (name.isEmpty())
  {
    ch->sendln(u"Donate what?"_s);
    return ReturnValue::eFAILURE;
  }

  obj = get_obj_in_list_vis(ch, name, ch->carrying);
  if (obj == nullptr)
  {
    dc_sprintf(buf, "You don't have any '%s' to donate.", name);
    act_to_character(buf, ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  if (ch->isPlayerCantQuit() && !ch->isNonPlayer() && ch->affected_by_spell(Character::PLAYER_OBJECT_THIEF))
  {
    ch->sendln(u"Your criminal acts prohibit it."_s);
    return ReturnValue::eFAILURE;
  }

  // Handle yielding the champion flag
  if (GET_OBJ_VNUM(obj) == 45)
  {
    if (isSet(dc_->world[ch->in_room]->room_flags_, SAFE))
    {
      if (IS_AFFECTED(ch, AFF_CHAMPION))
      {
        REMBIT(ch->affected_by, AFF_CHAMPION);

        dc_sprintf(buf, "\r\n##%s has just yielded %s!\r\n", qPrintable(ch->name()), qPrintable(obj->short_description()));
        send_info(buf);

        affected_type af;
        af.type = OBJ_CHAMPFLAG_TIMER;
        af.duration = 5;
        af.modifier = {};
        af.location = APPLY_NONE;
        af.bitvector = -1;
        affect_to_char(ch, &af);

        act_to_room("$n yields $p.", ch, obj, 0, 0);
        act_to_character("You yield $p.", ch, obj, 0, 0);

        location = CFLAG_HOME;
        origin = ch->in_room;
        move_char(ch, location);

        act_to_room("$p falls from the heavens...", ch, obj, 0, INVIS_NULL);

        move_char(ch, origin);
        move_obj(obj, location);

        ch->save();
        return ReturnValue::eSUCCESS;
      }
      else
      {
        dc_sprintf(buf, "%s had %s, but no AFF_CHAMPION.", qPrintable(ch->name()), qPrintable(obj->short_description()));
        dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
        return ReturnValue::eFAILURE;
      }
    }
    else
    {
      ch->sendln(u"You can only yield %1 from a safe room."_s.arg(obj->short_description()));
      return ReturnValue::eFAILURE;
    }
  }

  if (isSet(obj->flags_.extra_flags, ITEM_NODROP))
  {
    ch->sendln(u"Since you can't let go of it, how are you going to donate it?"_s);
    return ReturnValue::eFAILURE;
  }

  if (isSet(obj->flags_.more_flags, ITEM_NO_TRADE))
  {
    if (ch->getLevel() > IMMORTAL)
    {
      ch->sendln(u"That was a NO_TRADE item btw...."_s);
    }
    else
    {
      ch->sendln(u"It seems magically attached to you."_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (contains_no_trade_item(obj))
  {
    if (ch->getLevel() > IMMORTAL)
    {
      ch->sendln(u"That was a NO_TRADE item btw...."_s);
    }
    else
    {
      ch->sendln(u"Something inside it seems magically attached to you."_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (isSet(obj->flags_.extra_flags, ITEM_SPECIAL))
  {
    ch->sendln(u"You can't donate godload equipment."_s);
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n donates $p.", ch, obj, 0, 0);
  act_to_character("You donate $p.", ch, obj, 0, 0);

  if (obj->flags_.type_flag != ITEM_MONEY)
  {
    QString log_buf = {};
    dc_sprintf(log_buf, "%s donates %s[%d]", qPrintable(ch->name()), qPrintable(obj->name()), dc_->obj_index_[obj->item_number]->vnum());
    dc_->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
    for (ObjectPtr loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
      dc_->logf(IMPLEMENTER, DC::LogChannel::LOG_OBJECTS, "The %s contained %s[%d]", qPrintable(obj->short_description()),
                qPrintable(loop_obj->short_description()),
                dc_->obj_index_[loop_obj->item_number]->vnum());
  }

  location = room;
  origin = ch->in_room;
  move_char(ch, location, false);

  act_to_room("$n has made a donation...", ch, obj, 0, 0);
  act("$p falls through a glowing white portal in the top of the ceiling.",
      ch, obj, 0, TO_ROOM, INVIS_NULL);

  move_char(ch, origin, false);
  move_obj(obj, location);

  ch->save();
  return ReturnValue::eSUCCESS;
}

auto Character::do_notitle(QStringList arguments, cmd_t cmd) -> ReturnValue
{
  sendln(u"You now have no title."_s);
  title_ = {};
  save(cmd_t::SAVE_SILENTLY);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_title(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  qint32 ctr;

  if (argument.isEmpty())
  {
    ch->sendln(u"Type \"title message\" to set a title or \"notitle\" to remove your title."_s);
    return ReturnValue::eFAILURE;
  }

  if (!ch->isNonPlayer() && isSet(ch->player->punish, PUNISH_NOTITLE))
  {
    ch->sendln(u"You can't do that.  You must have been naughty."_s);
    return ReturnValue::eFAILURE;
  }

  for (; isspace(*argument); argument++)
    ;

  if (dc_strlen(argument) > 40)
  {
    ch->sendln(u"Title field too big.  40 characters max."_s);
    return ReturnValue::eFAILURE;
  }

  if (strchr(argument, '[') || strchr(argument, ']'))
  {
    ch->sendln(u"You cannot have a '[' or a ']' in your title."_s);
    return ReturnValue::eFAILURE;
  }

  // TODO - decide if we still need this anymore since I think the $color code
  // keeps mortals from using $'s anyway  No idea why we don't let them use ?'s offhand
  for (ctr = {}; (quint32)ctr <= dc_strlen(argument); ctr++)
  {
    if (((argument[ctr] == '$') && (argument[ctr + 1] == '$')) || ((argument[ctr] == '?') && (argument[ctr + 1] == '?')))
    {
      ch->sendln(u"Your title is now: Common Dork of Dark Castle."_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (ch->title) // this should always be true, but why not check anyway?
    ch->title = {};
  ch->title = (argument);
  dc_sprintf(buf, "Your title is now: %s\r\n", argument);
  ch->send(buf);
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
  {
    send(u"You can't toggle anything, you're a mob!\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (arguments.isEmpty())
  {
    qsizetype longest_toggle_name = {};
    for (const auto &t : Player::togglables)
    {
      if (t.name_.size() > longest_toggle_name)
      {
        longest_toggle_name = t.name_.size();
      }
    }

    for (const auto &t : Player::togglables)
    {
      if (t.value_ != Player::PLR_GUIDE_TOG || (isSet(player->toggles, Player::PLR_GUIDE)))
      {
        send(u"%1 "_s.arg(t.name_, -11));
        send(u"%1\r\n"_s.arg(isSet(player->toggles, t.value_) ? t.on_message_ : t.off_message_));
      }
    }
    return ReturnValue::eSUCCESS;
  }

  bool txt_found = false;
  const QString arg1 = arguments.value(0);
  Toggle found_toggle;
  if (!arg1.isEmpty())
  {
    for (const auto &t : Player::togglables)
    {
      if (is_abbrev(arg1, t.name_))
      {
        txt_found = true;
        found_toggle = t;
        break;
      }
    }
  }

  if (!txt_found || !found_toggle.isValid())
  {
    send(u"Bad option.  Type toggle with no arguments for a list of "
         "good ones.\r\n");
    return ReturnValue::eFAILURE;
  }

  if (found_toggle.function_)
  {
    return (*(found_toggle.function_))({}, cmd_t::DEFAULT);
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_config(QStringList arguments, cmd_t cmd)
{
  if (player->config == nullptr)
  {
    player->config = new PlayerConfig();
  }

  if (arguments.isEmpty())
  {
    for (auto setting = player->config->constBegin(); setting != player->config->constEnd(); ++setting)
    {
      send(u"%1=%2\r\n"_s.arg(setting.key()).arg(setting.value()));
    }
    return ReturnValue::eSUCCESS;
  }

  QMap<QString, QString> colors;
  // colors["black"]="$0";
  colors["blue"] = "$1";
  colors["green"] = "$2";
  colors["cyan"] = "$3";
  colors["red"] = "$4";
  colors["yellow"] = "$5";
  colors["magenta"] = "$6";
  colors["white"] = "$7";
  colors["gray"] = "$B$0";
  colors["bright blue"] = "$B$1";
  colors["bright green"] = "$B$2";
  colors["bright cyan"] = "$B$3";
  colors["bright red"] = "$B$4";
  colors["bright yellow"] = "$B$5";
  colors["bright magenta"] = "$B$6";
  colors["bright white"] = "$B$7";

  if (arguments.size() > 0 && arguments.at(0) == "help")
  {
    send(u"Usage:\r\n"_s);
    send(u"config                       - Show all currently set configuration options.\r\n"_s);
    send(u"config color.good=color name - Set color to use for \"good\" values in game.\r\n"_s);
    send(u"config color.bad=color name  - Set color to use for \"bad\" values in game.\r\n"_s);
    send(u"                               Use ? as color name to see valid colors.\r\n"_s);
    send(u"config color.good=           - Unset color.good. Will use default \"good\" color.\r\n"_s);
    send(u"config color.bad=            - Unset color.bad. Will use default \"bad\" color.\r\n\r\n"_s);
    return ReturnValue::eSUCCESS;
  }

  QString argument1 = arguments.at(0);
  QStringList setting = argument1.split('=');
  QString key, value;
  if (setting.size() >= 1)
  {
    key = setting.at(0);
  }
  if (setting.size() == 2)
  {
    value = setting.at(1);
  }

  // config
  // config key
  if (setting.size() < 2)
  {
    bool found = false;
    for (auto i = player->config->begin(); i != player->config->end(); ++i)
    {
      if (key == conn->key() || key.isEmpty() || conn->key().startsWith(key))
      {
        found = true;
        send(u"%1=%2\r\n"_s.arg(conn->key()).arg(conn->value()));
      }
    }

    if (found == false)
    {
      if (key.isEmpty() == false)
      {
        send(u"%1 not found.\r\n"_s.arg(key));
      }
      else
      {
        send(u"No config options set.\r\n"_s);
      }
      return ReturnValue::eFAILURE;
    }

    return ReturnValue::eSUCCESS;
  }

  // config key=
  if (value.isEmpty() && key.isEmpty() == false && value.isEmpty() == true)
  {
    if (player->config->find(key) != player->config->end())
    {
      sendln(u"%1 unset."_s.arg(key));
      player->config->insert(key, QString());
      save(cmd_t::SAVE_SILENTLY);
      return ReturnValue::eSUCCESS;
    }
    send(u"%1 not found.\r\n"_s.arg(key));
    return ReturnValue::eFAILURE;
  }

  // config key=value
  if (key.isEmpty() == false && value.isEmpty() == false)
  {
    if (key == "color.good" || key == "color.bad")
    {
      if (colors.find(value) == colors.end())
      {
        if (value == "?")
        {
          send(u"Valid colors:\r\n"_s);
        }
        else
        {
          send(u"Invalid color specified. Valid colors:\r\n"_s);
        }

        for (auto color = colors.constBegin(); color != colors.constEnd(); ++color)
        {
          if (color.key() == "black")
          {
            send(u"%1\r\n"_s.arg(color.key()));
          }
          else
          {
            send(u"%1 - %2Example$R\r\n"_s.arg(color.key(), -15).arg(color.value()));
          }
        }

        return ReturnValue::eFAILURE;
      }
    }
    else if (key == "mode")
    {
      if (value.startsWith("character"))
      {
        telnet_sga(desc);
        telnet_echo_off(desc);
      }
      else if (value.startsWith("line") == false)
      {
        send(u"Valid telnet modes are line for linemode or character for character mode.\r\n"_s);
        return ReturnValue::eFAILURE;
      }
    }
    else if (key == "locale")
    {
      QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyTerritory);
      bool found_locale = false;
      for (const auto &locale : locales)
        if (locale.name() == value)
        {
          found_locale = true;
          break;
        }

      if (!found_locale)
      {
        if (value != u"?"_s)
        {
          sendln(u"'%1' is an invalid locale. Type config locale=? to see a list of valid locales."_s.arg(value));
          return ReturnValue::eSUCCESS;
        }
        sendln(u"Here's a list of valid locales:"_s);
        for (const auto &locale : locales)
        {
          sendln(locale.name());
        }
        return ReturnValue::eSUCCESS;
      }
    }
    else if (key == "timezone")
    {
      bool found = false;
      for (const auto &tz : QTimeZone::availableTimeZoneIds())
      {
        if (value == "?")
          sendln(tz);
        else if (value == tz)
        {
          found = true;
          break;
        }
      }
      if (value != "?")
      {
        if (!found)
        {
          sendln(u"Invalid timezone '%1' specified. Type config timezone=? to see the full list."_s.arg(value));
          return ReturnValue::eSUCCESS;
        }
      }
      else
        return ReturnValue::eSUCCESS;
    }
    else if (key == "dateformat")
    {
      if (u"TextDate"_s.compare(value, Qt::CaseInsensitive) &&
          u"ISODateWithMs"_s.compare(value, Qt::CaseInsensitive) &&
          u"ISODate"_s.compare(value, Qt::CaseInsensitive) &&
          u"RFC2822Date"_s.compare(value, Qt::CaseInsensitive))
      {
        sendln(u"Valid timestamp formats are:"_s);
        auto timezone_str = getSetting("timezone", "America/Chicago");
        auto timezone = QTimeZone(timezone_str.toLatin1());
        sendln(u"%1 %2"_s.arg("TextDate", 15).arg(QDateTime::currentDateTimeUtc().toTimeZone(timezone).toString(Qt::DateFormat::TextDate)));
        sendln(u"%1 %2"_s.arg("ISODateWithMs", 15).arg(QDateTime::currentDateTimeUtc().toTimeZone(timezone).toString(Qt::DateFormat::ISODateWithMs)));
        sendln(u"%1 %2"_s.arg("ISODate", 15).arg(QDateTime::currentDateTimeUtc().toTimeZone(timezone).toString(Qt::DateFormat::ISODate)));
        sendln(u"%1 %2"_s.arg("RFC2822Date", 15).arg(QDateTime::currentDateTimeUtc().toTimeZone(timezone).toString(Qt::DateFormat::RFC2822Date)));
        return ReturnValue::eSUCCESS;
      }
    }
    else if (!QRegularExpression("^(color.(good|bad)|(tell|gossip).history.timestamp|locale|mode|fighting.showdps|timezone)$").match(key).hasMatch())
    {
      send(u"Invalid config option.\r\n"_s);
      return ReturnValue::eFAILURE;
    }

    if (QRegularExpression("^((tell|gossip).history.timestamp|fighting.showdps)$").match(key).hasMatch() && !QRegularExpression("^([01tf]{1}|true|false)$").match(value).hasMatch())
    {
      sendln(u"Invalid config option. Valid options are: 0, 1, t, f, true and false."_s);
      return ReturnValue::eFAILURE;
    }

    player->config->insert(key, value);

    send(u"Setting %1=%2\r\n"_s.arg(key).arg(value));
    save(cmd_t::SAVE_SILENTLY);
    return ReturnValue::eSUCCESS;
  }

  return ReturnValue::eFAILURE;
}

ReturnValues Character::do_brief(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_BRIEF))
  {
    send(u"Brief mode $B$4off$R.\r\n"_s);
    REMOVE_BIT(player->toggles, Player::PLR_BRIEF);
  }
  else
  {
    send(u"Brief mode $B$2on$R.\r\n"_s);
    SET_BIT(player->toggles, Player::PLR_BRIEF);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_ansi(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_ANSI))
  {
    sendln(u"ANSI COLOR $B$4off$R."_s);
    REMOVE_BIT(player->toggles, Player::PLR_ANSI);
  }
  else
  {
    sendln(u"ANSI COLOR $B$2on$R."_s);
    SET_BIT(player->toggles, Player::PLR_ANSI);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_vt100(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_VT100))
  {
    sendln(u"VT100 $B$4off$R."_s);
    REMOVE_BIT(player->toggles, Player::PLR_VT100);
  }
  else
  {
    sendln(u"VT100 $B$2on$R."_s);
    SET_BIT(player->toggles, Player::PLR_VT100);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_compact(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_COMPACT))
  {
    sendln(u"Compact mode $B$4off$R."_s);
    REMOVE_BIT(player->toggles, Player::PLR_COMPACT);
  }
  else
  {
    sendln(u"Compact mode $B$2on$R."_s);
    SET_BIT(player->toggles, Player::PLR_COMPACT);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_summon_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_SUMMONABLE))
  {
    sendln(u"You may no longer be summoned by other players."_s);
    REMOVE_BIT(player->toggles, Player::PLR_SUMMONABLE);
  }
  else
  {
    send_to_char("You may now be summoned by other players.\r\n"
                 "Make _sure_ you want this...they could summon you to your death!\r\n",
                 this);
    SET_BIT(player->toggles, Player::PLR_SUMMONABLE);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_lfg_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_LFG))
  {
    sendln(u"You are no longer Looking For Group."_s);
    REMOVE_BIT(player->toggles, Player::PLR_LFG);
  }
  else
  {
    sendln(u"You are now Looking For Group."_s);
    SET_BIT(player->toggles, Player::PLR_LFG);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_guide_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (!isSet(player->toggles, Player::PLR_GUIDE))
  {
    sendln(u"You must be assigned as a $BGuide$R by the gods before you can toggle it."_s);
    return ReturnValue::eFAILURE;
  }

  if (isSet(player->toggles, Player::PLR_GUIDE_TOG))
  {
    sendln(u"You have hidden your $B(Guide)$R tag."_s);
    REMOVE_BIT(player->toggles, Player::PLR_GUIDE_TOG);
  }
  else
  {
    sendln(u"You will now show your $B(Guide)$R tag."_s);
    SET_BIT(player->toggles, Player::PLR_GUIDE_TOG);
  }

  return ReturnValue::eSUCCESS;
}
ReturnValues Character::do_news_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_NEWS))
  {
    sendln(u"You now view news in an up-down fashion."_s);
    REMOVE_BIT(player->toggles, Player::PLR_NEWS);
  }
  else
  {
    sendln(u"You now view news in a down-up fashion.."_s);
    SET_BIT(player->toggles, Player::PLR_NEWS);
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_ascii_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_ASCII))
  {
    REMOVE_BIT(player->toggles, Player::PLR_ASCII);
    sendln(u"Cards are now displayed through ASCII."_s);
  }
  else
  {
    sendln(u"Cards are no longer dislayed through ASCII."_s);
    SET_BIT(player->toggles, Player::PLR_ASCII);
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_damage_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_DAMAGE))
  {
    REMOVE_BIT(player->toggles, Player::PLR_DAMAGE);
    sendln(u"Damage numbers will no longer be displayed in combat."_s);
  }
  else
  {
    sendln(u"Damage numbers will now be displayed in combat."_s);
    SET_BIT(player->toggles, Player::PLR_DAMAGE);
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_notax_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_NOTAX))
  {
    sendln(u"You will now be taxed on all your loot."_s);
    REMOVE_BIT(player->toggles, Player::PLR_NOTAX);
  }
  else
  {
    sendln(u"You will no longer be taxed."_s);
    SET_BIT(player->toggles, Player::PLR_NOTAX);
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_charmiejoin_toggle(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_CHARMIEJOIN))
  {
    sendln(u"Your followers will no longer automatically join you."_s);
    REMOVE_BIT(player->toggles, Player::PLR_CHARMIEJOIN);
  }
  else
  {
    sendln(u"Your followers will automatically aid you in battle."_s);
    SET_BIT(player->toggles, Player::PLR_CHARMIEJOIN);
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_autoeat(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_AUTOEAT))
  {
    sendln(u"You no longer automatically eat and drink."_s);
    REMOVE_BIT(player->toggles, Player::PLR_AUTOEAT);
  }
  else
  {
    sendln(u"You now automatically eat and drink when hungry and thirsty."_s);
    SET_BIT(player->toggles, Player::PLR_AUTOEAT);
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_anonymous(QStringList arguments, cmd_t cmd)
{
  if (level_ < 40)
  {
    sendln(u"You are too inexperienced to disguise your profession."_s);
    return ReturnValue::eSUCCESS;
  }
  if (isSet(player->toggles, Player::PLR_ANONYMOUS))
  {
    sendln(u"Your class and level information is now public."_s);
  }
  else
  {
    sendln(u"Your class and level information is now private."_s);
  }

  TOGGLE_BIT(player->toggles, Player::PLR_ANONYMOUS);
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_wimpy(QStringList arguments, cmd_t cmd)
{
  if (isSet(player->toggles, Player::PLR_WIMPY))
  {
    sendln(u"You are no longer a wimp....maybe."_s);
    REMOVE_BIT(player->toggles, Player::PLR_WIMPY);
    return ReturnValue::eFAILURE;
  }

  sendln(u"You are now an official wimp."_s);
  SET_BIT(player->toggles, Player::PLR_WIMPY);
  return ReturnValue::eSUCCESS;
}

// Remember that his is "no-pager".  So if it's set, we don't page
// If it's not set, we do.
ReturnValues Character::do_pager(QStringList arguments, cmd_t cmd)
{
  if (isSet(player->toggles, Player::PLR_PAGER))
  {
    sendln(u"You now page your strings in 24 line chunks."_s);
    REMOVE_BIT(player->toggles, Player::PLR_PAGER);
    return ReturnValue::eFAILURE;
  }

  sendln(u"You no longer page strings in 24 line chunks."_s);
  SET_BIT(player->toggles, Player::PLR_PAGER);
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_bard_song_toggle(QStringList arguments, cmd_t cmd)
{
  if (isSet(player->toggles, Player::PLR_BARD_SONG))
  {
    sendln(u"Bard singing now in verbose mode."_s);
    REMOVE_BIT(player->toggles, Player::PLR_BARD_SONG);
    return ReturnValue::eFAILURE;
  }

  sendln(u"Bard singing now in brief mode."_s);
  SET_BIT(player->toggles, Player::PLR_BARD_SONG);
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_nodupekeys_toggle(QStringList arguments, cmd_t cmd)
{
  if (isSet(player->toggles, Player::PLR_NODUPEKEYS))
  {
    sendln(u"You will attach duplicate keys to keyrings."_s);
    REMOVE_BIT(player->toggles, Player::PLR_NODUPEKEYS);
    return ReturnValue::eFAILURE;
  }

  sendln(u"You will not attach duplicate keys to keyrings."_s);
  SET_BIT(player->toggles, Player::PLR_NODUPEKEYS);
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_beep_set(QStringList arguments, cmd_t cmd)
{
  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  if (isSet(player->toggles, Player::PLR_BEEP))
  {
    REMOVE_BIT(player->toggles, Player::PLR_BEEP);
    sendln(u"\r\nTell is now silent."_s);
    return ReturnValue::eFAILURE;
  }

  SET_BIT(player->toggles, Player::PLR_BEEP);
  sendln(u"\r\nTell now beeps.\a"_s);
  return ReturnValue::eSUCCESS;
}

ReturnValues do_stand(CharacterPtr ch, QString argument, cmd_t cmd)
{
  switch (GET_POS(ch))
  {
  case position_t::STANDING:
  {
    act_to_character("You are already standing.", ch, 0, 0, 0);
  }
  break;
  case position_t::SITTING:
  {
    act_to_character("You stand up.", ch, 0, 0, 0);
    act_to_room("$n clambers on $s feet.", ch, 0, 0, INVIS_NULL);
    if (ch->fighting)
      ch->setPOSFighting();
    else
      ch->setStanding();
    ;
  }
  break;
  case position_t::RESTING:
  {
    act_to_character("You stop resting, and stand up.", ch, 0, 0, 0);
    act("$n stops resting, and clambers on $s feet.",
        ch, 0, 0, TO_ROOM, INVIS_NULL);
    ch->setStanding();
    ;
  }
  break;
  case position_t::SLEEPING:
  {
    act_to_character("You have to wake up first!", ch, 0, 0, 0);
  }
  break;
  case position_t::FIGHTING:
  {
    act("Do you not consider fighting as standing?",
        ch, 0, 0, TO_CHAR, 0);
  }
  break;
  default:
  {
    act_to_character("You stop floating around, and put your feet on the ground.", ch, 0, 0, 0);
    act_to_room("$n stops floating around, and puts $s feet on the ground.", ch, 0, 0, INVIS_NULL);
    ch->setStanding();
    ;
  }
  break;
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_sit(CharacterPtr ch, QString argument, cmd_t cmd)
{

  if (isSet(dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  switch (GET_POS(ch))
  {
  case position_t::STANDING:
  {
    act_to_character("You sit down.", ch, 0, 0, 0);
    act_to_room("$n sits down.", ch, 0, 0, INVIS_NULL);
    ch->setSitting();
  }
  break;
  case position_t::SITTING:
  {
    ch->sendln(u"You're sitting already."_s);
  }
  break;
  case position_t::RESTING:
  {
    act_to_character("You stop resting, and sit up.", ch, 0, 0, 0);
    act_to_room("$n stops resting.", ch, 0, 0, INVIS_NULL);
    ch->setSitting();
  }
  break;
  case position_t::SLEEPING:
  {
    act_to_character("You have to wake up first.", ch, 0, 0, 0);
  }
  break;
  case position_t::FIGHTING:
  {
    act("Sit down while fighting? are you MAD?",
        ch, 0, 0, TO_CHAR, 0);
  }
  break;
  default:
  {
    act("You stop floating around, and sit down.",
        ch, 0, 0, TO_CHAR, 0);
    act("$n stops floating around, and sits down.",
        ch, 0, 0, TO_ROOM, INVIS_NULL);
    ch->setSitting();
  }
  break;
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_rest(CharacterPtr ch, QString argument, cmd_t cmd)
{

  if (isSet(dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  switch (GET_POS(ch))
  {
  case position_t::STANDING:
  {
    act("You sit down and rest your tired bones.",
        ch, 0, 0, TO_CHAR, 0);
    act_to_room("$n sits down and rests.", ch, 0, 0, INVIS_NULL);
    ch->setResting();
  }
  break;
  case position_t::SITTING:
  {
    act_to_character("You rest your tired bones.", ch, 0, 0, 0);
    act_to_room("$n rests.", ch, 0, 0, INVIS_NULL);
    ch->setResting();
  }
  break;
  case position_t::RESTING:
  {
    act_to_character("You are already resting.", ch, 0, 0, 0);
  }
  break;
  case position_t::SLEEPING:
  {
    act_to_character("You have to wake up first.", ch, 0, 0, 0);
  }
  break;
  case position_t::FIGHTING:
  {
    act_to_character("Rest while fighting? are you MAD?", ch, 0, 0, 0);
  }
  break;
  default:
  {
    act_to_character("You stop floating around, and stop to rest your tired bones.", ch, 0, 0, 0);
    act_to_room("$n stops floating around, and rests.", ch, 0, 0, INVIS_NULL);
    ch->setSitting();
  }
  break;
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues do_sleep(CharacterPtr ch, QString argument, cmd_t cmd)
{
  affected_typePtr paf;
  if (isSet(dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_INSOMNIA))
  {
    ch->sendln(u"You are far too alert for that."_s);
    return ReturnValue::eFAILURE;
  }
  if (!isSet(dc_->world[ch->in_room]->room_flags_, SAFE))
    if (!check_make_camp(ch->in_room))
    {
      ch->sendln(u"Be careful sleeping out here!  This isn't a safe room, so people can steal your equipment while you sleep!"_s);
    }

  if ((paf = ch->affected_by_spell(SPELL_SLEEP)) &&
      paf->modifier == 1 && GET_POS(ch) != position_t::SLEEPING)
    paf->modifier = {};

  switch (GET_POS(ch))
  {
  case position_t::STANDING:
    ch->sendln(u"You lie down and go to sleep."_s);
    act_to_room("$n lies down and falls asleep.", ch, 0, 0, INVIS_NULL);
    ch->setSleeping();
    break;
  case position_t::SITTING:
  case position_t::RESTING:
    ch->sendln(u"You lay back and go to sleep."_s);
    act_to_room("$n lies back and falls asleep.", ch, 0, 0, INVIS_NULL);
    ch->setSleeping();
    break;
  case position_t::SLEEPING:
  {
    ch->sendln(u"You are already sound asleep."_s);
    return ReturnValue::eFAILURE; // so we don't set INTERNAL_SLEEPING
  }
  break;
  case position_t::FIGHTING:
  {
    ch->sendln(u"Sleep while fighting? Are you MAD?"_s);
    return ReturnValue::eFAILURE; // so we don't set INTERNAL_SLEEPING
  }
  break;
  default:
  {
    act_to_character("You stop floating around, and lie down to sleep.", ch, 0, 0, 0);
    act_to_room("$n stops floating around, and lie down to sleep.", ch, 0, 0, INVIS_NULL);
    ch->setSleeping();
  }
  break;
  }

  affected_type af;
  af.type = INTERNAL_SLEEPING;
  af.duration = {};
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  return ReturnValue::eSUCCESS;
}

ReturnValue Character::wake(CharacterPtr victim)
{
  if (!isSleeping())
  {
    sendln(u"You are already awake..."_s);
    return ReturnValue::eFAILURE;
  }

  auto af = affected_by_spell(SPELL_SLEEP);
  if (af && af->modifier == 1)
  {
    sendln(u"You can't wake up!"_s);
    return ReturnValue::eFAILURE;
  }

  sendln(u"You wake, and stand up."_s);
  act_to_room("$n awakens.", this, 0, 0, 0);
  setStanding();
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_wake(QStringList arguments, cmd_t cmd)
{
  CharacterPtr tmp_char = {};
  QString arg1 = arguments.value(0);
  if (arg1.isEmpty())
  {
    tmp_char = this;
  }
  else
  {
    tmp_char = get_char_room_vis(arg1);
  }

  if (!tmp_char)
  {
    sendln(u"You do not see that person here."_s);
    return ReturnValue::eFAILURE;
  }

  if (tmp_char != this)
  {
    if (isSleeping())
    {
      act_to_character("You can't wake people up if you are asleep yourself!", this, 0, 0, 0);
      return ReturnValue::eFAILURE;
    }
  }

  if (GET_POS(tmp_char) != position_t::SLEEPING)
  {
    if (tmp_char == this)
    {
      act_to_character("You are already awake.", this, 0, tmp_char, 0);
    }
    else
    {
      act_to_character("$N is already awake.", this, 0, tmp_char, 0);
    }
    return ReturnValue::eFAILURE;
  }

  auto af = tmp_char->affected_by_spell(SPELL_SLEEP);
  if (af && af->modifier == 1)
  {
    if (tmp_char == this)
    {
      act_to_character("You can not wake yourself up!", this, 0, tmp_char, 0);
    }
    else
    {
      act_to_character("You can not wake $M up!", this, 0, tmp_char, 0);
    }
    return ReturnValue::eFAILURE;
  }

  if (GET_POS(this) == position_t::FIGHTING)
  {
    if (ch->dc_->number(1, 100) > GET_DEX(this) && tmp_char != this)
    {
      act_to_character("You cannot meneuver yourself over to $M!", this, 0, tmp_char, 0);
      act_to_room("$n tries to move the flow of battle towards $N but is unable.", this, 0, tmp_char, 0);
      return ReturnValue::eSUCCESS;
    }

    act_to_character("You manage to give $M a swift kick in the ribs.", this, 0, tmp_char, 0);
    tmp_char->setStanding();
    act_to_room("$n awakens $N.", this, 0, tmp_char, NOTVICT);
    act_to_victim("$n wakes you up with a sharp kick to the ribs.  The sounds of battle ring in your ears.", this, 0, tmp_char, 0);
    affect_from_char(tmp_char, INTERNAL_SLEEPING);
    return ReturnValue::eSUCCESS;
  }

  tmp_char->setStanding();
  affect_from_char(tmp_char, INTERNAL_SLEEPING);
  if (tmp_char != this)
  {
    act_to_character("You wake $M up.", this, 0, tmp_char, 0);
    act_to_room("$n awakens $N.", this, 0, tmp_char, NOTVICT);
    act_to_victim("You are awakened by $n.", this, 0, tmp_char, 0);
  }
  else
  {
    act_to_character("You wake yourself up.", this, 0, 0, 0);
    act_to_room("$N awakens.", this, 0, tmp_char, NOTVICT);
  }

  return ReturnValue::eSUCCESS;
}

// global tag var
CharacterPtr tagged_person;

ReturnValues do_tag(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString name;
  CharacterPtr victim;

  one_argument(name, argument);

  if (name.isEmpty() || !(victim = ch->get_char_room_vis(name)))
  {
    ch->sendln(u"Tag who?"_s);
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eSUCCESS;
}

void CVoteData::DisplayVote(CharacterPtr ch)
{
  QString buf;
  QList<SVoteData>::iterator answer_it;
  qint32 i = 1;
  if (vote_question.isEmpty())
  {
    ch->send(u"\r\nSorry! There are no active votes right now!\r\n\r\n"_s);
    return;
  }
  ch->send(u"\r\n--Current Vote Infortmation--\r\nTo vote, type \"vote #\".\r\n"
           "Enter \"vote results\" to see the current voting demographics.\r\n\r\n");
  dc_strncpy(buf, qPrintable(vote_question), MAX_STRING_LENGTH);
  ch->send(buf);
  ch->send(u"\r\n"_s);
  for (answer_it = answers.begin(); answer_it != answers.end(); answer_it++)
    ch->send(u"%1: %2\r\n"_s.arg(i++, 2).arg(qPrintable(answer_it->answer)));
  ch->send(u"\r\n"_s);
}

void CVoteData::RemoveAnswer(CharacterPtr ch, quint32 answer)
{
  if (active)
  {
    ch->sendln(u"You have to end the current vote before you can remove answers."_s);
    return;
  }
  if (answers.isEmpty())
  {
    ch->sendln(u"That answer doesn't exist!"_s);
    return;
  }
  if (answer > answers.size())
  {
    ch->sendln(u"That answer doesn't exist!"_s);
    return;
  }
  QList<SVoteData>::iterator answer_it = answers.begin();
  answers.erase(answer_it + answer - 1); // need to offset by 1
  ch->sendln(u"Answer removed!"_s);
}

void CVoteData::StartVote(CharacterPtr ch)
{
  if (active)
  {
    ch->sendln(u"There is already an active vote, you can't start another"_s);
    return;
  }
  if (vote_question.isEmpty())
  {
    ch->sendln(u"You can't start a vote without a topic to vote on!"_s);
    return;
  }
  if (answers.isEmpty())
  {
    ch->sendln(u"You can't start a vote without any answers!"_s);
    return;
  }

  ch->sendln(u"$4**MAKE SURE YOU VOTESET CLEAR IF THIS IS A NEW VOTE!**$R"_s);
  send_info("\r\n##Attention! There is now a vote in progress!\r\n##Type Vote for more information!\r\n");

  active = true;
  OutToFile();
}

void CVoteData::EndVote(CharacterPtr ch)
{
  if (!active)
  {
    ch->sendln(u"Can't end a vote if there isn't one started."_s);
    return;
  }

  active = false;
  OutToFile();
  send_info("\r\n##The vote has ended! Type \"Vote Results\" to see the results!\r\n");
}

void CVoteData::AddAnswer(CharacterPtr ch, QString answer)
{
  if (active)
  {
    ch->sendln(u"You can't add answers during an active vote!"_s);
    return;
  }
  ch->sendln(u"Answer added."_s);
  SVoteData tmp;
  tmp.votes = {};
  tmp.answer = answer;
  answers.push_back(tmp);
}

bool CVoteData::HasVoted(CharacterPtr ch)
{
  return (ip_voted[ch->conn_->getPeerOriginalAddress().toString(qPrintable())] || char_voted[qPrintable(ch->name())]);
}

bool CVoteData::Vote(CharacterPtr ch, quint32 vote)
{
  if (!ch->conn_)
  {
    ch->sendln(u"Monsters don't get to vote!"_s);
    return false;
  }

  if (HasVoted(ch))
  {
    ch->sendln(u"You have already voted!"_s);
    return false;
  }

  if (vote > answers.size() || vote == 0)
  {
    ch->sendln(u"That answer doesn't exist."_s);
    return false;
  }

  ip_voted[ch->conn_->getPeerOriginalAddress().toString(qPrintable())] = true;
  char_voted[qPrintable(ch->name())] = true;
  total_votes++;
  answers.at(vote - 1).votes++;

  ch->sendln(u"Vote sent!"_s);
  OutToFile();
  return true;
}

void CVoteData::DisplayResults(CharacterPtr ch)
{
  if (active && ch->getLevel() > 39 && !ip_voted[ch->conn_->getPeerOriginalAddress().toString(qPrintable())] && ch->isMortalPlayer())
  {
    ch->sendln(u"Sorry, but you have to cast a vote before you can see the results."_s);
    return;
  }
  if (!total_votes)
  {
    ch->sendln(u"There hasn't been any votes to view the results of."_s);
    return;
  }

  QList<SVoteData>::iterator answer_it;
  ch->sendln(u"--Current Vote Results--"_s);
  ch->sendln(vote_question);
  for (answer_it = answers.begin(); answer_it != answers.end(); answer_it++)
  {
    if (ch->isMortalPlayer())
    {
      qint32 percent = (answer_it->votes * 100) / total_votes;
      ch->send(u"%3d\%: %s\r\n"_s.arg(percent).arg(qPrintable(answer_it->answer)));
    }
    else
      ch->send(u"%3d: %s\r\n"_s.arg(answer_it->votes).arg(qPrintable(answer_it->answer)));
  }
  ch->sendln(u""_s);
}

void CVoteData::Reset(CharacterPtr ch)
{
  if (active)
  {
    if (ch) // this can be called with null
      ch->sendln(u"Can't reset a vote while one is active."_s);
    return;
  }
  if (ch)
    ch->sendln(u"Ok. Vote cleared."_s);

  total_votes = {};
  vote_question.clear();
  answers.clear();
  ip_voted.clear();
  char_voted.clear();
}

void CVoteData::OutToFile()
{

  QTextStream the_file;

  the_file = fopen("vote_data", "w");

  if (!the_file)
  {
    dc_->logentry(u"Unable to open/create save file for vote data"_s, ANGEL,
                  DC::LogChannel::LOG_BUG);
    return;
  }

  dc_fprintf(the_file, "%d\n", active);
  dc_fprintf(the_file, "%d\n", total_votes);

  dc_fprintf(the_file, "%s\n", qPrintable(vote_question));

  dc_fprintf(the_file, "%d\n", answers.size());

  QList<SVoteData>::iterator answer_it;

  for (answer_it = answers.begin(); answer_it != answers.end(); answer_it++)
  {
    dc_fprintf(the_file, "%d\n", answer_it->votes);
    dc_fprintf(the_file, "%s\n", qPrintable(answer_it->answer));
  }

  QMap<QString, bool>::iterator ip_it;

  dc_fprintf(the_file, "%d\n", ip_voted.size());
  for (ip_it = ip_voted.begin(); ip_it != ip_voted.end(); ip_it++)
  {
    dc_fprintf(the_file, "%s\n", qPrintable(ip_it->first));
  }

  dc_fprintf(the_file, "%d\n", char_voted.size());
  for (ip_it = char_voted.begin(); ip_it != char_voted.end(); ip_it++)
  {
    dc_fprintf(the_file, "%s\n", qPrintable(ip_it->first));
  }
}

void CVoteData::SetQuestion(CharacterPtr ch, QString question)
{
  if (active)
  {
    ch->sendln(u"Can't change the question while the vote is active."_s);
    return;
  }
  ch->sendln(u"Ok. Question changed."_s);
  vote_question = question;
}

CVoteData::CVoteData()
    : active(false), total_votes(0)
{
  QString buf;
  QTextStream the_file = {};
  ;
  qint32 num = {};
  bool is_active = {};
  qint32 i = {};
  SVoteData tmp_vote_data;
  active = false;

  the_file = fopen("../lib/vote_data", "r");
  if (!the_file)
  {
    Reset(nullptr);
    return;
  }

  // save is_active for later
  fscanf(the_file, "%d\n", &is_active);

  if (feof(the_file))
  {

    Reset(nullptr);
    return;
  }

  fscanf(the_file, "%d\n", &num);
  total_votes = num;

  if (!fgets(buf, MAX_STRING_LENGTH, the_file))
  {

    Reset(nullptr);
    dc_->logentry(u"Error reading question from vote file."_s, 0, DC::LogChannel::LOG_MISC);
    return;
  }
  buf[dc_strlen(buf) - 1] = {};
  vote_question = buf;

  // ANSWERS
  fscanf(the_file, "%d\n", &i);
  for (; i > 0; i--)
  {
    fscanf(the_file, "%d\n", &num);
    if (!fgets(buf, MAX_STRING_LENGTH, the_file))
    {

      dc_->logentry(u"Error reading answers from vote file."_s, 0, DC::LogChannel::LOG_MISC);
      Reset(nullptr);
      return;
    }

    buf[dc_strlen(buf) - 1] = {};
    tmp_vote_data.votes = num;
    tmp_vote_data.answer = buf;
    answers.push_back(tmp_vote_data);
  }

  // IP ADDRESSES
  fscanf(the_file, "%d\n", &i);
  for (; i > 0; i--)
  {
    if (!fgets(buf, MAX_STRING_LENGTH, the_file))
    {

      dc_->logentry(u"Error reading ip addresses from vote file."_s, 0, DC::LogChannel::LOG_MISC);
      Reset(nullptr);
      return;
    }
    buf[dc_strlen(buf) - 1] = {};
    ip_voted[buf] = true;
  }

  // CHAR NAMES
  fscanf(the_file, "%d\n", &i);
  for (; i > 0; i--)
  {
    if (!fgets(buf, MAX_STRING_LENGTH, the_file))
    {

      dc_->logentry(u"Error reading character names from vote file."_s, 0, DC::LogChannel::LOG_MISC);
      Reset(nullptr);
      return;
    }
    buf[dc_strlen(buf) - 1] = {};
    char_voted[buf] = true;
  }

  // everything must have been correct, activate it here
  active = (bool)is_active;
}

CVoteData::~CVoteData()
{
}

ReturnValues do_vote(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString buf;
  qint32 vote;
  arg = one_argument(arg, buf);

  if (buf == u"results"_s)
  {
    dc_->DCVote.DisplayResults(ch);
    return ReturnValue::eSUCCESS;
  }

  if (!dc_->DCVote.IsActive())
  {
    ch->sendln(u"Sorry, there is nothing to vote on right now."_s);
    return ReturnValue::eSUCCESS;
  }
  if (!dc_strlen(buf))
  {
    dc_->DCVote.DisplayVote(ch);
    return ReturnValue::eSUCCESS;
  }

  if (ch->getLevel() < 40)
  {
    ch->sendln(u"Sorry, you must be at least level 40 to vote."_s);
    return ReturnValue::eSUCCESS;
  }

  vote = dc_atoi(buf);
  if (true == dc_->DCVote.Vote(ch, vote))
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_PLAYER, "%s just voted %d\r\n", qPrintable(ch->name()), vote);

  return ReturnValue::eSUCCESS;
}

ReturnValues do_random(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  qint32 i = {};

  if (isSet(dc_->world[ch->in_room]->room_flags_, QUIET))
  {
    ch->sendln(u"SHHHHHH!! Can't you see people are trying to read?"_s);
    return ReturnValue::eFAILURE;
  }

  i = dc_->number(1, 100);
  ch->send(u"You roll a random number between 1 and 100 resulting in: $B%1$R.\r\n"_s.arg(i));
  dc_sprintf(buf, "$n rolls a number between 1 and 100 resulting in: $B%u$R.\r\n", i);
  act_to_room(buf, ch, 0, 0, 0);
  return ReturnValue::eSUCCESS;
}
