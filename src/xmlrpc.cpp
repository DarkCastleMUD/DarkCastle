#include <iostream>
#include <unistd.h>
#include "DC/dc_xmlrpc.h"
#include <cstring>

#include "DC/character.h"
#include "DC/handler.h"
#include "DC/connect.h"

#ifdef __CYGWIN__
#include <crypt.h>
#endif

using namespace XmlRpc;

class common
{
protected:
  int authenticated(XmlRpcValue &params)
  {
    std::string &login = params[0];
    std::string &password = params[1];
    char buffer[MAX_STRING_LENGTH];

    strncpy(buffer, login.c_str(), MAX_STRING_LENGTH);
    Character *ch = get_all_pc(buffer);

    if (ch && ch->isImmortalPlayer() && ch->player->pwd &&
        !strncmp(crypt(password.c_str(), ch->player->pwd),
                 ch->player->pwd, (PASSWORD_LEN)))
    {
      return 1;
    }

    return 0;
  }
};

class login : public XmlRpcServerMethod, public common
{
public:
  login(XmlRpcServer *s) : XmlRpcServerMethod("login", s) {}

  void execute(XmlRpcValue &params, XmlRpcValue &result)
  {
    if (!authenticated(params))
    {
      result = "unauthorized";
    }
    else
    {
      result = "authorized";
    }

    return;
  }
};

class editor : public XmlRpcServerMethod, public common
{
public:
  editor(XmlRpcServer *s) : XmlRpcServerMethod("editor", s) {}

  void execute(XmlRpcValue &params, XmlRpcValue &result)
  {
    if (!authenticated(params))
    {
      result = "unauthorized";
      return;
    }

    std::string &login = params[0];
    char buffer[MAX_STRING_LENGTH];

    strncpy(buffer, login.c_str(), MAX_STRING_LENGTH);
    Character *ch = get_all_pc(buffer);
    if (ch == 0)
    {
      result = "player not found in game";
      return;
    }

    if (IS_PC(ch) && !isSet(ch->player->toggles, Player::PLR_EDITOR_WEB))
    {
      result = "plr_editor_dc";
      return;
    }

    std::string &contents = params[2];

    // Remove \r characters from web input
    unsigned int index = 0;
    while ((index = contents.find('\r', index)) != std::string::npos)
    {
      contents.erase(index, 1);
    }

    if (ch->isImmortalPlayer() && ch->desc->strnew)
    {
      switch (ch->desc->web_connected)
      {
      case Connection::states::EDIT_MPROG:
        if (!contents.empty())
        {
          if (!(*ch->desc->strnew))
          {
            if ((int)contents.size() > ch->desc->max_str)
            {
              SEND_TO_Q("String too long - Truncated.\r\n", ch->desc);
              contents[ch->desc->max_str] = '\0';
            }
            *ch->desc->strnew = new char[contents.size() + 5];
            strcpy(*ch->desc->strnew, contents.c_str());
          }
          else
          {
            auto buffer = new char[strlen(*ch->desc->strnew) + contents.size() + 5];
            strcpy(buffer, *ch->desc->strnew);
            delete[] *ch->desc->strnew;
            *ch->desc->strnew = buffer;
            strcat(*ch->desc->strnew, contents.c_str());
          }
          ch->desc->web_connected = Connection::states::PLAYING;
          result = *(ch->desc->strnew);
          ch->desc->strnew = 0;
          ch->sendln("Entry submitted.");
        }
        else
        {
          std::string str_result = *(ch->desc->strnew);

          // Remove \r characters before sending this to the web form
          index = 0;
          while ((index = str_result.find('\r', index)) != std::string::npos)
          {
            str_result.erase(index, 1);
          }
          result = str_result.c_str();
        }
        break;
      default:
        break;
      }
    }
    else
    {
      ch->desc->web_connected = Connection::states::PLAYING;
    }
  }
};

class get_editor_type : public XmlRpcServerMethod, public common
{
public:
  get_editor_type(XmlRpcServer *s) : XmlRpcServerMethod("get_editor_type", s) {}

  void execute(XmlRpcValue &params, XmlRpcValue &result)
  {
    if (!authenticated(params))
    {
      result = "unauthorized";
      return;
    }

    std::string &login = params[0];
    char buffer[MAX_STRING_LENGTH];

    strncpy(buffer, login.c_str(), MAX_STRING_LENGTH);
    Character *ch = get_all_pc(buffer);
    if (ch == 0)
    {
      result = "player not found in game";
      return;
    }

    if (IS_PC(ch) && !isSet(ch->player->toggles, Player::PLR_EDITOR_WEB))
    {
      result = "plr_editor_dc";
      return;
    }

    if (ch->isImmortalPlayer())
    {
      switch (ch->desc->web_connected)
      {
      case Connection::states::EDIT_MPROG:
        result = "Obj or Mob program";
        break;
      default:
        result = "unknown";
        break;
      }
    }
  }
};

XmlRpcServer *xmlrpc_init(int xmlrpc_port)
{
  XmlRpcServer *s = new XmlRpcServer;
  new login(s);
  new editor(s);
  new get_editor_type(s);

  while (!s->bindAndListen(xmlrpc_port))
  {
    xmlrpc_port++;
    // std::cerr <<  "Trying port " << xmlrpc_port << std::endl;
  }

  return s;
}
