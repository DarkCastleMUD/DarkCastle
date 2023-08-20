#ifndef CONNECT_H_
#define CONNECT_H_
/************************************************************************
| $Id: connect.h,v 1.14 2011/08/28 18:29:45 jhhudso Exp $
| connect.h
| Description: State of connectedness information.
*/
#include "character.h"
#include "structs.h" // MAX_INPUT_LENGTH
#include "comm.h"

int isbanned(QString host);

#define STATE(d) ((d)->connected)

// if you change, make sure you update char *connected_states[] in const.C
// also update connected_types[]

#define MAX_RAW_INPUT_LENGTH 512

class stat_data
{
public:
  stat_data();
  void setMin(void);
  uint8_t getMin(uint8_t cur, int8_t mod, uint8_t min);
  int str[5], tel[5], wis[5], dex[5], con[5];
  int min_str, min_int, min_wis, min_dex, min_con;
  uint64_t points;
  uint64_t selection;
  uint8_t race;
  uint8_t clss;
  bool increase(uint64_t points = 1);
  bool decrease(uint64_t points = 1);
};

class Proxy
{
public:
  Proxy(QString);
  Proxy(void) {}

  enum class inet_protocol_family_t
  {
    UNKNOWN,
    TCP4,
    TCP6,
    UNRECOGNIZED
  };
  inet_protocol_family_t getInet_Protocol_Fanily(void) { return inet_protocol_family; }
  QString getHeader(void) { return header; }
  bool isActive(void) { return active; }
  QHostAddress getSourceAddress(void) { return source_address; }
  QHostAddress getDestinationAddress(void) { return destination_address; }
  quint16 getSourcePort(void) { return source_port; }
  quint16 getDestinationPort(void) { return destination_port; }

private:
  inet_protocol_family_t inet_protocol_family;
  QString header;
  bool active = false;
  QHostAddress source_address;
  QHostAddress destination_address;
  quint16 source_port;
  quint16 destination_port;
};

class Connection
{
public:
  enum states
  {
    PLAYING,
    GET_PROXY,
    GET_NAME,
    GET_OLD_PASSWORD,
    CONFIRM_NEW_NAME,
    GET_NEW_PASSWORD,
    CONFIRM_NEW_PASSWORD,
    GET_NEW_SEX,
    OLD_GET_CLASS,
    READ_MOTD,
    SELECT_MENU,
    RESET_PASSWORD,
    CONFIRM_RESET_PASSWORD,
    EXDSCR,
    OLD_GET_RACE,
    WRITE_BOARD,
    EDITING,
    SEND_MAIL,
    DELETE_CHAR,
    OLD_CHOOSE_STATS,
    PFILE_WIPE,
    ARCHIVE_CHAR,
    CLOSE,
    CONFIRM_PASSWORD_CHANGE,
    EDIT_MPROG,
    DISPLAY_ENTRANCE,
    PRE_DISPLAY_ENTRANCE,
    SELECT_RECOVERY_MENU,
    GET_NEW_RECOVERY_QUESTION,
    GET_NEW_RECOVERY_ANSWER,
    GET_NEW_RECOVERY_EMAIL,
    QUESTION_ANSI,
    GET_ANSI,
    QUESTION_SEX,
    QUESTION_STAT_METHOD,
    GET_STAT_METHOD,
    OLD_STAT_METHOD,
    NEW_STAT_METHOD,
    NEW_PLAYER,
    QUESTION_RACE,
    GET_RACE,
    QUESTION_CLASS,
    GET_CLASS,
    QUESTION_STATS,
    GET_STATS
  };

  Proxy proxy;

  int descriptor = {}; /* file descriptor for socket	*/
  int desc_num = {};
  char *name = {};       /* Copy of the player name	*/
  states connected = {}; /* mode of 'connectedness'	*/
  int web_connected = {};
  int wait = {};           /* wait for how many loops	*/
  char *showstr_head = {}; /* for paging through texts	*/
  const char **showstr_vector = {};
  int showstr_count = {};
  int showstr_page = {};
  bool new_newline = {}; /* prepend newline in output	*/
  //  char	**str;			/* for the modify-str system	*/
  char **hashstr = {};
  char *astr = {};
  int max_str = {};
  string buf = {};        /* buffer for raw input	*/
  string last_input = {}; /* the last input	*/
  string output = {};     /* queue of strings to send	*/
  string inbuf = {};
  queue<string> input = {};        /* queue of unprocessed input	*/
  Character *character = {};       /* linked to char		*/
  Character *original = {};        /* for switch / return		*/
  class Connection *snooping = {}; /* Who is this char snooping       */
  class Connection *snoop_by = {}; /* And who is snooping this char   */
  class Connection *next = {};     /* link to next descriptor	*/
  int tick_wait = {};              /* # ticks desired to wait	*/
  int reallythere = {};            /* Goddamm #&@$*% sig 13 (hack) */
  int prompt_mode = {};
  uint8_t idle_tics = {};
  time_t login_time = {};
  stat_data *stats = {}; // for rolling up a char

  char **strnew = {}; /* for the modify-str system	*/
  char *backstr = {};
  int idle_time = {}; // How long the descriptor has been idle, overall.
  bool color = {};
  bool server_size_echo = false;
  bool allowColor = 1;
  void send(QString txt);
  const char *getHostC(void)
  {
    if (proxy.isActive())
    {
      if (proxy.getSourceAddress().isNull())
      {
        return "";
      }

      return proxy.getSourceAddress().toString().toStdString().c_str();
    }
    return host_.toStdString().c_str();
  }
  QString getHost(void)
  {
    if (proxy.isActive())
    {
      if (proxy.getSourceAddress().isNull())
      {
        return "";
      }

      return proxy.getSourceAddress().toString();
    }
    return host_;
  }
  void setHost(QString host) { host_ = host; }

private:
  QString host_ = {};
};

#endif
