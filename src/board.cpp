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

#include <qcompare.h>
#include <qstring.h>
#include <qstringview.h>
#include <qswap.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include <vector>

#include "DC/dcstdio.h" // FILEPtr
#include "DC/DC.h"
#include "DC/connect.h" // Connection::states::WRITE_BOARD
#include "DC/clan.h"
#include "DC/character.h"
#include "DC/utility.h" // false
#include "DC/act.h"
#include "DC/returnvals.h"
#include "DC/interp.h"
#include "DC/obj.h"
#include "DC/common.h"
#include "DC/levels.h"
#include "DC/db.h"

#define MAX_MESSAGE_LENGTH 2048

class message
{
public:
  QString date;
  QString title;
  QString author;
  QString text;
};

class BOARD_INFO
{
public:
  Character *locked_for = {};
  bool lock = {};
  int min_read_level = {};
  int min_write_level = {};
  int min_remove_level = {};
  int type = {};
  int owner = {};
  QString save_file;
  QList<message> msgs;
};
using boards_t = std::map<QString, BOARD_INFO>;

// These are the binary files in which to save/load messages
void board_do_write(Character *ch, QStringList arg, boards_t::iterator board);
int board_do_read(Character *ch, QStringList arg, boards_t::iterator board);
int board_do_erase(Character *ch, QStringList arg, boards_t::iterator board);
int board_do_look(Character *ch, QStringList arg, boards_t::iterator board);

void board_save_board(boards_t::iterator board);
void board_load_board();
void new_edit_board_unlock_board(Character *ch, int abort);

char *fread_string(FILEPtr fl, int hasher);

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

class RESERVATION_DATA
{
public:
  QString buf;
  message new_post;
  boards_t::iterator board;
};

// std::map to hold callback information for writing
std::map<Character *, RESERVATION_DATA *> wait_for_write;

