#include <iostream>
#include <unistd.h>
#include <xmlrpc.h>

#include <character.h>
#include <handler.h>
#include <connect.h>
#include <levels.h>

using namespace std;

class common {
protected:
  int authenticated(XmlRpcValue &params) {
    std::string& login = params[0];
    std::string& password = params[1];
    char buffer[MAX_STRING_LENGTH];
    
    strncpy(buffer, login.c_str(), MAX_STRING_LENGTH);
    char_data * ch = get_all_pc(buffer);

    if (ch && IS_PC(ch) && GET_LEVEL(ch) >= IMMORTAL && ch->pcdata->pwd &&
	!strncmp(crypt(password.c_str(), ch->pcdata->pwd),
		 ch->pcdata->pwd, (PASSWORD_LEN))) {
      return 1;
    }
    
    return 0;
  }
};

class login : public XmlRpcServerMethod, public common {
public:
  login(XmlRpcServer* s) : XmlRpcServerMethod("login", s) {}

  void execute(XmlRpcValue& params, XmlRpcValue& result)
  {
    if (!authenticated(params)) {
      result = "unauthorized";
    } else {
      result = "authorized";      
    }

    return;
  }
};

class editor : public XmlRpcServerMethod, public common {
public:
  editor(XmlRpcServer* s) : XmlRpcServerMethod("editor", s) {}

  void execute(XmlRpcValue& params, XmlRpcValue& result)
  {
    if (!authenticated(params)) {
      result = "unauthorized";
      return;
    }

    std::string& login = params[0];
    char buffer[MAX_STRING_LENGTH];
    
    strncpy(buffer, login.c_str(), MAX_STRING_LENGTH);
    char_data *ch = get_all_pc(buffer);
    if (ch == 0) {
      result = "player not found in game";
      return;
    }

    if (IS_PC(ch) && !IS_SET(ch->pcdata->toggles, PLR_EDITOR_WEB)) {
      result = "plr_editor_dc";
      return;
    }

    std::string& contents = params[2];

    if (IS_PC(ch) && GET_LEVEL(ch) >= IMMORTAL && ch->desc->strnew) {
      switch(ch->desc->web_connected) {
      case CON_EDIT_MPROG:
	if (!contents.empty()) {
	  if (!(*ch->desc->strnew)) {
	    if ((int)contents.size() > ch->desc->max_str) {
	      SEND_TO_Q("String too long - Truncated.\r\n", ch->desc);
	      contents[ch->desc->max_str] = '\0';
	    }
	    CREATE(*ch->desc->strnew, char, contents.size() + 5);
	    strcpy(*ch->desc->strnew, contents.c_str());
	  } else {
	    if (!(*ch->desc->strnew = (char *) dc_realloc(*ch->desc->strnew,
							  strlen(*ch->desc->strnew) + contents.size() + 5))) {
	      perror("string_add");
	      abort();
	    }
	    strcpy(*ch->desc->strnew, contents.c_str());
	  }
	  ch->desc->web_connected = CON_PLAYING;
	  result = *(ch->desc->strnew);
	  ch->desc->strnew = 0;
	  send_to_char("Entry submitted.\n\r", ch);
	} else {
	  result = *(ch->desc->strnew);
	}
	break;
      default:
	break;
      }
    } else {
      ch->desc->web_connected = CON_PLAYING;
    }
  }
};

class get_editor_type : public XmlRpcServerMethod, public common {
public:
  get_editor_type(XmlRpcServer* s) : XmlRpcServerMethod("get_editor_type", s) {}

  void execute(XmlRpcValue& params, XmlRpcValue& result)
  {
    if (!authenticated(params)) {
      result = "unauthorized";
      return;
    }

    std::string& login = params[0];
    char buffer[MAX_STRING_LENGTH];
    
    strncpy(buffer, login.c_str(), MAX_STRING_LENGTH);
    char_data *ch = get_all_pc(buffer);
    if (ch == 0) {
      result = "player not found in game";
      return;
    }
    
    if (IS_PC(ch) && !IS_SET(ch->pcdata->toggles, PLR_EDITOR_WEB)) {
      result = "plr_editor_dc";
      return;
    }

    if (IS_PC(ch) && GET_LEVEL(ch) >= IMMORTAL) {
      switch(ch->desc->web_connected) {
      case CON_EDIT_MPROG:
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

  while (!s->bindAndListen(xmlrpc_port)) {
    xmlrpc_port++;
    cout << "Trying port " << xmlrpc_port << endl;
  }

  return s;
}

