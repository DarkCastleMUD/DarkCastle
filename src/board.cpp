/************************************************************************
| $Id: board.cpp,v 1.45 2014/07/26 23:01:47 jhhudso Exp $
| board.C
| Description:  This file contains the implementation for the board
|   code.  It's old and should be rewritten --Morc XXX
*/

/*
board.cpp version 1.3 - Nov 2008 by Rubicon

1.3 changes:

   Replaced nearly everything with STL based code.
   Changed how the new_edit_unlock works by using it as a callback function
   from new_edit.cpp ( new_add_string() )


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

#include <cstdio>  // FILE *
#include <cstring> // memset()
#include <cctype>  // isspace(), isdigit()

#include "room.h"
#include "obj.h"
#include "player.h"   // MAX_*
#include "connect.h"  // Connection::states::WRITE_BOARD
#include "terminal.h" // BOLD
#include "fileinfo.h" // for the board files
#include "levels.h"   // levels..
#include "clan.h"
#include "character.h"
#include "utility.h" // false
#include "memory.h"
#include "act.h"
#include "db.h"
#include "returnvals.h"
#include <string>
#include <map>
#include <vector>
#include "interp.h"
#include <sstream>

#define MAX_MESSAGE_LENGTH 2048

struct message
{
  std::string date;
  std::string title;
  std::string author;
  std::string text;
};

struct BOARD_INFO
{
  Character *locked_for;
  bool lock;
  int min_read_level;
  int min_write_level;
  int min_remove_level;
  int type;
  int owner;
  std::string save_file;
  std::vector<message> msgs;
};

// These are the binary files in which to save/load messages

void board_write_msg(Character *ch, const char *arg, std::map<std::string, BOARD_INFO>::iterator board);
int board_display_msg(Character *ch, const char *arg, std::map<std::string, BOARD_INFO>::iterator board);
char *fread_string(FILE *fl, int hasher);
int board_remove_msg(Character *ch, const char *arg, std::map<std::string, BOARD_INFO>::iterator board);
void board_save_board(std::map<std::string, BOARD_INFO>::iterator board);
void board_load_board();
int board_show_board(Character *ch, const char *arg, std::map<std::string, BOARD_INFO>::iterator board);
int fwrite_string(char *buf, FILE *fl);
void new_edit_board_unlock_board(Character *ch, int abort);

#define ANY_BOARD 0
#define CLASS_BOARD 1
#define CLAN_BOARD 2

#define NO_OWNER -1
#define CLAN_ULNHYRR 1
#define CLAN_DARKTIDE 2
#define CLAN_ARCANA 3
#define CLAN_DARKENED 4
#define CLAN_DCGUARD 5
#define CLAN_TIMEWARP 6
#define CLAN_CONTINUUM 7
#define CLAN_MERC 8
#define CLAN_NAZGUL 9
#define CLAN_BLACKAXE 10
#define CLAN_TRIAD 11
#define CLAN_KOBAL 12
#define CLAN_SLACKERS 13
#define CLAN_KEHUA 14
#define CLAN_ASKANI 15
#define CLAN_HOUSELESSROGUES 16
#define CLAN_THEHORDE 17
#define CLAN_ANARCHIST 18
#define CLAN_SOLARIS 19
#define CLAN_SINDICATE 20

#define CLASS_MAGE 1
#define CLASS_CLERIC 2
#define CLASS_THIEF 3
#define CLASS_WARRIOR 4
#define CLASS_ANTI 5
#define CLASS_PAL 6
#define CLASS_BARB 7
#define CLASS_MONK 8
#define CLASS_RANGER 9
#define CLASS_BARD 10
#define CLASS_DRUID 11

struct RESERVATION_DATA
{
  char *buf;
  message new_post;
  std::map<std::string, BOARD_INFO>::iterator board;
};

// std::map to hold callback information for writing
std::map<Character *, RESERVATION_DATA *> wait_for_write;

/*
Function to populate the board_db with all of the current clan info
*/
std::map<std::string, BOARD_INFO> populate_boards()
{
  std::map<std::string, BOARD_INFO> board_tmp;
  BOARD_INFO board_struct;

  board_struct.min_read_level = 0;
  board_struct.min_write_level = 5;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/mortal";
  board_tmp["board bulletin"] = board_struct;

  board_struct.min_read_level = IMMORTAL;
  board_struct.min_write_level = IMMORTAL;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/wiz";
  board_tmp["board wizard"] = board_struct;

  board_struct.min_read_level = OVERSEER;
  board_struct.min_write_level = OVERSEER;
  board_struct.min_remove_level = OVERSEER;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/imp";
  board_tmp["board implementer"] = board_struct;

  board_struct.min_read_level = IMMORTAL;
  board_struct.min_write_level = IMMORTAL;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/build";
  board_tmp["board builder"] = board_struct;

  board_struct.min_read_level = 0;
  board_struct.min_write_level = SERAPH;
  board_struct.min_remove_level = SERAPH;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/quest";
  board_tmp["board quest"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_ULNHYRR;
  board_struct.save_file = "board/ulnhyrr";
  board_tmp["board clan ulnhyrr"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/kindred";
  board_tmp["board clan kindred"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/ferach";
  board_tmp["board clan_ferach"] = board_struct;

  board_struct.min_read_level = IMMORTAL;
  board_struct.min_write_level = IMMORTAL;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/quests2do";
  board_tmp["board quests2do"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_ARCANA;
  board_struct.save_file = "board/arcana";
  board_tmp["board arcana"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_SINDICATE;
  board_struct.save_file = "board/knight_sabers";
  board_tmp["board clan clanboard sindicate"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/horde";
  board_tmp["board clan horde"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/bandaleros";
  board_tmp["board clan bandaleros"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/uruk";
  board_tmp["board uruk"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_ASKANI;
  board_struct.save_file = "board/askani";
  board_tmp["board askani"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/studs";
  board_tmp["board clan studs"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_MERC;
  board_struct.save_file = "board/merc";
  board_tmp["board clan merc"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_DARKENED;
  board_struct.save_file = "board/darkened";
  board_tmp["board clan darkened"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_DARKTIDE;
  board_struct.save_file = "board/tengu";
  board_tmp["board clan darktide"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/vampyre";
  board_tmp["board clan vampyre"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_NAZGUL;
  board_struct.save_file = "board/nazgul";
  board_tmp["board clan nazgul"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_ANARCHIST;
  board_struct.save_file = "board/anarchist";
  board_tmp["screen window board clan anarchist"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/bankuul";
  board_tmp["board clan ban'kuul globe"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_BLACKAXE;
  board_struct.save_file = "board/askani";
  board_tmp["board clan blackaxe"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_TIMEWARP;
  board_struct.save_file = "board/timewarp";
  board_tmp["board clan timewarp"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_ASKANI;
  board_struct.save_file = "board/epoch2";
  board_tmp["board askani"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_HOUSELESSROGUES;
  board_struct.save_file = "board/darktide";
  board_tmp["board clan houselessrogues"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_NAZGUL;
  board_struct.save_file = "board/nazgulspecialboard";
  board_tmp["board special nazgul"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_SLACKERS;
  board_struct.save_file = "board/slackers";
  board_tmp["board slackers clanboard"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/tayledras";
  board_tmp["board tayledras"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_SINDICATE;
  board_struct.save_file = "board/knightsabertwo";
  board_tmp["board checklist sabers knight"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_KOBAL;
  board_struct.save_file = "board/smkjags";
  board_tmp["board window kobal"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_SOLARIS;
  board_struct.save_file = "board/solaris";
  board_tmp["board golden bulletin solaris"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_CONTINUUM;
  board_struct.save_file = "board/continuum";
  board_tmp["board clan continuum"] = board_struct;

  board_struct.min_read_level = IMMORTAL;
  board_struct.min_write_level = IMMORTAL;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/punishment";
  board_tmp["board punishment"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/anaphro";
  board_tmp["board clan anaphrodisia"] = board_struct;

  board_struct.min_read_level = OVERSEER;
  board_struct.min_write_level = OVERSEER;
  board_struct.min_remove_level = OVERSEER;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/coder";
  board_tmp["board coding"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_MAGE;
  board_struct.save_file = "board/mage";
  board_tmp["board guild mage glyph glyphs column"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_CLERIC;
  board_struct.save_file = "board/cleric";
  board_tmp["board guild cleric"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_THIEF;
  board_struct.save_file = "board/thief";
  board_tmp["board guild thief"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_WARRIOR;
  board_struct.save_file = "board/warrior";
  board_tmp["board guild warrior"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_ANTI;
  board_struct.save_file = "board/anti";
  board_tmp["board guild anti journal"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_PAL;
  board_struct.save_file = "board/pal";
  board_tmp["board guild paladin register"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_BARB;
  board_struct.save_file = "board/barb";
  board_tmp["board guild barb"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_MONK;
  board_struct.save_file = "board/monk";
  board_tmp["board guild monk"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_RANGER;
  board_struct.save_file = "board/ranger";
  board_tmp["board guild ranger notes tree"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_BARD;
  board_struct.save_file = "board/bard";
  board_tmp["board guild bard bulletin"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_DRUID;
  board_struct.save_file = "board/druid";
  board_tmp["board guild druid sheet papyrus"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = 1;
  board_struct.type = CLAN_BOARD;
  board_struct.owner = CLAN_TRIAD;
  board_struct.save_file = "board/triad";
  board_tmp["board clan triad chalk"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = OVERSEER;
  board_struct.min_remove_level = OVERSEER;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/punish";
  board_tmp["board punishment mortal"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 10;
  board_struct.min_remove_level = 10;
  board_struct.type = ANY_BOARD;
  board_struct.owner = NO_OWNER;
  board_struct.save_file = "board/vend";
  board_tmp["board bulletin vend"] = board_struct;

  return board_tmp;
}

std::map<std::string, BOARD_INFO> board_db = populate_boards();

int save_boards()
{
  std::map<std::string, BOARD_INFO>::iterator board_it;

  FILE *the_file;

  the_file = fopen("board/index", "w");

  if (!the_file)
  {
    logentry("Unable to open/create save file for bulletin board index", ANGEL,
             LogChannels::LOG_BUG);
    return eFAILURE;
  }

  for (board_it = board_db.begin(); board_it != board_db.end(); board_it++)
  {

    fprintf(the_file, " %d ", board_it->second.min_read_level);
    fprintf(the_file, " %d ", board_it->second.min_write_level);
    fprintf(the_file, " %d ", board_it->second.min_remove_level);
    fprintf(the_file, " %d ", board_it->second.type);
    fprintf(the_file, " %d ", board_it->second.owner);
    fwrite_string((char *)board_it->second.save_file.c_str(), the_file);
    fwrite_string((char *)board_it->first.c_str(), the_file);
  }
  fclose(the_file);
  return eSUCCESS;
}

/*
Entry function called from assign_proc.
handles commands and calls appropriate functions
*/
int board(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  static int has_loaded = 0;

  std::map<std::string, BOARD_INFO>::iterator board;

  if (cmd != CMD_LOOK && cmd != CMD_READ && cmd != CMD_WRITE && cmd != CMD_ERASE)
  {
    return eFAILURE;
  }

  if (!arg)
    return eFAILURE;

  if (!ch->desc)
    return eFAILURE; /* By MS or all NPC's will be trapped at the board */

  if (!has_loaded)
  {
    board_load_board();
    has_loaded = 1;
  }

  // Identify which board we're dealing with

  if (!obj)
    return eFAILURE;

  board = board_db.find(obj->name);

  if (board == board_db.end())
    return eFAILURE;

  char arg1[MAX_INPUT_LENGTH];
  one_argument(arg, arg1);

  if (!isexact(arg1, obj->name) && cmd == CMD_LOOK)
    return eFAILURE;

  switch (cmd)
  {
  case CMD_LOOK: // look
    return (board_show_board(ch, arg, board));
  case CMD_WRITE: // write
    if (GET_INT(ch) < 9)
    {
      ch->sendln("You are too stupid to know how to write!");
      return eSUCCESS;
    }
    board_write_msg(ch, arg, board);
    return eSUCCESS;
  case CMD_READ: // read
    if (GET_INT(ch) < 9)
    {
      ch->sendln("You are too stupid to know how to read!");
      return eSUCCESS;
    }
    board_display_msg(ch, arg, board);
    return eSUCCESS;
  case CMD_ERASE: /* erase */
    if (GET_INT(ch) < 9)
    {
      ch->sendln("You are too stupid to read them!\r\nDon't erase them they might be important!");
      return eSUCCESS;
    }
    if (
        ((!strcmp(obj->name, "board uruk")) && ch->clan != CLAN_NAZGUL && ch->getLevel() < PATRON))
    {
      ch->sendln("You can't erase posts from this board.");
      return eSUCCESS;
      // added this for Uruk'hai board so only members can remove posts
    }
    board_remove_msg(ch, arg, board);
    return eSUCCESS;
  default:
    return eFAILURE;
  }
}

/*
This function acts as a callback from edit_new
function call new_add_string()
It notifies us when the user is done writing the post
and we can copy the std::string to the board
*/
void new_edit_board_unlock_board(Character *ch, int abort)
{
  RESERVATION_DATA *reserve = wait_for_write[ch];
  message new_msg;

  new_msg.text = reserve->buf;
  dc_free(reserve->buf);
  new_msg.date = reserve->new_post.date;
  new_msg.author = reserve->new_post.author;
  new_msg.title = reserve->new_post.title;

  if (new_msg.date.empty()) // means they were editing Topic
  {
    if (!new_msg.text.empty() && !new_msg.title.empty())
    {
      reserve->board->second.msgs[0].text = new_msg.text;
      reserve->board->second.msgs[0].title = new_msg.title;
    }
  }
  else
    reserve->board->second.msgs.push_back(new_msg);
  delete reserve;
}

void board_write_msg(Character *ch, const char *arg, std::map<std::string, BOARD_INFO>::iterator board)
{
  char buf[MAX_STRING_LENGTH];
  time_t timep; // clock time
  char *tmstr;

  if (board->second.type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->second.owner)
    {
      ch->sendln("You aren't in the right clan bucko.");
      return;
    }
    if (!has_right(ch, CLAN_RIGHTS_B_WRITE))
    {
      ch->sendln("You don't have the right!  Talk to your clan leader.");
      return;
    }
  }
  if (board->second.type == CLASS_BOARD && ch->isMortal() && GET_CLASS(ch) != board->second.owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return;
  }

  if ((ch->getLevel() < board->second.min_write_level))
  {
    send_to_char("You pick up a quill to write, but realize "
                 "you're not powerful enough\n\rto submit "
                 "intelligent material to this board.\r\n",
                 ch);
    return;
  }

  // skip blanks

  for (; isspace(*arg); arg++)
    ;

  if (!*arg)
  {
    ch->sendln("Need a header, fool.");
    return;
  }

  RESERVATION_DATA *reserve = new RESERVATION_DATA;

  reserve->new_post.title = arg;

  reserve->new_post.author = GET_NAME(ch);

  timep = time(0);
  tmstr = asctime(localtime(&timep));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  sprintf(buf, "%.10s", tmstr);

  reserve->new_post.date = buf;

  send_to_char("        Write your message and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n",
               ch);

  act("$n starts to write a message.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  // if the title is Topic, this clears the date to let us know that
  // during the callback the topic will need updated.
  if (!(strcmp("Topic", arg)) && ch->getLevel() > IMMORTAL)
  {
    reserve->new_post.date.clear();
  }

  reserve->buf = 0;
  reserve->board = board;
  wait_for_write[ch] = reserve;
  ch->desc->connected = Connection::states::WRITE_BOARD;
  ch->desc->strnew = &reserve->buf;
  ch->desc->max_str = MAX_MESSAGE_LENGTH;
}

int board_remove_msg(Character *ch, const char *arg, std::map<std::string, BOARD_INFO>::iterator board)
{
  unsigned int ind, tmessage;
  char buf[256], number[MAX_INPUT_LENGTH + 1];

  one_argument(arg, number);

  if (!*number || !isdigit(*number))
    return eFAILURE;

  if (!(tmessage = atoi(number)))
    return eFAILURE;

  if (board->second.msgs.empty())
  {
    ch->sendln("The board is empty!");
    return eSUCCESS;
  }

  if (tmessage == 0 || tmessage >= board->second.msgs.size())
  {
    ch->sendln("That message exists only in your imagination..");
    return eSUCCESS;
  }

  ind = tmessage;

  // if clan board
  if (board->second.type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->second.owner)
    {
      ch->sendln("You aren't in the right clan bucko.");
      return eSUCCESS;
    }
    if (!has_right(ch, CLAN_RIGHTS_B_REMOVE) && board->second.msgs[ind].author.compare(GET_NAME(ch)))
    {
      ch->sendln("You don't have the right!  Talk to your clan leader.");
      return eSUCCESS;
    }
  }
  else if (board->second.type == CLASS_BOARD && ch->isMortal() && GET_CLASS(ch) != board->second.owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return eSUCCESS;
  }
  else if ((ch->getLevel() < board->second.min_remove_level && board->second.msgs[ind].author.compare(GET_NAME(ch))) && ch->getLevel() < OVERSEER)
  {
    send_to_char("You try and grab one of the notes of the board but "
                 "get a nasty\n\rshock. Maybe you'd better leave it "
                 "alone.\r\n",
                 ch);
    return eSUCCESS;
  }

  board->second.msgs.erase(board->second.msgs.begin() + ind);

  ch->sendln("Message erased.");
  sprintf(buf, "$n just erased message %d.", tmessage);

  // Removal message also repaired
  act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL);

  board_save_board(board);
  return eSUCCESS;
}

std::string remove_slashr(std::string unformatted)
{
  if (unformatted.empty())
    return "";

  std::string write_me = unformatted;
  std::string::iterator slashr;

  for (slashr = write_me.end(); slashr != write_me.begin(); slashr--)
    if (*slashr == '\r')
      write_me.erase(slashr);

  if (*slashr == '\r')
    write_me.erase(slashr);
  return write_me;
}

void board_save_board(std::map<std::string, BOARD_INFO>::iterator board)
{

  FILE *the_file;
  std::string write_me;
  unsigned int ind;

  the_file = fopen(board->second.save_file.c_str(), "w");

  if (!the_file)
  {
    logentry("Unable to open/create save file for bulletin board", ANGEL,
             LogChannels::LOG_BUG);
    return;
  }

  fprintf(the_file, " %d ", board->second.msgs.size());
  for (ind = 0; ind < board->second.msgs.size(); ind++)
  {
    write_me = remove_slashr(board->second.msgs[ind].title);
    fwrite_string((char *)write_me.c_str(), the_file);

    write_me = remove_slashr(board->second.msgs[ind].author);
    fwrite_string((char *)write_me.c_str(), the_file);

    write_me = remove_slashr(board->second.msgs[ind].date);
    fwrite_string((char *)write_me.c_str(), the_file);

    write_me = remove_slashr(board->second.msgs[ind].text);
    fwrite_string((char *)write_me.c_str(), the_file);
  }
  fclose(the_file);
  return;
}

std::string check_returns(std::string in_str)
{
  std::string check_me = in_str;
  std::string::iterator checker;
  for (checker = check_me.begin(); checker != check_me.end(); checker++)
    if (*checker == '\n')
    {
      if (checker + 1 != check_me.end() && *(checker + 1) != '\r')
        check_me.insert(checker + 1, '\r');
    }

  return check_me;
}

void board_load_board()
{

  FILE *the_file;
  int ind;
  struct message curr_msg;
  int number;

  std::map<std::string, BOARD_INFO>::iterator map_it;

  for (map_it = board_db.begin(); map_it != board_db.end(); map_it++)
  {
    map_it->second.lock = 0;
    map_it->second.locked_for = nullptr;

    the_file = fopen((*map_it).second.save_file.c_str(), "r");
    if (!the_file)
      continue;

    fscanf(the_file, " %d ", &number);
    if (number < 0 || feof(the_file))
    {
      fclose(the_file);
      continue;
    }

    for (ind = 0; ind < number; ind++)
    {

      curr_msg.title = fread_string(the_file, 27);
      curr_msg.title = check_returns(curr_msg.title);

      curr_msg.author = fread_string(the_file, 27);
      curr_msg.author = check_returns(curr_msg.author);

      curr_msg.date = fread_string(the_file, 27);
      curr_msg.date = check_returns(curr_msg.date);

      curr_msg.text = fread_string(the_file, 27);
      curr_msg.text = check_returns(curr_msg.text);

      map_it->second.msgs.push_back(curr_msg);
    }

    fclose(the_file);
  }
}

int board_display_msg(Character *ch, const char *arg, std::map<std::string, BOARD_INFO>::iterator board)
{
  char buf[MAX_STRING_LENGTH], number[MAX_INPUT_LENGTH + 1];
  std::string board_msg;
  one_argument(arg, number);
  unsigned int tmessage;

  if (IS_MOB(ch))
  {
    if (!*number)
    {
      ch->sendln("Sorry, mobs have to specify the number of the post they want to read.");
      return eFAILURE;
    }
  }
  else
  {
    if (!*number && ch->player->last_mess_read > 0)
    {
      ch->player->last_mess_read++;
      sprintf(number, "%i", ch->player->last_mess_read);
    }
  }

  if (!*number || !isdigit(*number))
  {
    ch->sendln("Read what?");
    return eFAILURE;
  }
  if (!(tmessage = atoi(number)))
    return eFAILURE;

  if (board->second.type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->second.owner)
    {
      ch->sendln("You aren't in the right clan bucko.");
      return eSUCCESS;
    }
    if (!has_right(ch, CLAN_RIGHTS_B_READ))
    {
      ch->sendln("You don't have the right!  Talk to your clan leader.");
      return eSUCCESS;
    }
  }
  if (board->second.type == CLASS_BOARD && ch->isMortal() && GET_CLASS(ch) != board->second.owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return eSUCCESS;
  }

  if ((ch->getLevel() < board->second.min_read_level))
  {
    send_to_char("You try and look at the messages on the board but"
                 " you\n\rcannot comprehend their meaning.\n\r\n\r",
                 ch);
    act("$n tries to read the board, but looks bewildered.", ch, 0, 0,
        TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  if (board->second.msgs.empty())
  {
    ch->sendln("The board is empty!");
    return eSUCCESS;
  }

  if (tmessage == 0 || tmessage >= board->second.msgs.size())
  {
    ch->sendln("That message doesn't exist, moron.");
    return eSUCCESS;
  }

  if (!IS_MOB(ch))
    ch->player->last_mess_read = tmessage;

  sprintf(buf, "$n reads message %d titled: %s", tmessage, board->second.msgs[tmessage].title.c_str());
  act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL);

  if (IS_MOB(ch) || isSet(ch->player->toggles, Player::PLR_ANSI))
  {
    snprintf(buf, MAX_STRING_LENGTH, "Message %2d (%s): " RED BOLD "%-14s " YELLOW "- %s" NTEXT,
             tmessage, board->second.msgs[tmessage].date.c_str(),
             board->second.msgs[tmessage].author.c_str(), board->second.msgs[tmessage].title.c_str());
    board_msg += buf;
  }
  else
  {
    snprintf(buf, MAX_STRING_LENGTH, "Message %2d (%s): %-14s - %s", tmessage, board->second.msgs[tmessage].date.c_str(),
             board->second.msgs[tmessage].author.c_str(), board->second.msgs[tmessage].title.c_str());
    board_msg += buf;
  }

  snprintf(buf, MAX_STRING_LENGTH, "\n\r----------\n\r" CYAN "%s" NTEXT, board->second.msgs[tmessage].text.c_str());
  board_msg += buf;

  page_string(ch->desc, board_msg.c_str(), 1);
  return eSUCCESS;
}

int board_show_board(Character *ch, const char *arg, std::map<std::string, BOARD_INFO>::iterator board)
{
  int i;
  std::string board_msg;
  char buf[MAX_STRING_LENGTH];
  if (board->second.type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->second.owner)
    {
      ch->sendln("You aren't in the right clan bucko.");
      return eSUCCESS;
    }
    if (!has_right(ch, CLAN_RIGHTS_B_READ))
    {
      ch->sendln("You don't have the right!  Talk to your clan leader.");
      return eSUCCESS;
    }
  }
  if (board->second.type == CLASS_BOARD && ch->isMortal() && GET_CLASS(ch) != board->second.owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return eSUCCESS;
  }

  if ((ch->getLevel() < board->second.min_read_level))
  {
    send_to_char("You try and look at the messages on the board "
                 "but you\n\rcannot comprehend their meaning.\r\n",
                 ch);
    act("$n tries to read the board, but looks bewildered.", ch, 0, 0,
        TO_ROOM, INVIS_NULL);
    return eSUCCESS;
  }

  act("$n studies the board.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  ch->sendln("This is a bulletin board. Usage: READ/ERASE <mesg #>, WRITE <header>");
  if (board->second.msgs.empty())
    ch->sendln("The board is empty.");
  else
  {
    csendf(ch, "There are %d messages on the board.\r\n", board->second.msgs.size());
    ;

    csendf(ch, "Board Topic:\n\r%s------------\n\r", board->second.msgs[0].text.c_str());
    std::vector<message>::reverse_iterator msg_it;
    i = board->second.msgs.size() - 1;
    for (msg_it = board->second.msgs.rbegin(); (i > 0) && (msg_it < board->second.msgs.rend()); ++msg_it)
      if (IS_MOB(ch) || isSet(ch->player->toggles, Player::PLR_ANSI))
      {
        snprintf(buf, MAX_STRING_LENGTH, "(%s) " YELLOW "%-14s " RED "%2d: " GREEN "%.47s" NTEXT "\n\r",
                 msg_it->date.c_str(), msg_it->author.c_str(), i--, msg_it->title.c_str());
        board_msg += buf;
        //         csendf(ch, "(%s) "YELLOW"%-14s "RED"%2d: "GREEN"%.47s"NTEXT"\n\r", msg_it->date.c_str(),
        //                    msg_it->author.c_str(),i--, msg_it->title.c_str());
      }
      else
      {
        snprintf(buf, MAX_STRING_LENGTH, "(%s) %-14s %2d: %.47s\n\r", msg_it->date.c_str(),
                 msg_it->author.c_str(), i--, msg_it->title.c_str());
        board_msg += buf;
      }
  }
  board_save_board(board);
  page_string(ch->desc, board_msg.c_str(), 1);
  return eSUCCESS;
}

int fwrite_string(char *buf, FILE *fl)
{
  return (fprintf(fl, "%s~\n", buf));
}
