/************************************************************************
| token.C
| Written by: Morcallen
| Date: 27 July 1996
| Description:  This file contains the class declaration and implementation
|   for the Token and TokenList classes, which are used by the act()
|   function to split up the input stream.
*/
// Standard header files

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>

#include "DC/obj.h"
#include "DC/db.h"
#include "DC/room.h"
#include "DC/character.h" // Character
#include "DC/DC.h"        // Object
#include "DC/utility.h"   // GET_SHORT, GET_LEVEL, &c
#include "DC/terminal.h"  // colors
#include "DC/act.h"       // act flags
#include "DC/player.h"    // Player::PLR_ANSI Player::PLR_VT100
#include "DC/handler.h"   // fname()
#include "DC/token.h"     // fname()
#include "DC/connect.h"

#undef DEBUG_TOKEN

/************************************************************************
| TokenList::TokenList(char *str)
| Preconditions: str != 0
| Postconditions: TokenList is ready to go: the tokens have been assigned,
|   the types set, &c..
| Returns: Nothing
*/
TokenList::TokenList(const char *str) : head(0), current(0)
{

  int i, stri = 0;
  const char *strp = str;  // Keeps track of current position
  Token *cur_token;        // Current token
  char temp_str[_MAX_STR]; // Temporary std::string

  while (strp[stri] != 0 && stri < _MAX_STR)
  {
    if (strp[stri] == '$') /* It's a std::string code */
    {
      temp_str[0] = strp[stri + 0];
      temp_str[1] = strp[stri + 1];
      temp_str[2] = 0;
      stri += 2;
    }
    else /* Go until we find a std::string code or end */
    {
      for (i = 0; (strp[stri] != 0) && (strp[stri] != '$') && (stri < _MAX_STR); i++, stri++)
      {
        temp_str[i] = strp[stri];
      }
      temp_str[i] = 0;
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
    delete temp_token;
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
| const char * TokenList::Interpret()
| Description:  This function interprets the tokens in the list for
|   the given character/victim/send_to combination and then returns
|   the interpreted std::string.  This std::string should not be deallocated.
|   Zero is returned (0) if the std::string should not be sent -- either
|   the send_to is asleep or the INVIS_NULL flag was used (for example).
*/
std::string TokenList::Interpret(Character *from, Object *obj, void *vict_obj, Character *send_to, int flags)
{

  // Reset the std::string
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
  if (isSet(DC::getInstance()->world[send_to->in_room].room_flags, QUIET) && !(flags & FLAG_FORCE))
    return "";
  if ((send_to == (Character *)vict_obj) && (flags & NOTVICT))
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
              if (IS_NPC(send_to) ||
                  (send_to->isPlayer() &&
                   ((isSet(send_to->player->toggles, Player::PLR_ANSI) && current->IsAnsi()) ||
                    (isSet(send_to->player->toggles, Player::PLR_VT100) && current->IsVt100()))))
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
                if (send_to == nullptr || from == nullptr || GET_SHORT(from) == nullptr)
                {
                  break;
                }

                if (!CAN_SEE(send_to, from, true))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                    interp += GET_SHORT(from);
                  else
                    interp += "someone";
                }
                else
                {
                  if (GET_SHORT(from))
                  {
                    interp += GET_SHORT(from);
                  }
                }
                break;
              case 'N':
                if (vict_obj == nullptr || GET_SHORT((Character *)vict_obj) == nullptr)
                {
                  break;
                }
                if (!CAN_SEE(send_to, (Character *)vict_obj, true))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                    interp += GET_SHORT((Character *)vict_obj);
                  else
                    interp += "someone";
                }
                else
                {
                  if (vict_obj == nullptr || GET_SHORT((Character *)vict_obj) == nullptr)
                  {
                    break;
                  }
                  interp += GET_SHORT((Character *)vict_obj);
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
                interp += HMHR((Character *)vict_obj);
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
                interp += HSHR((Character *)vict_obj);
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
                interp += HSSH((Character *)vict_obj);
                break;
              case 'o':
                if (send_to == nullptr || obj == nullptr || obj->Name().isEmpty())
                {
                  break;
                }

                if (!CAN_SEE_OBJ(send_to, obj))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                  {
                    interp += fname(obj->Name()).toStdString();
                  }
                  else
                    interp += "something";
                }
                else
                {
                  interp += fname(obj->Name()).toStdString();
                }
                break;
              case 'O':
                if (send_to == nullptr || vict_obj == nullptr || ((Object *)vict_obj)->Name().isEmpty())
                {
                  break;
                }
                if (!CAN_SEE_OBJ(send_to, (Object *)vict_obj))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                  {
                    auto o = (Object *)vict_obj;
                    auto n = o->Name();
                    auto fs = fname(n).toStdString();
                    interp += fs;
                  }
                  else
                    interp += "something";
                }
                else
                {
                  auto o = (Object *)vict_obj;
                  auto n = o->Name();
                  auto fs = fname(n).toStdString();
                  interp += fs;
                }
                break;
              case 'p':
                if (send_to == nullptr || obj == nullptr || obj->short_description == nullptr)
                {
                  break;
                }

                if (!CAN_SEE_OBJ(send_to, obj))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                    interp += obj->short_description;
                  else
                    interp += "something";
                }
                else
                {
                  interp += obj->short_description;
                }
                break;
              case 'P':
                if (send_to == nullptr || vict_obj == nullptr || ((Object *)vict_obj)->short_description == nullptr)
                {
                  break;
                }

                if (!CAN_SEE_OBJ(send_to, (Object *)vict_obj))
                {
                  if (flags & INVIS_NULL)
                    return {};
                  else if (flags & INVIS_VISIBLE)
                    interp += ((Object *)vict_obj)->short_description;
                  else
                    interp += "something";
                }
                else
                {
                  interp += ((Object *)vict_obj)->short_description;
                }
                break;
              case 'a':
                if (obj == nullptr || obj->Name().isEmpty())
                {
                  break;
                }

                switch (*qPrintable((obj)->Name()))
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
                if (vict_obj != nullptr && !((Object *)vict_obj)->Name().isEmpty())
                {
                  switch (*qPrintable(((Object *)vict_obj)->Name()))
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
                  interp += (char *)vict_obj;
                }
                break;

              case 'F':
                if (vict_obj != nullptr)
                {
                  interp += fname(QString((char *)vict_obj)).toStdString();
                }
                break;

              default: // Illegal code - just output it
                interp += current->GetBuf();
                break;
              } /* switch */
            } /* if it's a code */
            else // It's unrecognized.  Shouldn't happen.
            {
              DC::getInstance()->logentry(QStringLiteral("TokenList::Interpret() sent bad Token!"), OVERSEER, DC::LogChannel::LOG_BUG);
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
| TokenList::Token::Token(char *tok_str)
| Description:  Used to create a new token
*/
Token::Token(char *tok_str) : buf(0), type(0), next(0)
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
  if (buf)
    delete[] buf;
  buf = 0;
}

/************************************************************************
| TokenList::Token::SetBuf(char *rhs)
| Description: Used to set the buffer
*/
void Token::SetBuf(char *rhs)
{
  if (buf)
    delete[] buf;
  buf = new char[strlen(rhs) + 1];

  strcpy(buf, rhs);

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
    // $$ is a little tricky, it truncates the std::string so that only one $
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
      type = TEXT; // buf[1] = 0;
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
