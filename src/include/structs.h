/************************************************************************
| $Id: structs.h,v 1.15 2008/11/21 00:37:15 kkoons Exp $
| structs.h
| Description:  This file should go away someday - it's stuff that I
|   wasn't sure how to break up.  --Morc XXX
*/
#ifndef STRUCTS_H_
#define STRUCTS_H_

extern "C" {
#include <sys/types.h>
#include <stdio.h> // FILE
}
#include <string>
#include <vector>
#include <map>
FILE * dc_fopen(const char *filename, const char *type);
int dc_fclose(FILE * fl);

typedef signed char		 sbyte;
typedef unsigned char		 ubyte;
typedef signed char		 byte;

typedef signed short int         int16;
typedef unsigned short int      uint16;

typedef signed int               int32;
typedef unsigned int            uint32;

// Can't use these unfortunatly because long longs just don't work right
// for some reason.
typedef signed long long         int64;
typedef unsigned long long      uint64;

typedef	struct char_data	CHAR_DATA;
typedef	struct obj_data		OBJ_DATA;

#define MAX_STRING_LENGTH   8192
#define MAX_INPUT_LENGTH     160
#define MAX_MESSAGES         150
#define MAX_OBJ_SDESC_LENGTH 100

#define MESS_ATTACKER 1
#define MESS_VICTIM   2
#define MESS_ROOM     3

/* ======================================================================== */
struct txt_block
{
    char *text;
    struct txt_block *next;
    int aliased;
};

typedef struct txt_q
{
    struct txt_block *head;
    struct txt_block *tail;
} TXT_Q;

struct snoop_data
{
    CHAR_DATA *snooping; 
    CHAR_DATA *snoop_by;
};


struct msg_type 
{
    char *attacker_msg;  /* message to attacker */
    char *victim_msg;    /* message to victim   */
    char *room_msg;      /* message to room     */
};

struct message_type
{
    struct msg_type die_msg;      /* messages when death            */
    struct msg_type miss_msg;     /* messages when miss             */
    struct msg_type hit_msg;      /* messages when hit              */
    struct msg_type sanctuary_msg;/* messages when hit on sanctuary */
    struct msg_type god_msg;      /* messages when hit on god       */
    struct message_type *next;/* to next messages of ths kind.*/
};

struct message_list
{
    int a_type;               /* Attack type				*/
    int number_of_attacks;    /* # messages to chose from		*/
    struct message_type *msg; /* List of messages			*/
    struct message_type *msg2; /* List of messages with toggle damage ON */
};

struct SVoteData
{
  std::string answer;
  int votes;
};

class CVoteData
{
public:
  void SetQuestion(std::string question);
  void AddAnswer(std::string answer);
  void RemoveAnswer(int answer);
  bool StartVote();
  bool EndVote();
  void Reset();
  bool Vote(struct char_data *ch, int vote);
  void DisplayVote(struct char_data *ch);
  void DisplayResults(struct char_data *ch);
  bool IsActive() {return active;}
  CVoteData();
  ~CVoteData();  

private:
  bool active;
  std::string vote_question;
  std::vector<SVoteData> answers;
  int total_votes;
  std::map<std::string, bool> ip_voted;
};

/*
 * TO types for act() output.
 */
/* OLD
#define TO_ROOM    0
#define TO_VICT    1
#define TO_NOTVICT 2
#define TO_CHAR    3
#define TO_GODS    4
*/

extern void debugpoint();


#endif
