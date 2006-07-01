/************************************************************************
| $Id: board.cpp,v 1.19 2006/07/01 19:36:41 dcastle Exp $
| board.C
| Description:  This file contains the implementation for the board
|   code.  It's old and should be rewritten --Morc XXX
*/

/*
board.c version 1.2 - Jun 1991 by Twilight.

1.2 changes:

   Added a board and message structure
   Took out all pointers in an effort to insure integrity in memory.
   Added differentiation between minimum read levels and minimum write/remove
   levels.

1.1 changes:

   Major repairs-- now allows multiple boards define at compile-time. 
   Set the constants NUM_BOARDS and add the new_new V-Numbers to the if/then
   structure directly below.  Also you must attach the board.c procedure in
   spec_assign.c as usual.

   Log message removals and restrict them to level 15 and above.
   Fixed act message resulting from message removal 
   Removed unused procedure "fix_long_desc"
   Added a message to inform others in room of a read in progress
   Added minimum level check for each board 
   (defined in array min_board_level[NUM_BOARDS]

*/

extern "C"
{
  #include <stdio.h> // FILE *
  #include <string.h> // memset()
  #include <ctype.h> // isspace(), isdigit()
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <room.h>
#include <obj.h>
#include <player.h> // MAX_*
#include <connect.h> // CON_WRITE_BOARD
#include <terminal.h> // BOLD
#include <fileinfo.h> // for the board files
#include <levels.h> // levels..
#include <clan.h>
#include <character.h> 
#include <utility.h> // FALSE
#include <memory.h>
#include <act.h>
#include <db.h>
#include <returnvals.h>

// These are the binary files in which to save/load messages

void board_write_msg(CHAR_DATA *ch, char *arg, int bnum);
int board_display_msg(CHAR_DATA *ch, char *arg, int bnum);
char *fread_string(FILE *fl, int hasher);
int board_remove_msg(CHAR_DATA *ch, char *arg, int bnum);
void board_save_board(int);
int board_check_locks (int bnum, CHAR_DATA *ch);
void board_load_board();
int board_show_board(CHAR_DATA *ch, char *arg, int bnum);
int fwrite_string(char *buf, FILE *fl);
void board_unlock_board(CHAR_DATA *ch);
void new_edit_board_unlock_board(CHAR_DATA *ch, int abort);
void check_for_awaymsgs(char_data *ch);

extern CHAR_DATA *character_list;
extern CWorld world;

const int MAX_MSGS           = 99;       // Max number of messages.
const int MAX_MESSAGE_LENGTH = 2048;     // that should be enough
const int NUM_BOARDS         = 55; 

int min_read_level[]   = {  0, IMMORTAL, OVERSEER, IMMORTAL, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0,  0, IMMORTAL, 0, 0, 0,//24
		     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                     IMMORTAL, 0 , OVERSEER,
		     0,0,0,0,0,0,0,0,0,0,0 };
int min_write_level[]  = {  5, IMMORTAL, OVERSEER, IMMORTAL, SERAPH , 1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, IMMORTAL,1,1,1,//24
		     1,1,1,1,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                     IMMORTAL, 0, OVERSEER,
		     1,1,1,1,1,1,1,1,1,1,1 };

int min_remove_level[] = { IMMORTAL, IMMORTAL, OVERSEER, IMMORTAL, SERAPH, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, IMMORTAL, 0, 0, 0,//24
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, IMMORTAL, 0, OVERSEER,
	IMMORTAL, IMMORTAL, IMMORTAL, IMMORTAL, IMMORTAL, IMMORTAL,
        IMMORTAL, IMMORTAL, IMMORTAL, IMMORTAL, IMMORTAL
 };
	
int board_clan[] = { -1,
                     -1, -1, -1, -1,  1,  2,  8, 9, 9, 4, // 1 through 10
                     15,  8, 10,  7, 24,  9, 17, -1, 19, 11, // 11 through 20
                     -1,  3, 20, 13, 27, 18,  5,  14, 6,  15, // 21 - 30
                     16, -1, 9, 13, -1, -1, 10, 26, 19, 15, // 31 - 40
                     -1, 21, -1, -1, -1, -1, -1, -1, -1, -1,
		     -1, -1, -1, -1, -1
		     };

int board_class[] = {
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
                        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1, -1, 1, 2, 3, 4, 5, 6, 7, 8,
			9, 10, 11

//			1947, 1946, 1940, 1944, 1941, 1950, 
//			1942, 1945, 1943, 1949, 1948, -1, -1
		    };

char save_file[NUM_BOARDS][42] = { 
  "board/mortal",  // 0
  "board/wiz",
  "board/imp",
  "board/build",
  "board/quest",
  "board/ulnhyrr",
  "board/tengu",
  "board/vampyre",
  "board/nazgul",
  "board/eclipse",  
  "board/DC_Guard", // 10
  "board/co.rpse",
  "board/merc",
  "board/askani",
  "board/studs",
  "board/smkjags", // 15
  "board/sng",
  "board/vig",
  "board/uruk",      
  "board/bandaleros", 
  "board/horde",    // 20
  "board/quests2do",
  "board/arcana",   
  "board/knight_sabers", // (actually sindicate)
  "board/ferach",
  "board/kindred",
  "board/anarchist",
  "board/bankuul",
  "board/blackaxe", 
  "board/timewarp",  
  "board/epoch2",   // 30
  "board/darktide",
  "board/eclipse2",
  "board/nazgulspecialboard",
  "board/slackers",
  "board/corpsetwo",
  "board/eclipseduvak",
  "board/tayledras",
  "board/knightsabertwo",
  "board/solaris",
  "board/overlords",
  "board/punishment",
  "board/anaphro",
  "board/coder",
  "board/mage",
  "board/cleric",
  "board/thief",
  "board/warrior",
  "board/anti",
  "board/pal",
  "board/barb",
  "board/monk",
  "board/ranger",
  "board/bard",
  "board/druid"
};
 
 static struct board_lock_struct {
  CHAR_DATA *locked_for;
  bool lock;
} board_lock[NUM_BOARDS];

struct message {
  char *date;
  char *title;
  char *author;
  char *text;
};

struct board {
  struct message msg[MAX_MSGS + 1];
  int number;
};

struct board boards[NUM_BOARDS];
struct board *curr_board;


int find_board(CHAR_DATA *ch)
{
  struct obj_data *i;

  if(!(world[ch->in_room].contents))
    return (-1);

  for(i = world[ch->in_room].contents; i; i = i->next_content) {
     if(!(strcmp(i->name, "board bulletin")))
       return (0);
     else if(!(strcmp(i->name, "board wizard")))
       return (1);
     else if(!(strcmp(i->name, "board implementer")))
       return (2);
     else if(!(strcmp(i->name, "board builder")))
       return(3);
     else if(!(strcmp(i->name, "board quest")))
       return(4);
     else if (!(strcmp(i->name, "board clan ulnhyrr")))
       return(5);
     else if (!(strcmp(i->name, "board clan tengu")))
       return(6);
     else if (!(strcmp(i->name, "board clan vampyre")))
       return(7);
     else if (!(strcmp(i->name, "board clan nazgul")))
       return(8);   
     else if (!(strcmp(i->name, "board clan eclipse")))
       return(9);  
     else if (!(strcmp(i->name, "board clan clanboard dcguard")))
       return(10);
     else if (!(strcmp(i->name, "board clan co.rpse")))
       return(11); 
     else if (!(strcmp(i->name, "board clan merc")))
       return(12);
     else if (!(strcmp(i->name, "board clan moor")))
       return(13); 
     else if (!(strcmp(i->name, "board clan studs")))
       return(14); 
     else if (!(strcmp(i->name, "board clan jaguar smoke")))
       return(15);
     else if (!(strcmp(i->name, "board clan sng")))
       return(16);
     else if (!(strcmp(i->name, "board clan vig")))
       return(17);
     else if (!(strcmp(i->name, "board uruk")))
       return(18);
     else if (!(strcmp(i->name, "board clan bandaleros")))
       return(19);
     else if (!(strcmp(i->name, "board clan horde")))
       return(20);
     else if (!(strcmp(i->name, "board quests2do")))
       return(21);
     else if (!(strcmp(i->name, "board arcana")))
       return(22);
     else if (!(strcmp(i->name, "board clan clanboard sindicate")))
       return(23);
     else if (!(strcmp(i->name, "board clan ferach")))
       return(24);
     else if (!(strcmp(i->name, "board clan kindred")))
       return(25);
     else if (!(strcmp(i->name, "screen window board clan anarchist")))
       return(26);
     else if (!(strcmp(i->name, "board clan ban'kuul globe")))
       return(27);
     else if (!(strcmp(i->name, "tablet board clanboard black axe")))
       return(28);
     else if (!(strcmp(i->name, "board clan timewarp")))
       return(29);
     else if (!(strcmp(i->name, "board askani")))
       return(30);
     else if (!(strcmp(i->name, "board clan darktide")))
       return(31);
     else if (!(strcmp(i->name, "board eclipse book eclipsebulletin")))
       return(32);
     else if (!(strcmp(i->name, "board special nazgul")))
       return(33);
     else if (!(strcmp(i->name, "board slackers clanboard")))
       return(34);
     else if (!(strcmp(i->name, "board clan Co.Rpse second")))
       return(35);
     else if (!(strcmp(i->name, "board clan eclipse duvak")))
       return(36);
     else if (!(strcmp(i->name, "board tayledras")))
       return(37);
     else if (!(strcmp(i->name, "board checklist sabers knight")))
       return(38);
     else if (!(strcmp(i->name, "board golden bulletin solaris")))
       return(39);
     else if (!(strcmp(i->name, "board overlords clan")))
       return(40);
     else if (!(strcmp(i->name, "board punishment")))
       return(41);
     else if (!(strcmp(i->name, "board clan anaphrodisia")))
       return(42);
     else if (!(strcmp(i->name, "board coding")))
       return(43);
     else if (!strcmp(i->name, "board guild mage glyph glyphs column"))
	return 44;
     else if (!strcmp(i->name, "board guild cleric"))
        return 45;
     else if (!strcmp(i->name, "board guild thief"))
        return 46;
     else if (!strcmp(i->name, "board guild warrior"))
        return 47;
     else if (!strcmp(i->name, "board guild anti journal"))
        return 48;
     else if (!strcmp(i->name, "board guild paladin register"))
        return 49;
     else if (!strcmp(i->name, "board guild barb"))
        return 50;
     else if (!strcmp(i->name, "board guild monk"))
        return 51;
     else if (!strcmp(i->name, "board guild ranger notes tree"))
        return 52;
     else if (!strcmp(i->name, "board guild bard bulletin"))
        return 53;
     else if (!strcmp(i->name, "board guild druid sheet papyrus"))
        return 54;
     } 


  return (-1);
}

int board(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA* invoker)
{
  static int has_loaded = 0;
  int bnum = -1;
  if(!cmd) {
    return eFAILURE;
  }
  if(!ch->desc)
    return eFAILURE; /* By MS or all NPC's will be trapped at the board */
  
  if(!has_loaded) {
    board_load_board();
    has_loaded = 1;
  }


  // Identify which board we're dealing with

  if((bnum = find_board(ch)) == (-1))
    return eFAILURE;


  switch (cmd) {
  case 12:  // look
    return(board_show_board(ch, arg, bnum));
  case 128: // write
    if(GET_INT(ch) < 9) {
      send_to_char("You are too stupid to know how to write!\r\n", ch);
      return eSUCCESS;
    }
    board_write_msg(ch, arg, bnum);
    return eSUCCESS;
  case 67: // read
    if(GET_INT(ch) < 9) {
      send_to_char("You are too stupid to know how to read!\r\n", ch);
      return eSUCCESS;
    }
    board_display_msg(ch, arg, bnum);
    return eSUCCESS;
  case 70: /* erase */
    if(GET_INT(ch) < 9) {
      send_to_char("You are too stupid to read them!\r\nDon't erase them they might be important!\r\n", ch);
      return eSUCCESS;
    }
    if(
       (bnum == 32 && ch->clan != 16 && GET_LEVEL(ch) < PATRON) ||
       (bnum == 18 && ch->clan != 34 && GET_LEVEL(ch) < PATRON) ||
       (bnum == 36 && ch->clan != 16 && GET_LEVEL(ch) < PATRON) ||
       (bnum == 35 && ch->clan != 22 && GET_LEVEL(ch) < PATRON)
      )
    {
      send_to_char("You can't erase posts from this board.\r\n", ch);
      return eSUCCESS;
      // This is the eclipse bounty board.  I agreed to it since it is
      // for clan "roleplay" purposes... -pir
      // also added this for Uruk'hai and Darna boards so only members
      // of the clan can remove posts - Duvak too
    }
    board_remove_msg(ch, arg, bnum);
    return eSUCCESS;
  default:
    return eFAILURE;
  }
}

void new_edit_board_unlock_board(CHAR_DATA *ch, int abort)
{
  int x, ind;

  for(x = 0; x < NUM_BOARDS; x++) {
    if(board_lock[x].locked_for == ch) {
      if (!abort) {
        board_lock[x].lock = 0;
        board_lock[x].locked_for = NULL;
        csendf(ch, "Your message is number %d.\n\r", boards[x].number);
	check_for_awaymsgs(ch);
      } else {
        ind = boards[x].number;
        curr_board = &boards[x];
        dc_free(curr_board->msg[ind].text);
        dc_free(curr_board->msg[ind].date);
        dc_free(curr_board->msg[ind].author);
        dc_free(curr_board->msg[ind].title);
        curr_board->msg[ind].text = NULL;
        curr_board->msg[ind].date = NULL;
        curr_board->msg[ind].author = NULL;
        curr_board->msg[ind].title = NULL;
        for ( ; ind < (curr_board->number) ; ind++ )
          curr_board->msg[ind] = curr_board->msg[ind+1];
        curr_board->msg[curr_board->number].text = NULL;
        curr_board->msg[curr_board->number].date = NULL; 
        curr_board->msg[curr_board->number].author = NULL;
        curr_board->msg[curr_board->number].title = NULL;
        curr_board->number--;
      }
      board_save_board(x);
    }
  }
}

void board_unlock_board(CHAR_DATA *ch)
{
  int x;
  
  for(x = 0; x < NUM_BOARDS; x++)
    if(board_lock[x].locked_for == ch) {
      board_lock[x].lock = 0;
      board_lock[x].locked_for = NULL;
      csendf(ch, "Your message is number %d.\n\r", boards[x].number);
      board_save_board(x);
    }
}

void board_write_msg(CHAR_DATA *ch, char *arg, int bnum) {

  int highmessage;
  char buf[MAX_STRING_LENGTH];
  long ct; // clock time
  char *tmstr;
  struct message *curr_msg;

  if(bnum == -1) {
    log("Board special procedure called for non-board object", ANGEL, LOG_BUG);
    send_to_char("This board is not in operation at this time.\n\r", ch);
    return;
  }

  curr_board = &boards[bnum];

  if(board_clan[bnum] != -1 && GET_LEVEL(ch) < OVERSEER) {
    if(ch->clan !=  board_clan[bnum]) {
      send_to_char("You aren't in the right clan bucko.\n\r", ch);
      return;
    }
    if(!has_right(ch, CLAN_RIGHTS_B_WRITE)) {
      send_to_char("You don't have the right!  Talk to your clan leader.\n\r", ch);
      return;
    }
  }
  if (board_class[bnum] != -1 && GET_LEVEL(ch) < IMMORTAL && GET_CLASS(ch) != board_class[bnum]) {
     send_to_char("You do not understand the writings written on this board.\r\n",ch);
     return;
  }

  if ((GET_LEVEL(ch) < min_write_level[bnum])) {
    send_to_char("You pick up a quill to write, but realize "
                  "you're not powerful enough\n\rto submit "
                  "intelligent material to this board.\n\r", ch);
    return;
  }

 if((curr_board->number) > (MAX_MSGS - 1)) {
    send_to_char("The board is full already.\n\r", ch);
    return;
  }

  // Check for locks, return if lock is found on this board

  if (board_check_locks(bnum, ch))
    return;

  // skip blanks

  for(; isspace(*arg); arg++);

  if (!*arg) {
    send_to_char("Need a header, fool.\n\r", ch);
    return;
  }

  // Now we're committed to writing a message.  Let's lock the board.

  board_lock[bnum].lock = 1;
  board_lock[bnum].locked_for = ch;

  // Lock set

  highmessage = boards[bnum].number;
  curr_msg = &curr_board->msg[++highmessage];

  if (!(strcmp("Topic", arg)) && GET_LEVEL(ch) > IMMORTAL) {
    curr_msg = &curr_board->msg[0];
    dc_free(curr_msg->title);
    curr_msg->title = NULL;
    dc_free(curr_msg->author);
    curr_msg->author = NULL;
    dc_free(curr_msg->date);
    curr_msg->date = NULL;
    dc_free(curr_msg->text);
    curr_msg->text = NULL;
    (boards[bnum].number)--;
  }

#ifdef LEAK_CHECK  
  curr_msg->title = (char *)calloc(strlen(arg) + 1, sizeof(char));
#else
  curr_msg->title = (char *)dc_alloc(strlen(arg) + 1, sizeof(char));
#endif

  strcpy(curr_msg->title, arg);

#ifdef LEAK_CHECK
  curr_msg->author = (char *)calloc(strlen(GET_NAME(ch)) + 1, sizeof(char));
#else
  curr_msg->author = (char *)dc_alloc(strlen(GET_NAME(ch)) + 1, sizeof(char));
#endif

  strcpy(curr_msg->author, GET_NAME(ch));
  ct = time(0);
  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  sprintf(buf, "%.10s", tmstr);
#ifdef LEAK_CHECK
  curr_msg->date = (char *)calloc(strlen(buf) + 1, sizeof(char));
#else
  curr_msg->date = (char *)dc_alloc(strlen(buf) + 1, sizeof(char));
#endif
  strcpy(curr_msg->date, buf);
  send_to_char("        Write your message and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n", ch);
 // send_to_char("Write your message. Terminate with a '~'.\n\r\n\r", ch);
  act("$n starts to write a message.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  // Take care of free-ing and zeroing if the message text is already
  // allocated previously

  if (curr_msg->text)
    dc_free (curr_msg->text);
  curr_msg->text = 0;

  // Initiate the string_add procedures from comm.c

//  ch->desc->connected = CON_WRITE_BOARD;
//  ch->desc->str = &curr_msg->text;
//  ch->desc->max_str = MAX_MESSAGE_LENGTH;
    ch->desc->connected = CON_WRITE_BOARD;
    ch->desc->strnew = &curr_msg->text;
    ch->desc->max_str = MAX_MESSAGE_LENGTH;

  (boards[bnum].number)++;
  
  if (boards[bnum].number < 0)
    boards[bnum].number = 0;
}

int board_remove_msg(CHAR_DATA *ch, char *arg, int bnum) {

  int ind, tmessage;
  char buf[256], number[MAX_INPUT_LENGTH+1];
  
  one_argument(arg, number);
  
  if (!*number || !isdigit(*number))
    return(0);
  
  if (!(tmessage = atoi(number))) return(0);
  
  if ( bnum == -1 ) {
    log("Board special procedure called for non-board object", ANGEL, LOG_BUG);
    send_to_char("This board is not in operation at this time.\n\r", ch);
    return 1;
  }

  curr_board = &boards[bnum];

  if (curr_board->number < 1) {
    send_to_char("The board is empty!\n\r", ch);
    return(1);
  }

  if (tmessage < 0 || tmessage > curr_board->number) {
    send_to_char("That message exists only in your imagination..\n\r",
		 ch);
    return(1);
  }

  /* Check for board locks, return if lock is found */
  
  if (board_check_locks(bnum, ch))
    return(1);

  ind = tmessage;

  // if clan board
  if(board_clan[bnum] != -1 && GET_LEVEL(ch) < OVERSEER) {
    if(ch->clan !=  board_clan[bnum]) {
      send_to_char("You aren't in the right clan bucko.\n\r", ch);
      return 1;
    }
    if(!has_right(ch, CLAN_RIGHTS_B_REMOVE) && strcmp(GET_NAME(ch), curr_board->msg[ind].author)) {
      send_to_char("You don't have the right!  Talk to your clan leader.\n\r", ch);
      return 1;
    }
  }
  else if (board_class[bnum] != -1 && GET_LEVEL(ch) < IMMORTAL && GET_CLASS(ch) != board_class[bnum]) {
     send_to_char("You do not understand the writings written on this board.\r\n",ch);
     return 1;
  }
  else if((GET_LEVEL(ch) < 
min_remove_level[bnum] && strcmp(GET_NAME(ch), curr_board->msg[ind].author)
          ) &&
           GET_LEVEL(ch) < OVERSEER) 
  {
    send_to_char("You try and grab one of the notes of the board but "
                 "get a nasty\n\rshock. Maybe you'd better leave it "
                 "alone.\n\r",ch);
    return 1;
  }

 /* If you change any of this remove stuff you also need to change new_edit_board_unlock_board - Rahz */

  dc_free(curr_board->msg[ind].text);
  dc_free(curr_board->msg[ind].date);
  dc_free(curr_board->msg[ind].author);
  dc_free(curr_board->msg[ind].title);
  curr_board->msg[ind].text = NULL;
  curr_board->msg[ind].date = NULL;
  curr_board->msg[ind].author = NULL;
  curr_board->msg[ind].title = NULL;

  for ( ; ind < (curr_board->number) ; ind++ )
    curr_board->msg[ind] = curr_board->msg[ind+1];

/* You MUST do ths, or the next message written after a remove will */
/* end up doing a free(curr_board->msg[ind].text) because it's not!! */
/* Causing strange shit to happen, because now the message has a     */
/* To a memory location that doesn't exist, and if THAT message gets */
/* Removed, it will destroy what it's pointing to. THIS is the board */
/* Bug we've been looking for!        -=>White Gold<=-               */

  curr_board->msg[curr_board->number].text = NULL;
  curr_board->msg[curr_board->number].date = NULL;
  curr_board->msg[curr_board->number].author = NULL;
  curr_board->msg[curr_board->number].title = NULL;

  curr_board->number--;

  send_to_char("Message erased.\n\r", ch);
  sprintf(buf, "$n just erased message %d.", tmessage);

  // Removal message also repaired
  act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL);

  board_save_board(bnum);
  return(1);
  }
 
char *fix_returns(char *text_string)
{
  char *localbuf;
  int point=0;
  int point2 = 0;

  if (!text_string) {
#ifdef LEAK_CHECK
    localbuf = (char *)calloc(2, sizeof(char));
#else
    localbuf = (char *)dc_alloc(2, sizeof(char));
#endif
    strcpy(localbuf,"\n");
    return(localbuf);
  }

  if (!(*text_string)) {
#ifdef LEAK_CHECK
    localbuf = (char *)calloc(strlen("(NULL)") + 1, sizeof(char));
#else
    localbuf = (char *)dc_alloc(strlen("(NULL)") + 1, sizeof(char));
#endif
    strcpy(localbuf,"(NULL)");
    return(localbuf);
  }

#ifdef LEAK_CHECK
  localbuf = (char *)calloc(strlen(text_string) + 1, sizeof(char));
#else
  localbuf = (char *)dc_alloc(strlen(text_string) + 1, sizeof(char));
#endif

  while(*(text_string+point) != '\0') 
    if (*(text_string+point) != '\r') {
      *(localbuf+point2) = *(text_string+point);
      point2++;
      point++;
    }
    else
      point++;
  *(localbuf + point2) = '\0'; /* You never made sure of null termination */
  return(localbuf);
}
  
void board_save_board(int bnum) {

  FILE * the_file;
  int ind;
  char *temp_add;
  struct message *curr_msg;

  // We're assuming the board number is valid since it was passed by
  // our own code

  curr_board = &boards[bnum];

  the_file = dc_fopen(save_file[bnum], "w");

  if(!the_file) {
      log("Unable to open/create save file for bulletin board", ANGEL,
          LOG_BUG);
      return;
  }

  fprintf(the_file," %d ", curr_board->number);
  for (ind = 0; ind <= curr_board->number; ind++) {
    curr_msg = &curr_board->msg[ind];
    fwrite_string(curr_msg->title, the_file);
    fwrite_string(curr_msg->author, the_file);
    fwrite_string(curr_msg->date, the_file);
    fwrite_string((temp_add = fix_returns(curr_msg->text)), the_file);
    dc_free(temp_add);
  }
  dc_fclose(the_file);
  return;
}

void free_boards_from_memory()
{
  int bnum = 0;
  struct message *curr_msg = NULL;

  for (bnum = 0; bnum < NUM_BOARDS; bnum++) 
  {
    curr_board = &boards[bnum];

    for (int ind = 0; ind <= curr_board->number; ind++) {

      curr_msg = &curr_board->msg[ind];

      if(curr_msg->title)
        dc_free(curr_msg->title);
      if(curr_msg->author)
        dc_free(curr_msg->author);
      if(curr_msg->date)
        dc_free(curr_msg->date);
      if(curr_msg->text)
        dc_free(curr_msg->text);
    }
  }
}

void board_load_board() {

  FILE *the_file;
  int ind;
  int bnum;
  struct message *curr_msg;
  
  memset(boards, 0, sizeof(boards)); /* Zero out the array, make sure no */
                                     /* Funky pointers are left in the   */
                                     /* Allocated space                  */

  for ( bnum = 0 ; bnum < NUM_BOARDS ; bnum++ ) {
    board_lock[bnum].lock = 0;
    board_lock[bnum].locked_for = 0;
  }

  for (bnum = 0; bnum < NUM_BOARDS; bnum++) {
    boards[bnum].number = -1;
    the_file = dc_fopen(save_file[bnum], "r");
    if (!the_file) 
      continue;


    fscanf( the_file, " %d ", &boards[bnum].number);
    if (boards[bnum].number < 0 || boards[bnum].number > MAX_MSGS || 
	feof(the_file)) {
      boards[bnum].number = -1;
      dc_fclose(the_file);
      continue;
    }

    curr_board = &boards[bnum];

    for (ind = 0; ind <= curr_board->number; ind++) {
      curr_msg = &curr_board->msg[ind];
      curr_msg->title = fread_string (the_file, 0);
      curr_msg->author = fread_string (the_file, 0);
      curr_msg->date = fread_string (the_file, 0);
      curr_msg->text = fread_string (the_file, 0);
    }
    dc_fclose(the_file);
  }
}

int board_display_msg(CHAR_DATA *ch, char *arg, int bnum)
{
  char buf[512], number[MAX_INPUT_LENGTH+1], buffer[MAX_STRING_LENGTH];
  int tmessage;
  struct message *curr_msg;

  one_argument(arg, number);

  if(IS_MOB(ch)) {
    if(!*number) {
      send_to_char("Sorry, mobs have to specify the number of the post they want to read.\r\n", ch);
      return(0);
    }
  }
  else {
    if (!*number && ch->pcdata->last_mess_read > 0) {
      ch->pcdata->last_mess_read++;
      sprintf(number, "%i", ch->pcdata->last_mess_read);
    }
  }

  if (!*number || !isdigit(*number))
  {
    send_to_char("Read what?\r\n",ch);
    return(0);
  }
  if (!(tmessage = atoi(number))) return(0);

  curr_board = &boards[bnum];


  if(board_clan[bnum] != -1 && GET_LEVEL(ch) < OVERSEER) {
    if(ch->clan !=  board_clan[bnum]) {
      send_to_char("You aren't in the right clan bucko.\n\r", ch);
      return 1;
    }
    if(!has_right(ch, CLAN_RIGHTS_B_READ)) {
      send_to_char("You don't have the right!  Talk to your clan leader.\n\r", ch);
      return 1;
    }
  }
  if (board_class[bnum] != -1 && GET_LEVEL(ch) < IMMORTAL && GET_CLASS(ch) != board_class[bnum]) {
     send_to_char("You do not understand the writings written on this board.\r\n",ch);
     return 1;
  }

  if ((GET_LEVEL(ch) < min_read_level[bnum]) &&
        (GET_LEVEL(ch) < OVERSEER)) 
  {
    send_to_char("You try and look at the messages on the board but"
                 " you\n\rcannot comprehend their meaning.\n\r\n\r",ch);
    act("$n tries to read the board, but looks bewildered.", ch, 0, 0,
	TO_ROOM, INVIS_NULL);
    return(1);
  }

  if(boards[bnum].number == -1) {
    send_to_char("The board is empty!\n\r", ch);
    return(1);
  }
  
  if (tmessage < 0 || tmessage > curr_board->number) {
    send_to_char("That message doesn't exist, moron.\n\r",ch);
    return(1);
  }

  if(!IS_MOB(ch))
    ch->pcdata->last_mess_read = tmessage;

  curr_msg = &curr_board->msg[tmessage];

  sprintf(buf, "$n reads message %d titled: %s", tmessage, curr_msg->title);
  act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL);
  if(IS_MOB(ch) || IS_SET(ch->pcdata->toggles, PLR_ANSI))
    sprintf(buffer, "Message %2d (%s): " RED BOLD "%-14s " YELLOW"- %s"NTEXT,
         tmessage, curr_msg->date, curr_msg->author, curr_msg->title );
  else
    sprintf(buffer, "Message %2d (%s): %-14s - %s",
         tmessage, curr_msg->date, curr_msg->author, curr_msg->title );

  sprintf(buffer + strlen(buffer), "\n\r----------\n\r"
          CYAN"%s"NTEXT, curr_msg->text);
  page_string(ch->desc, buffer, 1);
  return(1);
}
		
