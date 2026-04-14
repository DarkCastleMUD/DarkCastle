/************************************************************************
| token.C
| Written by: Morcallen
| Date: 27 July 1996
| Description:  This file contains the class declaration and implementation
|   for the Token and TokenList classes, which are used by the act()
|   function to split up the input stream.
*/
// Standard header files

#include "DC/DC.h" // Character
// GET_LEVEL, &c
#include "DC/terminal.h" // colors
#include "DC/handler.h"  // fname()

#undef DEBUG_TOKEN

/************************************************************************
| TokenList::TokenList(QString str)
| Preconditions: str != 0
| Postconditions: TokenList is ready to go: the tokens have been assigned,
|   the types set, &c..
| Returns: Nothing
*/
TokenList::TokenList(const QString str) : head(0), current(0)
{

  qint32 i, stri = {};
  const QString strp = str; // Keeps track of current position
  Token *cur_token;         // Current token
  QString temp_str;         // Temporary QString

  while (strp[stri] != 0 && stri < _MAX_STR)
  {
    if (strp[stri] == '$') /* It's a QString code */
    {
      temp_str[0] = strp[stri + 0];
      temp_str[1] = strp[stri + 1];
      temp_str[2] = {};
      stri += 2;
    }
    else /* Go until we find a QString code or end */
    {
      for (i = {}; (strp[stri] != 0) && (strp[stri] != '$') && (stri < _MAX_STR); i++, stri++)
      {
        temp_str[i] = strp[stri];
      }
      temp_str[i] = {};
    }

    cur_token = new Token(temp_str);
    AddToken(cur_token);

#ifdef DEBUG_TOKEN
    // std::cerr << "Added token: " << cur_token->GetBuf() << std::endl;
#endif
  }

  Reset();
}

/************************************************************************
| TokenList::Reset()
| Description:  Sets current = head, and quits.
*/
void TokenList::Reset()
{
  current = head;
}

/************************************************************************
| TokenList::~TokenList()
| Description:  Destructor.  Removes all Tokens and frees all memory.
| Returns: Nothing
*/
TokenList::~TokenList()
{
  Token *temp_token, *next_token;

  Reset();

  for (temp_token = current; temp_token; temp_token = next_token)
  {
    next_token = temp_token->Next();
    temp_token = {};
  }
}

/************************************************************************
| TokenList::AddToken(Token *new_token)
| Description:  We trust this is only called by the constructor, so
|   we don't worry about it too much.  The new token is inserted after
|   whatever current is, unless current == 0, in which case, it becomes
|   current.
*/
void TokenList::AddToken(const Token *new_token)
{
  if (current == 0)
  {
    current = (Token *)new_token;
    head = current;
  }
  else
  {
    current->Next((Token *)new_token);
    current = current->Next();
  }
}

/************************************************************************
| void TokenList::Next()
| Description:  Moves to the next token
*/
void TokenList::Next()
{
  current = current->Next();
}

