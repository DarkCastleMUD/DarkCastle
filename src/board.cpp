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
#include "DC/DC.h"

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
  CharacterPtr locked_for = {};
  bool lock = {};
  qint32 min_read_level = {};
  qint32 min_write_level = {};
  qint32 min_remove_level = {};
  qint32 type = {};
  qint32 owner = {};
  QString save_file;
  QList<message> msgs;
};

// These are the binary files in which to save/load messages

void board_write_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board);
qint32 board_display_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board);
qint32 board_remove_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board);
void board_save_board(QMap<QString, BOARD_INFO>::iterator board);
void board_load_board();
qint32 board_show_board(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board);
qint32 fwrite_string(const QString buf, FILE *stream);
void new_edit_board_unlock_board(CharacterPtr ch, qint32 abort);

constexpr auto ANY_BOARD = 0;
constexpr auto CLASS_BOARD = 1;
constexpr auto CLAN_BOARD = 2;

constexpr auto NO_OWNER = -1;
constexpr auto CLAN_ULNHYRR = 1;
constexpr auto CLAN_DARKTIDE = 2;
constexpr auto CLAN_ARCANA = 3;
constexpr auto CLAN_DARKENED = 4;
constexpr auto CLAN_DCGUARD = 5;
constexpr auto CLAN_TIMEWARP = 6;
constexpr auto CLAN_CONTINUUM = 7;
constexpr auto CLAN_MERC = 8;
constexpr auto CLAN_NAZGUL = 9;
constexpr auto CLAN_BLACKAXE = 10;
constexpr auto CLAN_TRIAD = 11;
constexpr auto CLAN_KOBAL = 12;
constexpr auto CLAN_SLACKERS = 13;
constexpr auto CLAN_KEHUA = 14;
constexpr auto CLAN_ASKANI = 15;
constexpr auto CLAN_HOUSELESSROGUES = 16;
constexpr auto CLAN_THEHORDE = 17;
constexpr auto CLAN_ANARCHIST = 18;
constexpr auto CLAN_SOLARIS = 19;
constexpr auto CLAN_SINDICATE = 20;
class RESERVATION_DATA
{
public:
  QString buf;
  message new_post;
  QMap<QString, BOARD_INFO>::iterator board;
};

// QMap to hold callback information for writing
QMap<CharacterPtr, RESERVATION_DATA *> wait_for_write;