int board_show_board(CHAR_DATA *ch, char *arg, int bnum)
{
  int i;
  char buf[MAX_STRING_LENGTH], tmp[MAX_INPUT_LENGTH+1],tmp2[MAX_STRING_LENGTH];
  
  arg = one_argument(arg, tmp);
  one_argument(arg, tmp2);
  if (!*tmp)     return eFAILURE;
  if (!isname(tmp, "board bulletin"))
    if (!isname(tmp, "at") || !isname(tmp2, "board bulletin"))
       return eFAILURE;
  curr_board = &boards[bnum];

  if(board_clan[bnum] != -1 && GET_LEVEL(ch) < OVERSEER) {
    if(ch->clan !=  board_clan[bnum]) {
      send_to_char("You aren't in the right clan bucko.\n\r", ch);
      return eSUCCESS;
    }
    if(!has_right(ch, CLAN_RIGHTS_B_READ)) {
      send_to_char("You don't have the right!  Talk to your clan leader.\n\r", ch);
      return eSUCCESS;
    }
  }
  if (board_class[bnum] != -1 && GET_LEVEL(ch) < IMMORTAL && GET_CLASS(ch) != board_class[bnum]) {
     send_to_char("You do not understand the writings written on this board.\r\n",ch);
     return eSUCCESS;
  }

  if ((GET_LEVEL(ch) < min_read_level[bnum]) &&
        (GET_LEVEL(ch) < OVERSEER)) {
    send_to_char("You try and look at the messages on the board "
                 "but you\n\rcannot comprehend their meaning.\n\r",ch);
    act("$n tries to read the board, but looks bewildered.",ch, 0, 0,
        TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  curr_board = &boards[bnum];

  act("$n studies the board.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  strcpy(buf,"This is a bulletin board. Usage: "
         "READ/ERASE <mesg #>, WRITE <header>\n\r");
  if (boards[bnum].number == -1)
    strcat(buf, "The board is empty.\n\r");
  else {
    sprintf(buf + strlen(buf), "There are %d messages on the board.\n\r",
	    curr_board->number);
    sprintf(buf + strlen(buf), "\n\rBoard Topic:\n\r%s------------\n\r",
            curr_board->msg[0].text);
    for ( i = curr_board->number ; i >= 1; i-- ) 
       if(IS_MOB(ch) || IS_SET(ch->pcdata->toggles, PLR_ANSI))
         sprintf(buf + strlen(buf), "(%s) "YELLOW"%-14s "RED"%2d: "
               GREEN"%.47s"NTEXT"\n\r", curr_board->msg[i].date,
               curr_board->msg[i].author, i, curr_board->msg[i].title);
       else 
         sprintf(buf + strlen(buf), "(%s) %-14s %2d: "
               "%.47s\n\r", curr_board->msg[i].date,
               curr_board->msg[i].author, i, curr_board->msg[i].title);
 

  }
  board_save_board(bnum);
  page_string(ch->desc, buf, 1);
  return eSUCCESS;
}

int board_check_locks (int bnum, CHAR_DATA *ch) {
  
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *tmp_char;
  bool found = FALSE;
  if (!board_lock[bnum].lock) return(0);
  
  /* FIRST lets' see if ths character is even in the game anymore! -WG-*/
  for(tmp_char = character_list; tmp_char; tmp_char = tmp_char->next) {
     if(tmp_char == board_lock[bnum].locked_for) {
       found = TRUE;
       break;
     }
   }

  if(!found) {
      log("Board: board locked for a user not in game.", ANGEL, LOG_BUG);
      board_lock[bnum].lock = 0;
      board_lock[bnum].locked_for = NULL;
      return(0);
  }

  /* Check for link-death of lock holder */

  if (!board_lock[bnum].locked_for->desc) {
    sprintf(buf,"You push %s aside and approach the "
                "board.\n\r",
                 board_lock[bnum].locked_for->name);
    send_to_char(buf, ch);
  }

  /* Else see if lock holder is still in write-string mode */

  else if (board_lock[bnum].locked_for->desc->str) { /* Lock still holding */
    sprintf(buf,"You try to approach the board but %s blocks your"
                " way.\n\r",board_lock[bnum].locked_for->name);
    send_to_char(buf, ch);
    return (1);
  }

  /* Otherwise, the lock has been lifted */

  board_save_board(bnum);
  board_lock[bnum].lock = 0;
  board_lock[bnum].locked_for = 0;
  return(0);
}
  
int fwrite_string (char *buf, FILE *fl)
{
  return (fprintf(fl, "%s~\n", buf));
}
