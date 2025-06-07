#ifndef CONNECT_H_
#define CONNECT_H_
/************************************************************************
| $Id: connect.h,v 1.14 2011/08/28 18:29:45 jhhudso Exp $
| connect.h
| Description: State of connectedness information.
*/
#include "DC/character.h"
#include "DC/structs.h" // MAX_INPUT_LENGTH
#include "DC/comm.h"
#include "DC/common.h"

int isbanned(QHostAddress address);

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
  attribute_t selection{};
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

  inet_protocol_family_t getInet_Protocol_Fanily(void) { return inet_protocol_family; }
  QString getHeader(void) { return header; }
  bool isActive(void) { return active; }
  QHostAddress getSourceAddress(void) { return source_address; }
  QHostAddress getDestinationAddress(void) { return destination_address; }
  quint16 getSourcePort(void) { return source_port; }
  quint16 getDestinationPort(void) { return destination_port; }

private:
  inet_protocol_family_t inet_protocol_family = {};
  QString header = {};
  bool active = false;
  QHostAddress source_address = {};
  QHostAddress destination_address = {};
  quint16 source_port = {};
  quint16 destination_port = {};
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
    EDITING_V2,
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

  Proxy proxy = {};

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
  std::string buf = {};        /* buffer for raw input	*/
  std::string last_input = {}; /* the last input	*/
  QByteArray output = {};      /* output buffer for writing to connection	*/
  std::string inbuf = {};
  std::queue<std::string> input = {}; /* queue of unprocessed input	*/
  Character *character = {};          /* linked to char		*/
  Character *original = {};           /* for switch / return		*/
  class Connection *snooping = {};    /* Who is this char snooping       */
  class Connection *snoop_by = {};    /* And who is snooping this char   */
  class Connection *next = {};        /* link to next descriptor	*/
  int tick_wait = {};                 /* # ticks desired to wait	*/
  int reallythere = {};               /* Goddamm #&@$*% sig 13 (hack) */
  int prompt_mode = {};
  uint8_t idle_tics = {};
  time_t login_time = {};
  stat_data *stats = {}; // for rolling up a char

  char **strnew = {};    /* for the modify-str system	*/
  QString *qstrnew = {}; // for the modify-str system for QStrings */
  char *backstr = {};
  int idle_time = {}; // How long the descriptor has been idle, overall.
  bool color = {};
  bool server_size_echo = false;
  bool allowColor = 1;

  void send(QString txt);

  const char *getPeerOriginalAddressC(void);

  QHostAddress getPeerAddress(void)
  {
    return peer_address_;
  }
  QHostAddress getPeerOriginalAddress(void)
  {
    if (proxy.isActive())
    {
      return proxy.getSourceAddress();
    }
    return getPeerAddress();
  }

  QString getPeerFullAddressString(void)
  {
    if (proxy.isActive())
    {
      return QStringLiteral("%1 via %2").arg(getPeerOriginalAddress().toString()).arg(getPeerAddress().toString());
    }
    else
    {
      return getPeerOriginalAddress().toString();
    }
  }

  void setPeerAddress(QHostAddress address)
  {
    peer_address_ = address;
  }

  void setPeerPort(uint16_t port)
  {
    peer_port_ = port;
  }

  QString getName(void);
  inline bool isEditing(void) const noexcept
  {
    return connected == Connection::states::EDITING ||
           connected == Connection::states::EDITING_V2 ||
           connected == Connection::states::WRITE_BOARD ||
           connected == Connection::states::EDIT_MPROG ||
           connected == Connection::states::SEND_MAIL ||
           connected == Connection::states::EXDSCR;
  }
  inline bool isPlaying(void) const noexcept
  {
    return connected == Connection::states::PLAYING;
  }
  int process_output(void);
  QString createBlackjackPrompt(void);
  QString createPrompt(void);
  void setOutput(QString output_buffer);
  void appendOutput(QString output_buffer);
  QByteArray getOutput(void) const;

private:
  QHostAddress peer_address_ = {};
  uint16_t peer_port_ = {};
};

class Sockets
{
public:
  Sockets(Character *ch = nullptr, QString searchkey = "");
  QMap<QString, uint64_t> getIPs(void) const { return IPs_; }
  QList<Connection *> getConnections(void) const { return connections_; }
  uint64_t getLongestNameSize(void) const { return longest_name_size_; }
  uint64_t getLongestIPSize(void) const { return longest_IP_size_; }
  uint64_t getLongestConnectionStateSize(void) const { return longest_connection_state_size_; }
  uint64_t getLongestIdleSize(void) const { return longest_idle_size_; }

private:
  QMap<QString, uint64_t> IPs_{};
  QList<Connection *> connections_{};
  qsizetype longest_name_size_{};
  qsizetype longest_IP_size_{};
  qsizetype longest_connection_state_size_{};
  qsizetype longest_idle_size_{};
};

#endif