/*
Function to populate the board_db with all of the current clan info
*/
QMap<QString, BOARD_INFO> populate_boards()
{
  QMap<QString, BOARD_INFO> board_tmp;
  BOARD_INFO board_struct = {};

  board_struct.min_read_level = {};
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

  board_struct.min_read_level = {};
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
  board_struct.owner = CLASS_ANTI_PAL;
  board_struct.save_file = "board/anti";
  board_tmp["board guild anti journal"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_PALADIN;
  board_struct.save_file = "board/pal";
  board_tmp["board guild paladin register"] = board_struct;

  board_struct.min_read_level = 1;
  board_struct.min_write_level = 1;
  board_struct.min_remove_level = IMMORTAL;
  board_struct.type = CLASS_BOARD;
  board_struct.owner = CLASS_BARBARIAN;
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

QMap<QString, BOARD_INFO> board_db = populate_boards();

command_return_t DC::save_boards(void)
{
  QSaveFile board_file("board/index");
  if (!board_file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
  {
    logentry(u"Unable to open/create save file for bulletin board index"_s, ANGEL,
             DC::LogChannel::LOG_BUG);
    return ReturnValue::eFAILURE;
  }
  QTextStream stream(&board_file);

  for (const auto &[key, value] : board_db.asKeyValueRange())
  {
    stream << value.min_read_level;
    stream << value.min_write_level;
    stream << value.min_remove_level;
    stream << value.type;
    stream << value.owner;
    stream << value.save_file;
    stream << key;
  }

  return ReturnValue::eSUCCESS;
}

/*
Entry function called from assign_proc.
handles commands and calls appropriate functions
*/
qint32 board(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr invoker)
{
  static qint32 has_loaded = {};

  QMap<QString, BOARD_INFO>::iterator board;

  if (cmd != cmd_t::LOOK && cmd != cmd_t::READ && cmd != cmd_t::WRITE && cmd != cmd_t::ERASE)
  {
    return ReturnValue::eFAILURE;
  }

  if (arg.isEmpty())
    return ReturnValue::eFAILURE;

  if (!ch->conn_)
    return ReturnValue::eFAILURE; /* By MS or all NPC's will be trapped at the board */

  if (!has_loaded)
  {
    board_load_board();
    has_loaded = 1;
  }

  // Identify which board we're dealing with

  if (!obj)
    return ReturnValue::eFAILURE;

  board = board_db.find(qPrintable(obj->name()));

  if (board == board_db.end())
    return ReturnValue::eFAILURE;

  QString arg1;
  one_argument(arg, arg1);

  if (!isexact(arg1, obj->name()) && cmd == cmd_t::LOOK)
    return ReturnValue::eFAILURE;

  switch (cmd)
  {
  case cmd_t::LOOK: // look
    return (board_show_board(ch, arg, board));
  case cmd_t::WRITE: // write
    if (GET_INT(ch) < 9)
    {
      ch->sendln("You are too stupid to know how to write!");
      return ReturnValue::eSUCCESS;
    }
    board_write_msg(ch, arg, board);
    return ReturnValue::eSUCCESS;
  case cmd_t::READ: // read
    if (GET_INT(ch) < 9)
    {
      ch->sendln("You are too stupid to know how to read!");
      return ReturnValue::eSUCCESS;
    }
    board_display_msg(ch, arg, board);
    return ReturnValue::eSUCCESS;
  case cmd_t::ERASE: /* erase */
    if (GET_INT(ch) < 9)
    {
      ch->sendln("You are too stupid to read them!\r\nDon't erase them they might be important!");
      return ReturnValue::eSUCCESS;
    }
    if (
        ((obj->name() == u"board uruk"_s) && ch->clan != CLAN_NAZGUL && ch->getLevel() < PATRON))
    {
      ch->sendln("You can't erase posts from this board.");
      return ReturnValue::eSUCCESS;
      // added this for Uruk'hai board so only members can remove posts
    }
    board_remove_msg(ch, arg, board);
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
void new_edit_board_unlock_board(CharacterPtr ch, qint32 abort)
{
  RESERVATION_DATA *reserve = wait_for_write[ch];
  message new_msg;

  new_msg.text = reserve->buf;
  reserve->buf = {};
  new_msg.date = reserve->new_post.date;
  new_msg.author = reserve->new_post.author;
  new_msg.title = reserve->new_post.title;

  reserve->board->msgs.push_back(new_msg);
  reserve = {};
}

void board_write_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board)
{
  QString buf;
  time_t timep; // clock time
  QString tmstr;

  if (board->type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->owner)
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
  if (board->type == CLASS_BOARD && !ch->isImmortalPlayer() && GET_CLASS(ch) != board->owner)
  {
    ch->sendln("You do not understand the writings written on this board.");
    return;
  }

  if ((ch->getLevel() < board->min_write_level))
  {
    send_to_char("You pick up a quill to write, but realize "
                 "you're not powerful enough\r\nto submit "
                 "intelligent material to this board.\r\n",
                 ch);
    return;
  }

  // skip blanks

  arg = arg.trimmed();

  if (arg.isEmpty())
  {
    ch->sendln("Need a header, fool.");
    return;
  }

  RESERVATION_DATA *reserve = new RESERVATION_DATA;

  reserve->new_post.title = arg;

  reserve->new_post.author = ch->name();

  timep = time(0);
  tmstr = asctime(localtime(&timep));
  dc_sprintf(buf, "%.10s", tmstr);

  reserve->new_post.date = buf;

  send_to_char("        Write your message and stay within the line.  (/s saves /h for help)\r\n"
               "   |--------------------------------------------------------------------------------|\r\n",
               ch);

  act_to_room("$n starts to write a message.", ch, 0, 0, INVIS_NULL);

  // if the title is Topic, this clears the date to let us know that
  // during the callback the topic will need updated.
  if (!(dc_strcmp("Topic", arg)) && ch->getLevel() > IMMORTAL)
  {
    reserve->new_post.date.clear();
  }

  reserve->buf = {};
  reserve->board = board;
  wait_for_write[ch] = reserve;
  ch->conn_->connected = Connection::states::WRITE_BOARD;
  ch->conn_->strnew = &reserve->buf;
}

qint32 board_remove_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board)
{
  quint32 ind, tmessage;
  QString buf, number;

  one_argument(arg, number);

  if (number.isEmpty() || !isdigit(*number))
    return ReturnValue::eFAILURE;

  if (!(tmessage = dc_atoi(number)))
    return ReturnValue::eFAILURE;

  if (board->second.msgs.isEmpty())
  {
    ch->sendln("The board is empty!");
    return ReturnValue::eSUCCESS;
  }

  if (tmessage == 0 || tmessage >= board->second.msgs.size())
  {
    ch->sendln("That message exists only in your imagination..");
    return ReturnValue::eSUCCESS;
  }

  ind = tmessage;

  // if clan board
  if (board->second.type == CLAN_BOARD && ch->getLevel() < OVERSEER)
  {
    if (ch->clan != board->second.owner)
    {
      ch->sendln("You aren't in the right clan bucko.");
      return ReturnValue::eSUCCESS;
    }
    if (!has_right(ch, CLAN_RIGHTS_B_REMOVE) && board->second.msgs[ind].author.compare(qPrintable(ch->name())))
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
  else if ((ch->getLevel() < board->second.min_remove_level && board->second.msgs[ind].author.compare(qPrintable(ch->name()))) && ch->getLevel() < OVERSEER)
  {
    send_to_char("You try and grab one of the notes of the board but "
                 "get a nasty\r\nshock. Maybe you'd better leave it "
                 "alone.\r\n",
                 ch);
    return ReturnValue::eSUCCESS;
  }

  board->second.msgs.erase(board->second.msgs.begin() + ind);

  ch->sendln("Message erased.");
  dc_sprintf(buf, "$n just erased message %d.", tmessage);

  // Removal message also repaired
  act_to_room(buf, ch, 0, 0, INVIS_NULL);

  board_save_board(board);
  return ReturnValue::eSUCCESS;
}

QString remove_slashr(QString unformatted)
{
  if (unformatted.isEmpty())
    return "";

  QString write_me = unformatted;
  QString::iterator slashr;

  for (slashr = write_me.end(); slashr != write_me.begin(); slashr--)
    if (*slashr == '\r')
      write_me.erase(slashr);

  if (*slashr == '\r')
    write_me.erase(slashr);
  return write_me;
}

void board_save_board(QMap<QString, BOARD_INFO>::iterator board)
{

  FILE *the_file;
  QString write_me;
  quint32 ind;

  the_file = fopen(board->second.save_file.c_str(), "w");

  if (!the_file)
  {
    dc_->logentry(u"Unable to open/create save file for bulletin board"_s, ANGEL,
                  DC::LogChannel::LOG_BUG);
    return;
  }

  dc_fprintf(the_file, " %zu ", board->second.msgs.size());
  for (ind = {}; ind < board->second.msgs.size(); ind++)
  {
    write_me = remove_slashr(board->second.msgs[ind].title);
    fwrite_string(write_me.c_str(), the_file);

    write_me = remove_slashr(board->second.msgs[ind].author);
    fwrite_string(write_me.c_str(), the_file);

    write_me = remove_slashr(board->second.msgs[ind].date);
    fwrite_string(write_me.c_str(), the_file);

    write_me = remove_slashr(board->second.msgs[ind].text);
    fwrite_string(write_me.c_str(), the_file);
  }
}

void board_load_board()
{

  FILE *the_file;
  qint32 ind;
  message curr_msg;
  qint32 number;

  QMap<QString, BOARD_INFO>::iterator map_it;

  for (map_it = board_db.begin(); map_it != board_db.end(); map_it++)
  {
    map_it->second.lock = {};
    map_it->second.locked_for = {};

    the_file = fopen((*map_it).second.save_file.c_str(), "r");
    if (!the_file)
      continue;

    fscanf(the_file, " %d ", &number);
    if (number < 0 || feof(the_file))
    {

      continue;
    }

    for (ind = {}; ind < number; ind++)
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
  }
}

qint32 board_display_msg(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board)
{
  QString buf, number;
  QString board_msg;
  one_argument(arg, number);
  quint32 tmessage;

  if (ch->isNonPlayer())
  {
    if (number.isEmpty())
    {
      ch->sendln("Sorry, mobs have to specify the number of the post they want to read.");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    if (number.isEmpty() && ch->player->last_mess_read > 0)
    {
      ch->player->last_mess_read++;
      dc_sprintf(number, "%i", ch->player->last_mess_read);
    }
  }

  if (number.isEmpty() || !isdigit(*number))
  {
    ch->sendln("Read what?");
    return ReturnValue::eFAILURE;
  }
  if (!(tmessage = dc_atoi(number)))
    return ReturnValue::eFAILURE;

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
                 " you\r\ncannot comprehend their meaning.\r\n\r\n",
                 ch);
    act("$n tries to read the board, but looks bewildered.", ch, 0, 0,
        TO_ROOM, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  if (board->second.msgs.isEmpty())
  {
    ch->sendln("The board is empty!");
    return ReturnValue::eSUCCESS;
  }

  if (tmessage == 0 || tmessage >= board->second.msgs.size())
  {
    ch->sendln("That message doesn't exist, moron.");
    return ReturnValue::eSUCCESS;
  }

  if (!ch->isNonPlayer())
    ch->player->last_mess_read = tmessage;

  dc_sprintf(buf, "$n reads message %d titled: %s", tmessage, board->second.msgs[tmessage].title.c_str());
  act_to_room(buf, ch, 0, 0, INVIS_NULL);

  if (ch->isNonPlayer() || isSet(ch->player->toggles, Player::PLR_ANSI))
  {
    dc_snprintf(buf, MAX_STRING_LENGTH, "Message %2d (%s): " RED BOLD "%-14s " YELLOW "- %s" NTEXT,
                tmessage, board->second.msgs[tmessage].date.c_str(),
                board->second.msgs[tmessage].author.c_str(), board->second.msgs[tmessage].title.c_str());
    board_msg += buf;
  }
  else
  {
    dc_snprintf(buf, MAX_STRING_LENGTH, "Message %2d (%s): %-14s - %s", tmessage, board->second.msgs[tmessage].date.c_str(),
                board->second.msgs[tmessage].author.c_str(), board->second.msgs[tmessage].title.c_str());
    board_msg += buf;
  }

  dc_snprintf(buf, MAX_STRING_LENGTH, "\r\n----------\r\n" CYAN "%s" NTEXT, board->second.msgs[tmessage].text.c_str());
  board_msg += buf;

  page_string(ch->conn_, board_msg.c_str(), 1);
  return ReturnValue::eSUCCESS;
}

qint32 board_show_board(CharacterPtr ch, QString arg, QMap<QString, BOARD_INFO>::iterator board)
{
  qint32 i;
  QString board_msg;
  QString buf;
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
                 "but you\r\ncannot comprehend their meaning.\r\n",
                 ch);
    act("$n tries to read the board, but looks bewildered.", ch, 0, 0,
        TO_ROOM, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  act_to_room("$n studies the board.", ch, 0, 0, INVIS_NULL);

  ch->sendln("This is a bulletin board. Usage: READ/ERASE <mesg #>, WRITE <header>");
  if (board->second.msgs.isEmpty())
    ch->sendln("The board is empty.");
  else
  {
    ch->send(u"There are %d messages on the board.\r\n"_s.arg(board->second.msgs.size()));
    ;

    ch->send(u"Board Topic:\r\n%s------------\r\n"_s.arg(board->second.msgs[0].text.c_str()));
    QList<message>::reverse_iterator msg_it;
    i = board->second.msgs.size() - 1;
    for (msg_it = board->second.msgs.rbegin(); (i > 0) && (msg_it < board->second.msgs.rend()); ++msg_it)
      if (ch->isNonPlayer() || isSet(ch->player->toggles, Player::PLR_ANSI))
      {
        dc_snprintf(buf, MAX_STRING_LENGTH, "(%s) " YELLOW "%-14s " RED "%2d: " GREEN "%.47s" NTEXT "\r\n",
                    msg_it->date.c_str(), msg_it->author.c_str(), i--, msg_it->title.c_str());
        board_msg += buf;
        //         ch->send(u"(%s) "YELLOW"%-14s "RED"%2d: "GREEN"%.47s"NTEXT"\r\n"_s.arg(msg_it->date.c_str()).arg(       //                    msg_it->author.c_str(),i--).arg(msg_it->title.c_str()));
      }
      else
      {
        dc_snprintf(buf, MAX_STRING_LENGTH, "(%s) %-14s %2d: %.47s\r\n", msg_it->date.c_str(),
                    msg_it->author.c_str(), i--, msg_it->title.c_str());
        board_msg += buf;
      }
  }
  board_save_board(board);
  page_string(ch->conn_, board_msg.c_str(), 1);
  return ReturnValue::eSUCCESS;
}

qint32 fwrite_string(const QString buf, FILE *stream)
{
  return (dc_fprintf(stream, "%s~\n", buf));
}
