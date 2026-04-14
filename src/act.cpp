/************************************************************************
| act.C
| Description:  This file contains the classes and methods used to make
|  the act() function work.
*/
#include "DC/DC.h"

extern bool MOBtrigger;

constexpr auto TO_ROOM = 0;           // Everyone in ch's room except ch
constexpr auto TO_VICT = 1;           // Just vict_obj
constexpr auto TO_CHAR = 2;           // Just ch
constexpr auto TO_ZONE = 3;           // Everyone in ch's zone except ch
constexpr auto TO_WORLD = 4;          // Everyone in the world except ch
constexpr auto TO_GROUP = 5;          // Everyone in the ch's group except ch
constexpr auto TO_ROOM_NOT_GROUP = 6; // Everyone in ch's room except ch's group or ch

act_return act_to_room(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags)
{
  return act_to_room(str, ch, obj, vict_obj, TO_ROOM, flags);
}

act_return act_to_victim(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags)
{
  return act_to_room(str, ch, obj, vict_obj, TO_VICT, flags);
}

act_return act_to_character(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags)
{
  return act_to_room(str, ch, obj, vict_obj, TO_CHAR, flags);
}

act_return act_to_zone(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags)
{
  return act_to_room(str, ch, obj, vict_obj, TO_ZONE, flags);
}

act_return act_to_world(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags)
{
  return act_to_room(str, ch, obj, vict_obj, TO_WORLD, flags);
}

act_return act_to_group(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags)
{
  return act_to_room(str, ch, obj, vict_obj, TO_GROUP, flags);
}

act_return act_to_room_not_group(QString str, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint16 flags)
{
  return act_to_room(str, ch, obj, vict_obj, TO_ROOM_NOT_GROUP, flags);
}

act_return act(
    QString str,        // Buffer
    CharacterPtr ch,    // Character from
    ObjectPtr obj,      // Object
    auto vict_obj,      // Victim object
    qint16 destination, // Destination flags
    qint16 flags)
{
  ConnectionPtr i;
  qint32 retval = {};

  send_tokens_return st_return;
  st_return.str = QString();
  st_return.retval = {};

  act_return ar;
  ar.str = QString();
  ar.retval = {};

  TokenList tokens(str);

  // This shouldn't happen
  if (ch == 0)
  {
    DC::getInstance()->logentry(u"Error in act(), character equal to 0"_s, OVERSEER, DC::LogChannel::LOG_BUG);
    ar.retval = ReturnValue::eFAILURE;
    return ar;
  }

  if ((IS_AFFECTED(ch, AFF_HIDE) || ISSET(ch->affected_by, AFF_FOREST_MELD)) && (destination != TO_CHAR) && (destination != TO_GROUP) && !(flags & GODS) && !(flags & STAYHIDE))
  {
    REMBIT(ch->affected_by, AFF_HIDE);
    affect_from_char(ch, SPELL_FOREST_MELD);
  }

  if (destination == TO_VICT)
  {
    st_return = send_tokens(tokens, ch, obj, vict_obj, flags, vict_obj);
    retval |= st_return.retval;
    ar.str = st_return.str;
    ar.retval = retval;
  }
  else if (destination == TO_CHAR)
  {
    if (!isSet(flags, BARDSONG) || ch->player == nullptr || !isSet(ch->player->toggles, Player::PLR_BARD_SONG))
    {
      st_return = send_tokens(tokens, ch, obj, vict_obj, flags, ch);
      retval |= st_return.retval;
      ar.str = st_return.str;
      ar.retval = retval;
    }
  }
  else if (destination == TO_ROOM || destination == TO_GROUP || destination == TO_ROOM_NOT_GROUP)
  {
    if (ch->in_room >= 1)
    {
      for (const auto &tmp_char : ch->dc_->world[ch->in_room].people_)
      {
        // If they're not really playing, and no force flag, don't send
        if (tmp_char == ch)
          continue;
        if (destination == TO_GROUP && !ARE_GROUPED(tmp_char, ch))
          continue;
        if (destination == TO_ROOM_NOT_GROUP && ARE_GROUPED(tmp_char, ch))
        {
          continue;
        }
        if (tmp_char->getPosition() > position_t::SLEEPING || isSet(flags, ASLEEP))
        {
          if (!isSet(flags, BARDSONG) || tmp_char->player == nullptr || !isSet(tmp_char->player->toggles, Player::PLR_BARD_SONG))
          {
            st_return = send_tokens(tokens, ch, obj, vict_obj, flags, tmp_char);
            retval |= st_return.retval;
            ar.str = st_return.str;
            ar.retval = retval;
          }
        }
      }
    }
  }
  // TO_ZONE, TO_WORLD
  else
  {
    if (destination != TO_ZONE && destination != TO_WORLD)
    {
      ch->dc_->logentry(u"Error in act(), invalid value sent as 'destination'"_s, OVERSEER, DC::LogChannel::LOG_BUG);
      ar.retval = ReturnValue::eFAILURE;
      return ar;
    }
    for (auto &conn : ch->dc_->connections_)
    {
      // Dropped link or they're not really playing and no force flag, don't send.
      if (!conn->character || conn->character == ch)
        continue;
      if (conn->character->in_room == DC::NOWHERE || ch->in_room == DC::NOWHERE)
        continue;
      if ((destination == TO_ZONE) && ch->dc_->world[conn->character->in_room].zone != ch->dc_->world[ch->in_room].zone)
        continue;
      st_return = send_tokens(tokens, ch, obj, vict_obj, flags, conn->character);
      retval |= st_return.retval;
      ar.str = st_return.str;
      ar.retval = retval;
    }
  }

  return ar;
}

/************************************************************************
| void send_message()
| Description:  This function just sends the message to the character,
|   with no interpretation and no checks.
*/

void send_message(QString str, CharacterPtr to)
{
  if (str.isEmpty())
    return;

  if (!to)
    return;

  if (!to->desc)
    return;

  write_to_output(str, to->desc);
}

send_tokens_return send_tokens(TokenList &tokens, CharacterPtr ch, ObjectPtr obj, auto vict_obj, qint32 flags, CharacterPtr to)
{
  qint32 retval = {};
  QString buf = tokens.Interpret(ch, obj, vict_obj, to, flags);

  // Uppercase first letter of sentence.
  if (buf.isEmpty() == false && buf[0] != 0)
  {
    buf[0] = buf[0].toUpper();
  }
  send_message(buf, to);

  if (MOBtrigger && buf.isEmpty() == false)
    retval |= mprog_act_trigger(buf, to, ch, obj, vict_obj);
  if (MOBtrigger && buf.isEmpty() == false)
    retval |= ch->oprog_act_trigger(buf);

  MOBtrigger = true;
  if (buf.isEmpty())
  {
    send_tokens_return str;
    str.str = QString();
    str.retval = retval;
    return str;
  }

  if (buf.endsWith("\r\n") || buf.endsWith("\r\n"))
  {
    buf = buf.chopped(2);
  }
  if (buf.endsWith("\n") || buf.endsWith("\r"))
  {
    buf = buf.chopped(1);
  }

  send_tokens_return str;
  str.str = buf;
  str.retval = retval;
  return str;
}

// The End
