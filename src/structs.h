/************************************************************************
| $Id: structs.h,v 1.19 2009/01/24 19:21:29 kkoons Exp $
| structs.h
| Description:  This file should go away someday - it's stuff that I
|   wasn't sure how to break up.  --Morc XXX
*/
#ifndef STRUCTS_H_
#define STRUCTS_H_

extern "C"
{
#include <sys/types.h>
}
#include <stdio.h> // FILE
#include <string>
#include <vector>
#include <map>
#include <cstdint>

const size_t MAX_STRING_LENGTH = 8192;

#include "character.h"

using namespace std;

// FILE * fopen(const char *filename, const char *type);
// int fclose(FILE * fl);

const size_t MAX_INPUT_LENGTH = 160;
const size_t MAX_MESSAGES = 150;
const size_t MAX_OBJ_SDESC_LENGTH = 100;

const size_t MESS_ATTACKER = 1;
const size_t MESS_VICTIM = 2;
const size_t MESS_ROOM = 3;

/* ======================================================================== */
struct txt_block
{
  string text = {};
  struct txt_block *next = {};
  int aliased = {};
};

typedef struct txt_q
{
  struct txt_block *head;
  struct txt_block *tail;
} TXT_Q;

struct snoop_data
{
  Character *snooping;
  Character *snoop_by;
};

struct msg_type
{
  char *attacker_msg; /* message to attacker */
  char *victim_msg;   /* message to victim   */
  char *room_msg;     /* message to room     */
};

struct message_type
{
  struct msg_type die_msg;       /* messages when death            */
  struct msg_type miss_msg;      /* messages when miss             */
  struct msg_type hit_msg;       /* messages when hit              */
  struct msg_type sanctuary_msg; /* messages when hit on sanctuary */
  struct msg_type god_msg;       /* messages when hit on god       */
  struct message_type *next;     /* to next messages of ths kind.*/
};

struct message_list
{
  int a_type;                /* Attack type				*/
  int number_of_attacks;     /* # messages to chose from		*/
  struct message_type *msg;  /* List of messages			*/
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
  void SetQuestion(Character *ch, std::string question);
  void AddAnswer(Character *ch, std::string answer);
  void RemoveAnswer(Character *ch, unsigned int answer);
  void StartVote(Character *ch);
  void EndVote(Character *ch);
  void Reset(Character *ch);
  void OutToFile();
  bool HasVoted(Character *ch);
  bool Vote(Character *ch, unsigned int vote);
  void DisplayVote(Character *ch);
  void DisplayResults(Character *ch);
  bool IsActive() { return active; }
  CVoteData();
  ~CVoteData();

private:
  bool active;
  std::string vote_question;
  std::vector<SVoteData> answers;
  int total_votes;
  std::map<std::string, bool> ip_voted;
  std::map<std::string, bool> char_voted;
};

/*
 * TO types for act() output.
 */
/* OLD
const size_t TO_ROOM    0
const size_t TO_VICT    1
const size_t TO_NOTVICT 2
const size_t TO_CHAR    3
const size_t TO_GODS    4
*/

extern void debugpoint();

#endif