/*
Function to populate the board_db with all of the current clan info
*/
boards_t populate_boards()
{
  boards_t board_tmp;
  BOARD_INFO board_struct = {};

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

boards_t board_db = populate_boards();

int save_boards()
{
  boards_t::iterator board_it;

  FILEPtr the_file;

  if (QDir::current().dirName() == u"lib"_s && !QDir(u"board"_s).exists())
  {
    if (!QDir::current().mkdir("board"))
    {
      qWarning("Error creating missing '%s/board' directory.", qPrintable(QDir::currentPath()));
    }
    else
    {
      qWarning("Created missing '%s/board' directory.", qPrintable(QDir::currentPath()));
    }
  }

  auto filename = u"board/index"_s;
  the_file = dc_fopen(filename, "w");

  if (!the_file)
  {
    logentry(u"Unable to create save file for bulletin board index at '%1'"_s.arg(QDir::currentPath() + "/" + filename), ANGEL,
             DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }

  for (board_it = board_db.begin(); board_it != board_db.end(); board_it++)
  {

    dc_fprintf(the_file, " %d ", board_it->second.min_read_level);
    dc_fprintf(the_file, " %d ", board_it->second.min_write_level);
    dc_fprintf(the_file, " %d ", board_it->second.min_remove_level);
    dc_fprintf(the_file, " %d ", board_it->second.type);
    dc_fprintf(the_file, " %d ", board_it->second.owner);
    fwrite_string(board_it->second.save_file, the_file);
    fwrite_string(board_it->first, the_file);
  }

  return ReturnValue::eSUCCESS;
}

/*
Entry function called from assign_proc.
handles commands and calls appropriate functions
*/

int board(Character *ch, class Object *obj, cmd_t cmd, const char *c_arguments, Character *invoker)
{
  auto arguments = QString(c_arguments).split(' ');
  auto arg1 = arguments.value(0);

  static int has_loaded = 0;

  boards_t::iterator board;

  if (cmd != cmd_t::LOOK && cmd != cmd_t::READ && cmd != cmd_t::WRITE && cmd != cmd_t::ERASE)
  {
    return ReturnValue::eFAILURE;
  }

  if (arg1.isEmpty())
    return ReturnValue::eFAILURE;

  if (!ch->desc)
    return ReturnValue::eFAILURE; /* By MS or all NPC's will be trapped at the board */

  if (!has_loaded)
  {
    board_load_board();
    has_loaded = 1;
  }

  // Identify which board we're dealing with

  if (!obj)
    return ReturnValue::eFAILURE;

  board = board_db.find(qPrintable(obj->Name()));

  if (board == board_db.end())
    return ReturnValue::eFAILURE;

  if (!isexact(arg1, obj->Name()) && cmd == cmd_t::LOOK)
    return ReturnValue::eFAILURE;

  switch (cmd)
  {
  case cmd_t::LOOK: // look
    return (board_do_look(ch, arguments, board));
  case cmd_t::WRITE: // write
    if (GET_INT(ch) < 9)
    {
      ch->sendln("Your intelligence is too low to write!");
      return ReturnValue::eSUCCESS;
    }
    board_do_write(ch, arguments, board);
    return ReturnValue::eSUCCESS;
  case cmd_t::READ: // read
    if (GET_INT(ch) < 9)
    {
      ch->sendln("Your intelligence is too low to read!");
      return ReturnValue::eSUCCESS;
    }
    board_do_read(ch, arguments, board);
    return ReturnValue::eSUCCESS;
  case cmd_t::ERASE: /* erase */
    if (GET_INT(ch) < 9)
    {
      ch->sendln("You are too stupid to read them!\nDon't erase them they might be important!");
      return ReturnValue::eSUCCESS;
    }
    if (
        ((obj->Name() == QStringLiteral("board uruk")) && ch->clan != CLAN_NAZGUL && ch->getLevel() < PATRON))
    {
      ch->sendln("You can't erase posts from this board.");
      return ReturnValue::eSUCCESS;
      // added this for Uruk'hai board so only members can remove posts
    }
    board_do_erase(ch, arguments, board);
    return ReturnValue::eSUCCESS;
  default:
    return ReturnValue::eFAILURE;
  }
}

/*
This function acts as a callback from edit_new
function call new_add_string()
It notifies us when the user is done writing the post
and we can copy the QString to the board
*/
void new_edit_board_unlock_board(Character *ch, int abort)
{
  RESERVATION_DATA *reserve = wait_for_write[ch];
  message new_msg;

  new_msg.text = reserve->buf;
  // qWarning("[%s]", qPrintable(reserve->buf));
  new_msg.date = reserve->new_post.date;
  new_msg.author = reserve->new_post.author;
  new_msg.title = reserve->new_post.title;

  if (new_msg.date.isEmpty()) // means they were editing Topic
  {
    if (!new_msg.text.isEmpty() && !new_msg.title.isEmpty())
    {
      reserve->board->second.msgs[0].text = new_msg.text;
      reserve->board->second.msgs[0].title = new_msg.title;
    }
  }
  else
    reserve->board->second.msgs.push_back(new_msg);
  delete reserve;
}

void board_do_write(Character *ch, QStringList arguments, boards_t::iterator board)
{
  auto arg = arguments.value(0);
  QString buf;
  time_t timep; // clock time
  QString tmstr;

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
  if (board->second.type == CLASS_BOARD && !ch->isImmortalPlayer() && GET_CLASS(ch) != board->second.owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return;
  }

  if ((ch->getLevel() < board->second.min_write_level))
  {
    send_to_char("You pick up a quill to write, but realize "
                 "you're not powerful enough\nto submit "
                 "intelligent material to this board.\n",
                 ch);
    return;
  }

  // skip blanks
  arg = arg.trimmed();

  if (arg.isEmpty())
  {
    ch->sendln("Need a header.");
    return;
  }

  RESERVATION_DATA *reserve = new RESERVATION_DATA;

  reserve->new_post.title = arg;

  reserve->new_post.author = GET_NAME(ch);

  timep = time(0);
  tmstr = asctime(localtime(&timep));
  buf = tmstr;
  reserve->new_post.date = QDateTime::currentDateTime().toString(Qt::RFC2822Date);

  ch->sendln(u"        Write your message and stay within the line.  (/s saves /h for help)\n"
             "   --------------------------------------------------------------------------"_s);

  act("$n starts to write a message.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  // if the title is Topic, this clears the date to let us know that
  // during the callback the topic will need updated.
  if (arg == u"Topic"_s && ch->getLevel() > IMMORTAL)
  {
    reserve->new_post.date.clear();
  }

  reserve->buf = {};
  reserve->board = board;
  wait_for_write[ch] = reserve;
  ch->desc->connected = Connection::states::WRITE_BOARD;
  ch->desc->strnew = &reserve->buf;
  ch->desc->max_str = MAX_MESSAGE_LENGTH;
}

int board_do_erase(Character *ch, QStringList arguments, boards_t::iterator board)
{
  auto arg1 = arguments.value(0);
  bool ok = false;
  auto tmessage = arg1.toUInt(&ok);

  if (board->second.msgs.empty())
  {
    ch->sendln("The board is empty!");
    return ReturnValue::eSUCCESS;
  }

  if (board->second.msgs.size() == 1 && (!ok || tmessage != 0))
  {
    ch->sendln(u"ID '%1' not valid. The only valid ID is 0."_s.arg(arg1));
    return ReturnValue::eFAILURE;
  }

  if (!ok || tmessage > board->second.msgs.size() - 1)
  {
    ch->sendln(u"ID '%1' not valid. Valid IDs are %2-%3"_s.arg(arg1.trimmed()).arg(0).arg(board->second.msgs.size() - 1));
    return ReturnValue::eFAILURE;
  }

  if (tmessage == 0 && board->second.msgs.size() != 1)
  {
    ch->sendln("You can't erase ID 0 until it's the last ID.");
    return ReturnValue::eSUCCESS;
  }

  // if clan board
  if (board->second.type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->second.owner)
    {
      ch->sendln("You aren't in the right clan bucko.");
      return ReturnValue::eSUCCESS;
    }
    if (!has_right(ch, CLAN_RIGHTS_B_REMOVE) && board->second.msgs[tmessage].author.compare(GET_NAME(ch)))
    {
      ch->sendln("You don't have the right!  Talk to your clan leader.");
      return ReturnValue::eSUCCESS;
    }
  }
  else if (board->second.type == CLASS_BOARD && !ch->isImmortalPlayer() && GET_CLASS(ch) != board->second.owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return ReturnValue::eSUCCESS;
  }
  else if ((ch->getLevel() < board->second.min_remove_level && board->second.msgs[tmessage].author.compare(GET_NAME(ch))) && ch->getLevel() < OVERSEER)
  {
    send_to_char("You try and grab one of the notes of the board but "
                 "get a nasty\nshock. Maybe you'd better leave it "
                 "alone.\n",
                 ch);
    return ReturnValue::eSUCCESS;
  }

  board->second.msgs.remove(tmessage);

  ch->sendln("Message erased.");
  auto buf = u"$n just erased message %1."_s.arg(tmessage);
  act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL);

  board_save_board(board);
  return ReturnValue::eSUCCESS;
}

void board_save_board(boards_t::iterator board)
{

  FILEPtr the_file;
  QString write_me;
  unsigned int ind;

  if (QDir::current().dirName() == u"lib"_s && !QDir(u"board"_s).exists())
  {
    if (!QDir::current().mkdir("board"))
    {
      qWarning("Error creating missing '%s/board' directory.", qPrintable(QDir::currentPath()));
    }
    else
    {
      qWarning("Created missing '%s/board' directory.", qPrintable(QDir::currentPath()));
    }
  }

  auto filename = QString(board->second.save_file);
  the_file = dc_fopen(filename, "w");

  if (!the_file)
  {
    logentry(u"Unable to create save file for bulletin board index at '%1'"_s.arg(QDir::currentPath() + "/" + filename), ANGEL,
             DC::LogChannel::LOG_BUG);
    return;
  }

  dc_fprintf(the_file, " %zu ", board->second.msgs.size());
  for (ind = 0; ind < board->second.msgs.size(); ind++)
  {
    write_me = board->second.msgs[ind].title;
    fwrite_string(write_me, the_file);

    write_me = board->second.msgs[ind].author;
    fwrite_string(write_me, the_file);

    write_me = board->second.msgs[ind].date;
    fwrite_string(write_me, the_file);

    write_me = board->second.msgs[ind].text;
    fwrite_string(write_me, the_file);
  }

  return;
}

void board_load_board()
{

  FILEPtr the_file;
  int ind;
  message curr_msg;
  int number;

  boards_t::iterator map_it;

  for (map_it = board_db.begin(); map_it != board_db.end(); map_it++)
  {
    map_it->second.lock = 0;
    map_it->second.locked_for = nullptr;

    the_file = dc_fopen((*map_it).second.save_file, "r");
    if (!the_file)
      continue;

    dc_fscanf(the_file, " %d ", &number);
    if (number < 0 || dc_feof(the_file))
    {

      continue;
    }

    for (ind = 0; ind < number; ind++)
    {

      curr_msg.title = fread_string(the_file, 27);
      curr_msg.author = fread_string(the_file, 27);
      curr_msg.date = fread_string(the_file, 27);
      curr_msg.text = fread_string(the_file, 27);

      map_it->second.msgs.push_back(curr_msg);
    }
  }
}

int board_do_read(Character *ch, QStringList arguments, boards_t::iterator board)
{
  QString buf, board_msg;
  auto numberStr = arguments.value(0);
  if (ch->isNonPlayer())
  {
    if (numberStr.isEmpty())
    {
      ch->sendln("Sorry, mobs have to specify the number of the post they want to read.");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    if (numberStr.isEmpty() && ch->player->last_mess_read > 0)
    {
      ch->player->last_mess_read++;
      numberStr = QString::number(ch->player->last_mess_read);
    }
  }

  bool ok = false;
  unsigned int tmessage = numberStr.toUInt(&ok);
  if (numberStr.isEmpty() || !ok)
  {
    ch->sendln("Read what?");
    return ReturnValue::eFAILURE;
  }

  if (board->second.type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->second.owner)
    {
      ch->sendln("You aren't in the right clan bucko.");
      return ReturnValue::eSUCCESS;
    }
    if (!has_right(ch, CLAN_RIGHTS_B_READ))
    {
      ch->sendln("You don't have the right!  Talk to your clan leader.");
      return ReturnValue::eSUCCESS;
    }
  }
  if (board->second.type == CLASS_BOARD && !ch->isImmortalPlayer() && GET_CLASS(ch) != board->second.owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return ReturnValue::eSUCCESS;
  }

  if ((ch->getLevel() < board->second.min_read_level))
  {
    send_to_char("You try and look at the messages on the board but"
                 " you\ncannot comprehend their meaning.\n\n",
                 ch);
    act("$n tries to read the board, but looks bewildered.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  if (board->second.msgs.empty())
  {
    ch->sendln("The board is empty!");
    return ReturnValue::eSUCCESS;
  }

  if (tmessage >= board->second.msgs.size())
  {
    ch->sendln(u"That message with ID %1 doesn't exist."_s.arg(tmessage));
    switch (board->second.msgs.size())
    {
    case 0:
      ch->sendln(u"There are no messages."_s);
      break;
    case 1:
      ch->sendln(u"0 is the only valid ID."_s);
      break;
    default:
      ch->sendln(u"Valid IDs are %2-%3."_s.arg(tmessage).arg(0).arg(board->second.msgs.size() - 1));
      break;
    }

    return ReturnValue::eSUCCESS;
  }

  if (ch->isPlayer())
    ch->player->last_mess_read = tmessage;

  buf = u"$n reads message %1 titled: %2"_s.arg(tmessage).arg(board->second.msgs[tmessage].title);
  act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL);

  if (ch->isNonPlayer() || isSet(ch->player->toggles, Player::PLR_ANSI))
  {
    buf = u"Message %1 (%2): $4$B%3 $5- %4$R"_s
              .arg(tmessage)
              .arg(board->second.msgs[tmessage].date)
              .arg(board->second.msgs[tmessage].author)
              .arg(board->second.msgs[tmessage].title);
    board_msg += buf;
  }
  else
  {
    buf = u"Message %1 (%2): %3 - %4"_s
              .arg(tmessage)
              .arg(board->second.msgs[tmessage].date)
              .arg(board->second.msgs[tmessage].author)
              .arg(board->second.msgs[tmessage].title);
    board_msg += buf;
  }

  buf = u"\n----------\n$3%1$R"_s.arg(board->second.msgs[tmessage].text);
  board_msg += buf;

  page_string(ch->desc, qPrintable(board_msg), 1);
  return ReturnValue::eSUCCESS;
}

int board_do_look(Character *ch, QStringList arguments, boards_t::iterator board)
{
  int i{};
  QString board_msg;
  QString buf;
  auto arg = arguments.value(0);
  if (board->second.type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->second.owner)
    {
      ch->sendln("You aren't in the right clan bucko.");
      return ReturnValue::eSUCCESS;
    }
    if (!has_right(ch, CLAN_RIGHTS_B_READ))
    {
      ch->sendln("You don't have the right!  Talk to your clan leader.");
      return ReturnValue::eSUCCESS;
    }
  }
  if (board->second.type == CLASS_BOARD && !ch->isImmortalPlayer() && GET_CLASS(ch) != board->second.owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return ReturnValue::eSUCCESS;
  }

  if ((ch->getLevel() < board->second.min_read_level))
  {
    send_to_char("You try and look at the messages on the board "
                 "but you\ncannot comprehend their meaning.\n",
                 ch);
    act("$n tries to read the board, but looks bewildered.", ch, 0, 0,
        TO_ROOM, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  act("$n studies the board.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  ch->sendln("This is a bulletin board. Usage: READ/ERASE <mesg #>, WRITE <header>");

  if (board->second.msgs.isEmpty())
  {
    ch->sendln("The board is empty.");
  }
  else
  {
    ch->sendln(u"There are %1 messages on the board."_s.arg(board->second.msgs.size() - 1));
    ch->sendln(u"Board Topic: %1"_s.arg(board->second.msgs[0].text));
  }

  i = board->second.msgs.size() - 1;
  for (auto msg_it = board->second.msgs.rbegin(); (i > 0) && (msg_it < board->second.msgs.rend()); ++msg_it)
  {
    auto truncated_date = msg_it->date;
    truncated_date.truncate(11);

    if (ch->isNonPlayer() || isSet(ch->player->toggles, Player::PLR_ANSI))
    {
      buf = u"(%1) $5%2 $4%3: $2%4$R\n"_s
                .arg(truncated_date)
                .arg(msg_it->author, -14)
                .arg(i--, 2)
                .arg(msg_it->title, -47);
      board_msg += buf;
      //         csendf(ch, "(%s) "YELLOW"%-14s "RED"%2d: "GREEN"%.47s"NTEXT"\n", msg_it->date,
      //                    msg_it->author,i--, msg_it->title);
    }
    else
    {
      buf = u"(%1) %2 %3: %4\n"_s
                .arg(truncated_date)
                .arg(msg_it->author, -14)
                .arg(i--, 2)
                .arg(msg_it->title, -47);
      board_msg += buf;
    }
  }
  board_save_board(board);
  page_string(ch->desc, qPrintable(board_msg), 1);
  return ReturnValue::eSUCCESS;
}

int fwrite_string(char *buf, FILEPtr fl)
{
  return dc_fprintf(fl, "%s~\n", buf);
}

int fwrite_string(QString buf, FILEPtr fl)
{
  return dc_fprintf(fl, "%s~\n", qPrintable(buf));
}