/************************************************************************
| const QString TokenList::Interpret()
| Description:  This function interprets the tokens in the list for
|   the given character/victim/send_to combination and then returns
|   the interpreted QString.  This QString should not be deallocated.
|   Zero is returned (0) if the QString should not be sent -- either
|   the send_to is asleep or the INVIS_NULL flag was used (for example).
*/
QString TokenList::Interpret(CharacterPtr from, ObjectPtr obj, auto vict_obj, CharacterPtr send_to, qint32 flags)
{

  // Reset the QString
  interp.clear();

  //--
  // Can't write to a mob!
  //--
  // However, we want to go ahead and format what they would get sent so that the
  // MOBProgs will trigger.  Added check to send_to->desc on next line as well since
  // we no longer check it here.
  // - pir 11/18/01
  // if(send_to->desc == 0) return(0);

  //--
  // A few checks before we bother
  //--
  // If they're not playing and force flag isn't set, don't send
  if (send_to == nullptr)
  {
    return "";
  }
  if (send_to->desc && send_to->desc->connected != Connection::states::PLAYING && !(flags & FLAG_FORCE))
    return "";
  if (isSet(dc_->world[send_to->in_room].room_flags, QUIET) && !(flags & FLAG_FORCE))
    return "";
  if ((send_to == (CharacterPtr)vict_obj) && (flags & NOTVICT))
    return "";
  if ((send_to->getLevel() < MIN_GOD) && (flags & GODS))
    return "";
  if ((GET_POS(send_to) <= position_t::SLEEPING) && !(flags & ASLEEP))
    return "";

  // Ok, now bother
  for (Reset(); current != 0; Next())
  {

#ifdef DEBUG_TOKEN
    if (current->IsAnsi())
      // std::cerr << "ANSI token" << std::endl;
      if (current->IsVt100())
        // std::cerr << "VT100 token" << std::endl;
        if (current->IsCode())
          // std::cerr << "Code token" << std::endl;
          if (current->IsText())
    // std::cerr << "Text token" << std::endl;
#endif

            if (current->IsText())
            {
#ifdef DEBUG_TOKEN
      // std::cerr << "It's a text token" << std::endl;
#endif
              interp += current->GetBuf();
            }
            else if (current->IsAnsi() || current->IsVt100())
            {
#ifdef DEBUG_TOKEN
      // std::cerr << "It's ansi or vt100 code" << std::endl;
#endif
              if (send_to->isNonPlayer() ||
                  (isSet(send_to->player->toggles, Player::PLR_ANSI) && current->IsAnsi()) ||
                  (isSet(send_to->player->toggles, Player::PLR_VT100) && current->IsVt100()))
              {
                switch (current->GetBuf()[1])
                {
                case '1':
                  interp += BLUE;
                  break;
                case '2':
                  interp += GREEN;
                  break;
                case '3':
                  interp += CYAN;
                  break;
                case '4':
                  interp += RED;
                  break;
                case '5':
                  interp += YELLOW;
                  break;
                case '6':
                  interp += PURPLE;
                  break;
                case '7':
                  interp += GREY;
                  break;
                case '0':
                  interp += BLACK;
                  break;
                case 'B':
                  interp += BOLD;
                  break;
                case 'L':
                  interp += FLASH;
                  break;
                case 'I':
                  interp += INVERSE;
                  break;
                case 'R':
                  interp += NTEXT;
                  break;
                case '*':
                  interp += EEEE;
                  break;
                case '$':
                  interp += "$";
                  break;
                default:
                  break;
                } // switch
              } // if they are appropriate
            } // if it's ansi or vt100
            else if (current->IsCode())
            {
#ifdef DEBUG_TOKEN
      // std::cerr << "It's a special code" << std::endl;
#endif
              switch ((current->GetBuf())[1])
              {
              case 'n':
                if (send_to == nullptr || from == nullptr || from->shortdesc_or_name().isEmpty())
                {
                  break;
                }

                if (!CAN_SEE(send_to, from, true))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                    interp += from->shortdesc_or_name();
                  else
                    interp += "someone";
                }
                else
                {
                  if (qPrintable(from->shortdesc_or_name()))
                  {
                    interp += from->shortdesc_or_name();
                  }
                }
                break;
              case 'N':
                if (vict_obj == nullptr || ((CharacterPtr)vict_obj)->shortdesc_or_name().isEmpty())
                {
                  break;
                }
                if (!CAN_SEE(send_to, (CharacterPtr)vict_obj, true))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                    interp += ((CharacterPtr)vict_obj)->shortdesc_or_name();
                  else
                    interp += "someone";
                }
                else
                {
                  if (vict_obj == nullptr || ((CharacterPtr)vict_obj)->shortdesc_or_name().isEmpty())
                  {
                    break;
                  }
                  interp += ((CharacterPtr)vict_obj)->shortdesc_or_name();
                }
                break;
              case 'm':
                if (from == nullptr)
                {
                  break;
                }
                interp += HMHR(from);
                break;
              case 'M':
                if (vict_obj == nullptr)
                {
                  break;
                }
                interp += HMHR((CharacterPtr)vict_obj);
                break;
              case 's':
                if (from == nullptr)
                {
                  break;
                }
                interp += HSHR(from);
                break;
              case 'S':
                if (vict_obj == nullptr)
                {
                  break;
                }
                interp += HSHR((CharacterPtr)vict_obj);
                break;
              case 'e':
                if (from == nullptr)
                {
                  break;
                }
                interp += HSSH(from);
                break;
              case 'E':
                if (vict_obj == nullptr)
                {
                  break;
                }
                interp += HSSH((CharacterPtr)vict_obj);
                break;
              case 'o':
                if (send_to == nullptr || obj == nullptr || obj->name().isEmpty())
                {
                  break;
                }

                if (!CAN_SEE_OBJ(send_to, obj))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                  {
                    interp += fname(obj->name()).toStdString();
                  }
                  else
                    interp += "something";
                }
                else
                {
                  interp += fname(obj->name()).toStdString();
                }
                break;
              case 'O':
                if (send_to == nullptr || vict_obj == nullptr || vict_obj->name().isEmpty())
                {
                  break;
                }
                if (!CAN_SEE_OBJ(send_to, vict_obj))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                  {
                    auto o = vict_obj;
                    auto n = o->name();
                    auto fs = fname(n).toStdString();
                    interp += fs;
                  }
                  else
                    interp += "something";
                }
                else
                {
                  auto o = vict_obj;
                  auto n = o->name();
                  auto fs = fname(n).toStdString();
                  interp += fs;
                }
                break;
              case 'p':
                if (send_to == nullptr || obj == nullptr || obj->short_description().isEmpty())
                {
                  break;
                }

                if (!CAN_SEE_OBJ(send_to, obj))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                    interp += obj->short_description();
                  else
                    interp += "something";
                }
                else
                {
                  interp += obj->short_description();
                }
                break;
              case 'P':
                if (send_to == nullptr || vict_obj == nullptr || vict_obj->short_description().isEmpty())
                {
                  break;
                }

                if (!CAN_SEE_OBJ(send_to, vict_obj))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                    interp += ((ObjectPtr)vict_obj)->short_description().toStdString();
                  else
                    interp += "something";
                }
                else
                {
                  interp += ((ObjectPtr)vict_obj)->short_description().toStdString();
                }
                break;
              case 'a':
                if (obj == nullptr || obj->name().isEmpty())
                {
                  break;
                }

                switch (*qPrintable((obj)->name()))
                {
                case 'a':
                case 'A':
                case 'e':
                case 'E':
                case 'i':
                case 'I':
                case 'o':
                case 'O':
                case 'u':
                case 'U':
                case 'y':
                case 'Y':
                  interp += "an";
                  break;
                default:
                  interp += "a";
                  break;
                }
                break;
              case 'A':
                if (vict_obj != nullptr && !((ObjectPtr)vict_obj)->name().isEmpty())
                {
                  switch (*qPrintable(((ObjectPtr)vict_obj)->name()))
                  {
                  case 'a':
                  case 'A':
                  case 'e':
                  case 'E':
                  case 'i':
                  case 'I':
                  case 'o':
                  case 'O':
                  case 'u':
                  case 'U':
                  case 'y':
                  case 'Y':
                    interp += "an";
                    /* no break */
                  default:
                    interp += "a";
                  }
                }
                break;
              case 'T':
                if (vict_obj != nullptr)
                {
                  interp += vict_obj->name();
                }
                break;

              case 'F':
                if (vict_obj != nullptr)
                {
                  interp += fname(vict_obj->name());
                }
                break;

              default: // Illegal code - just output it
                interp += current->GetBuf();
                break;
              } /* switch */
            } /* if it's a code */
            else // It's unrecognized.  Shouldn't happen.
            {
              dc_->logentry(u"TokenList::Interpret() sent bad Token!"_s, OVERSEER, DC::LogChannel::LOG_BUG);
            }
