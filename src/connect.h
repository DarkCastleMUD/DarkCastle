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

#define STATE(d) ((d)->connected)

enum conn
{
  PLAYING,
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
  unsigned int points;
  unsigned int selection;
  uint8_t race;
  uint8_t clss;
};

class Connection
{
public:
  int descriptor = {}; /* file descriptor for socket	*/
  int desc_num = {};
  char *name = {};     /* Copy of the player name	*/
  char host[80] = {};  /* hostname			*/
  conn connected = {}; /* mode of 'connectedness'	*/
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
};

#endif
