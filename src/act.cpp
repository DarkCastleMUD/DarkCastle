/************************************************************************
| act.C
| Description:  This file contains the classes and methods used to make
|  the act() function work.
*/
#include <cctype>
#include <cstring>
#include <cctype>

#include <string>

#include "DC/DC.h"
#include "DC/character.h"
#include "DC/comm.h"
#include "DC/db.h"
#include "DC/room.h"
#include "DC/utility.h"
#include "DC/player.h"
#include "DC/terminal.h"
#include "DC/handler.h"
#include "DC/connect.h"
#include "DC/act.h"
#include "DC/mobile.h"
#include "DC/token.h"
#include "DC/spells.h"
#include "DC/returnvals.h"

extern bool MOBtrigger;

act_return act(QString str, Character *ch, Object *obj, void *vict_obj, int16_t destination, int16_t flags)
{
  return act(str.toStdString().c_str(), ch, obj, vict_obj, destination, flags);
}

act_return act(const std::string &str, Character *ch, Object *obj, void *vict_obj, int16_t destination, int16_t flags)
{
  return act(str.c_str(), ch, obj, vict_obj, destination, flags);
}

act_return act(
    const char *str,     // Buffer
    Character *ch,       // Character from
    Object *obj,         // Object
    void *vict_obj,      // Victim object
    int16_t destination, // Destination flags
    int16_t flags        // Optional flags
)
{
  class Connection *i;
  int retval = 0;
  TokenList *tokens;

  send_tokens_return st_return;
  st_return.str = QString();
  st_return.retval = 0;

  act_return ar;
  ar.str = QString();
  ar.retval = 0;

  tokens = new TokenList(str);

  // This shouldn't happen
  if (ch == 0)
  {
    logentry(QStringLiteral("Error in act(), character equal to 0"), OVERSEER, DC::LogChannel::LOG_BUG);
    delete tokens;
    ar.retval = eFAILURE;
    return ar;
  }

  if (
      (IS_AFFECTED(ch, AFF_HIDE) || ISSET(ch->affected_by, AFF_FOREST_MELD)) && (destination != TO_CHAR) && (destination != TO_GROUP) && !(flags & GODS) && !(flags & STAYHIDE))
  {
    REMBIT(ch->affected_by, AFF_HIDE);
    affect_from_char(ch, SPELL_FOREST_MELD);
  }

  if (destination == TO_VICT)
  {
    st_return = send_tokens(tokens, ch, obj, vict_obj, flags, (Character *)vict_obj);
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
    Character *tmp_char, *next_tmp_char;
    if (ch->in_room >= 1)
    {
      for (tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = next_tmp_char)
      {
        next_tmp_char = tmp_char->next_in_room;
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
      logentry(QStringLiteral("Error in act(), invalid value sent as 'destination'"), OVERSEER, DC::LogChannel::LOG_BUG);
      delete tokens;
      ar.retval = eFAILURE;
      return ar;
    }
    for (i = DC::getInstance()->descriptor_list; i; i = i->next)
    {
      // Dropped link or they're not really playing and no force flag, don't send.
      if (!i->character || i->character == ch)
        continue;
      if (i->character->in_room == DC::NOWHERE || ch->in_room == DC::NOWHERE)
        continue;
      if ((destination == TO_ZONE) && DC::getInstance()->world[i->character->in_room].zone != DC::getInstance()->world[ch->in_room].zone)
        continue;
      st_return = send_tokens(tokens, ch, obj, vict_obj, flags, i->character);
      retval |= st_return.retval;
      ar.str = st_return.str;
      ar.retval = retval;
    }
  }

  delete tokens;
  return ar;
}

/************************************************************************
| void send_message()
| Description:  This function just sends the message to the character,
|   with no interpretation and no checks.
*/

void send_message(QString str, Character *to)
{
  if (str.isEmpty())
    return;

  if (!to)
    return;

  if (!to->desc)
    return;

  SEND_TO_Q(str, to->desc);
}

void send_message(const char *str, Character *to)
{
  // This will happen when a token shouldn't be interpreted
  if (str == 0)
    return;
  if (!to)
    return;
  if (!to->desc)
    return;

  SEND_TO_Q(str, to->desc);
}

void send_message(std::string str, Character *to)
{
  return send_message(str.c_str(), to);
}

send_tokens_return send_tokens(TokenList *tokens, Character *ch, Object *obj, void *vict_obj, int flags, Character *to)
{
  int retval = 0;
  QString buf = tokens->Interpret(ch, obj, vict_obj, to, flags).c_str();

  // Uppercase first letter of sentence.
  if (buf.isEmpty() == false && buf[0] != 0)
  {
    buf[0] = buf[0].toUpper();
  }
  send_message(buf, to);

  if (MOBtrigger && buf.isEmpty() == false)
    retval |= mprog_act_trigger(buf.toStdString(), to, ch, obj, vict_obj);
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

  if (buf.endsWith("\r\n") || buf.endsWith("\n\r"))
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