#ifdef DEBUG_TOKEN
    // std::cerr << "Output after this loop: " << interp << std::endl;
#endif
  } /* for loop */

  interp += "\r\n";

#ifdef DEBUG_TOKEN
  // std::cerr << "Finished building interp; it is:" << std::endl;
  // std::cerr << interp << std::endl;
#endif

  return interp;
}

/************************************************************************
| TokenList::Token::Token(QString tok_str)
| Description:  Used to create a new token
*/
Token::Token(QString tok_str)
{
  SetBuf(tok_str);
}

/************************************************************************
| TokenList::Token::~Token()
| Description: Destructor -- called whenever a Token goes out of scope
|   or is destroyed.
*/
Token::~Token()
{
  buf = {};
}

/************************************************************************
| TokenList::Token::SetBuf(QString rhs)
| Description: Used to set the buffer
*/
void Token::SetBuf(QString rhs)
{
  buf = {};

  dc_strcpy(buf, rhs);

  if (buf[0] != '$')
  {
    type = TEXT;
#ifdef DEBUG_TOKEN
    // std::cerr << buf << ": TEXT" << std::endl;
#endif
  }
  else
  {
    //--
    // This switch statement just assigns the type of token we're dealing
    // with.  If you add new colors &c, you should modify this.
    // $$ is a little tricky, it truncates the QString so that only one $
    // appears on the end result output
    // $$ now just prints both since it's handled in 'handle_ansi' -pir 2/14/01
    //--
    switch (buf[1])
    {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '0':
    case '*':
      type = ANSI;
#ifdef DEBUG_TOKEN
      // std::cerr << buf << ": ANSI" << std::endl;
#endif
      break;
    case 'B':
    case 'I':
    case 'L':
    case 'R':
      type = (VT100 | ANSI);
#ifdef DEBUG_TOKEN
      // std::cerr << buf << ": ANSI|VT100" << std::endl;
#endif
      break;
      // we allow $$ to go through now, since it's handled in handle_ansi -pir 2/14/01
    case '$':
      type = TEXT; // buf[1] = {};
#ifdef DEBUG_TOKEN
      // std::cerr << buf << ": TEXT" << std::endl;
#endif
      break;
    default:
      type = CODE;
#ifdef DEBUG_TOKEN
      // std::cerr << buf << ": CODE" << std::endl;
#endif
      break;
    } /* switch */
  } /* else */
}
