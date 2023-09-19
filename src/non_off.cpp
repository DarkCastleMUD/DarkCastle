/************************************************************************
| $Id: non_off.cpp,v 1.62 2011/08/28 18:24:54 jhhudso Exp $
| non_off.C
| Description:  Implementation of generic, non-offensive commands.
*/
/*****************************************************************************/
/* Revision History                                                          */
/* 12/08/2003   Onager   Revised do_tap() to prevent sacrifices in donations */
/*****************************************************************************/
extern "C"
{
#include <ctype.h>
#include <string.h>
}
#include "connect.h"
#include "character.h"
#include "room.h"
#include "obj.h"
#include "mobile.h"
#include "utility.h"
#include "levels.h"
#include "handler.h"
#include "db.h"
#include "interp.h"
#include "player.h"
#include "act.h"
#include "spells.h"
#include "fight.h"
#include "returnvals.h"
#include "comm.h"
#include "structs.h"
#include "utility.h"
#include "fileinfo.h"
#include <string>
#include <vector>
#include <map>
#include <set>


extern struct index_data *obj_index;
extern CVoteData *DCVote;

// decay variable means it's from a decaying corpse, not a player
void log_sacrifice(Character *ch, Object *obj, bool decay = false)
{

  if (GET_OBJ_RNUM(obj) == DC::NOWHERE)
    return;

  if (!decay)
  {
    logf(IMPLEMENTER, LogChannels::LOG_OBJECTS, "%s just sacrificed %s[%d] in room %d\n", GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), GET_ROOM_VNUM(ch->in_room));
  }
  else
  {
    logf(IMPLEMENTER, LogChannels::LOG_OBJECTS, "%s just poofed from decaying corpse %s[%d] in room %d\n", GET_OBJ_SHORT((Object *)ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), GET_ROOM_VNUM(obj->in_room));
  }

  for (Object *loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
  {
    logf(IMPLEMENTER, LogChannels::LOG_OBJECTS, "The %s contained %s[%d]\n",
         GET_OBJ_SHORT(obj),
         GET_OBJ_SHORT(loop_obj),
         GET_OBJ_VNUM(loop_obj));
  }
}

int do_sacrifice(Character *ch, char *argument, int cmd)
{
  class Object *obj;
  char name[MAX_INPUT_LENGTH + 1];

  if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if (!*name || !str_cmp(name, GET_NAME(ch)))
  {
    act("$n offers $mself to $s god, who graciously declines.", ch, 0, 0, TO_ROOM, 0);
    act("Your god appreciates your offer and may accept it later.", ch, 0, 0, TO_CHAR, 0);
    return eSUCCESS;
  }

  obj = get_obj_in_list_vis(ch, name, ch->carrying);

  /* Ok, lets see if it's a corpse on the ground then */
  if (obj == nullptr)
  {
    obj = get_obj_in_list_vis(ch, name, DC::getInstance()->world[ch->in_room].contents);
    if (obj == nullptr || GET_ITEM_TYPE(obj) != ITEM_CONTAINER || !isname("corpse", obj->name) || isname("pc", obj->name))
    {
      act("You don't seem to be holding that object.", ch, 0, 0, TO_CHAR, 0);
      return eFAILURE;
    }
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP))
  {
    if (GET_LEVEL(ch) < IMMORTAL)
    {
      send_to_char("You are unable to destroy this item, it must be CURSED!\n\r", ch);
      return eFAILURE;
    }
    else
      send_to_char("(This item is cursed, BTW.)\n\r", ch);
  }

  if (obj->obj_flags.value[3] == 1 && isname("pc", obj->name))
  {
    send_to_char("You probably don't *really* want to do that.\r\n", ch);
    return eFAILURE;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) && GET_LEVEL(ch) < ANGEL)
  {
    send_to_char("God, what a stupid fucking thing for you to do.\r\n", ch);
    return eFAILURE;
  }

  if (obj_index[obj->item_number].virt == CHAMPION_ITEM)
  {
    send_to_char("In soviet russia, champion flag sacrifice YOU!\r\n", ch);
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CANTQUIT) && !IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF))
  {
    send_to_char("Your criminal acts prohibit it.\r\n", ch);
    return eFAILURE;
  }

  /* don't let people sac stuff in donations */
  if (ch->in_room == real_room(3099))
  {
    send_to_char("Not in the donation room.\r\n", ch);
    return (eFAILURE);
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_LIMIT_SACRIFICE) && obj->contains)
  {
    act("You attempt to sacrifice $p to the gods but they refuse your foolish gift. Empty it first.", ch, obj, 0, TO_CHAR, 0);
    act("$n attempts to foolishly sacrifices $p to $s god.", ch, obj, 0, TO_ROOM, 0);
    return eFAILURE;
  }

  act("$n sacrifices $p to $s god.", ch, obj, 0, TO_ROOM, 0);
  act("You sacrifice $p to the gods and receive one $B$5gold$R coin.", ch, obj, 0, TO_CHAR, 0);
  ch->addGold(1);
  log_sacrifice(ch, obj);
  extract_obj(obj);
  return eSUCCESS;
}

int do_visible(Character *ch, char *argument, int cmd)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
  {
    affect_from_char(ch, SPELL_INVISIBLE);
    send_to_char("You drop your invisiblity spell.\r\n", ch);
    if (!IS_AFFECTED(ch, AFF_INVISIBLE))
      act("$n slowly fades into existence.", ch, 0, 0, TO_ROOM, 0);
    else
      send_to_char("You must remove the equipment making you invis to become visible.\r\n", ch);
    return eSUCCESS;
  }

  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    send_to_char("You must remove the equipment making you invis to become visible.\r\n", ch);
  else
    send_to_char("You aren't invisible.\r\n", ch);

  return eSUCCESS;
}

int do_donate(Character *ch, char *argument, int cmd)
{
  class Object *obj;
  char name[MAX_INPUT_LENGTH + 1];
  char buf[MAX_STRING_LENGTH];
  int location;
  int room = 3099;
  int origin;

  if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  if (ch->fighting)
  {
    send_to_char("Aren't we a little to busy for that right now?\n\r", ch);
    return eFAILURE;
  }

  one_argument(argument, name);

  if (!*name)
  {
    send_to_char("Donate what?\n\r", ch);
    return eFAILURE;
  }

  obj = get_obj_in_list_vis(ch, name, ch->carrying);
  if (obj == nullptr)
  {
    sprintf(buf, "You don't have any '%s' to donate.", name);
    act(buf, ch, 0, 0, TO_CHAR, 0);
    return eFAILURE;
  }

  if (IS_AFFECTED(ch, AFF_CANTQUIT) && !IS_MOB(ch) && affected_by_spell(ch, FUCK_PTHIEF))
  {
    send_to_char("Your criminal acts prohibit it.\r\n", ch);
    return eFAILURE;
  }

  // Handle yielding the champion flag
  if (GET_OBJ_VNUM(obj) == 45)
  {
    if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
    {
      if (IS_AFFECTED(ch, AFF_CHAMPION))
      {
        REMBIT(ch->affected_by, AFF_CHAMPION);

        sprintf(buf, "\n\r##%s has just yielded the Champion flag!\n\r",
                GET_NAME(ch));
        send_info(buf);

        struct affected_type af;
        af.type = OBJ_CHAMPFLAG_TIMER;
        af.duration = 5;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = -1;
        affect_to_char(ch, &af);

        act("$n yields $p.", ch, obj, 0, TO_ROOM, 0);
        act("You yield $p.", ch, obj, 0, TO_CHAR, 0);

        location = real_room(CFLAG_HOME);
        origin = ch->in_room;
        move_char(ch, location);

        act("$p falls from the heavens...", ch, obj, 0, TO_ROOM, INVIS_NULL);

        move_char(ch, origin);
        move_obj(obj, location);

        ch->save(0);
        return eSUCCESS;
      }
      else
      {
        sprintf(buf, "%s had the champion flag, but no AFF_CHAMPION.",
                GET_NAME(ch));
        logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
        return eFAILURE;
      }
    }
    else
    {
      send_to_char("You can only yield the Champion flag from a safe room.\r\n", ch);
      return eFAILURE;
    }
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP))
  {
    send_to_char("Since you can't let go of it, how are you going to donate it?\n\r", ch);
    return eFAILURE;
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE))
  {
    if (GET_LEVEL(ch) > IMMORTAL)
    {
      send_to_char("That was a NO_TRADE item btw....\r\n", ch);
    }
    else
    {
      send_to_char("It seems magically attached to you.\r\n", ch);
      return eFAILURE;
    }
  }

  if (contains_no_trade_item(obj))
  {
    if (GET_LEVEL(ch) > IMMORTAL)
    {
      send_to_char("That was a NO_TRADE item btw....\r\n", ch);
    }
    else
    {
      send_to_char("Something inside it seems magically attached to you.\r\n", ch);
      return eFAILURE;
    }
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    send_to_char("You can't donate godload equipment.\r\n", ch);
    return eFAILURE;
  }

  act("$n donates $p.", ch, obj, 0, TO_ROOM, 0);
  act("You donate $p.", ch, obj, 0, TO_CHAR, 0);

  if (obj->obj_flags.type_flag != ITEM_MONEY)
  {
    char log_buf[MAX_STRING_LENGTH] = {};
    sprintf(log_buf, "%s donates %s[%d]", GET_NAME(ch), obj->name, obj_index[obj->item_number].virt);
    logentry(log_buf, IMPLEMENTER, LogChannels::LOG_OBJECTS);
    for (Object *loop_obj = obj->contains; loop_obj; loop_obj = loop_obj->next_content)
      logf(IMPLEMENTER, LogChannels::LOG_OBJECTS, "The %s contained %s[%d]", obj->short_description,
           loop_obj->short_description,
           obj_index[loop_obj->item_number].virt);
  }

  location = real_room(room);
  origin = ch->in_room;
  move_char(ch, location, false);

  act("$n has made a donation...", ch, obj, 0, TO_ROOM, 0);
  act("$p falls through a glowing white portal in the top of the ceiling.",
      ch, obj, 0, TO_ROOM, INVIS_NULL);

  move_char(ch, origin, false);
  move_obj(obj, location);

  ch->save(0);
  return eSUCCESS;
}

int do_title(Character *ch, char *argument, int cmd)
{
  char buf[100];
  int ctr;

  if (!*argument)
  {
    send_to_char("Change your title to what?\n\r", ch);
    return eFAILURE;
  }

  if (!IS_MOB(ch) && IS_SET(ch->player->punish, PUNISH_NOTITLE))
  {
    send_to_char("You can't do that.  You must have been naughty.\r\n", ch);
    return eFAILURE;
  }

  for (; isspace(*argument); argument++)
    ;

  if (strlen(argument) > 40)
  {
    send_to_char("Title field too big.  40 characters max.\r\n", ch);
    return eFAILURE;
  }

  if (strchr(argument, '[') || strchr(argument, ']'))
  {
    send_to_char("You cannot have a '[' or a ']' in your title.\r\n", ch);
    return eFAILURE;
  }

  // TODO - decide if we still need this anymore since I think the $color code
  // keeps mortals from using $'s anyway  No idea why we don't let them use ?'s offhand
  for (ctr = 0; (unsigned)ctr <= strlen(argument); ctr++)
  {
    if (((argument[ctr] == '$') && (argument[ctr + 1] == '$')) || ((argument[ctr] == '?') && (argument[ctr + 1] == '?')))
    {
      send_to_char("Your title is now: Common Dork of Dark Castle.\r\n", ch);
      return eFAILURE;
    }
  }

  if (ch->title) // this should always be true, but why not check anyway?
    dc_free(ch->title);
  ch->title = str_dup(argument);
  sprintf(buf, "Your title is now: %s\n\r", argument);
  send_to_char(buf, ch);
  return eSUCCESS;
}

char *toggle_txt[] = {
    "brief",
    "compact",
    "beep",
    "anonymous",
    "ansi",
    "vt100",
    "wimpy",
    "pager",
    "bard-songs",
    "auto-eat",
    "summonable",
    "lfg",
    "charmiejoin",
    "notax",
    "guide",
    "news-up",
    "ascii",
    "damage",
    "nodupekeys",
    ""};

int do_toggle(Character *ch, char *arg, int cmd)
{
  int x;
  char buf[MAX_STRING_LENGTH];

  one_argument(arg, buf);

  if (IS_MOB(ch))
  {
    send_to_char("You can't toggle anything, you're a mob!\r\n", ch);
    return eFAILURE;
  }

  if (!*buf)
  {
    for (x = 0; toggle_txt[x][0] != '\0'; x++)
    {
      if (x != 14 || (IS_SET(ch->player->toggles, PLR_GUIDE)))
      {
        sprintf(buf + strlen(buf), "%-11s ", toggle_txt[x]);
      }

      switch (x)
      {
      case 0:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_BRIEF) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 1:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_COMPACT) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 2:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_BEEP) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 3:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_ANONYMOUS) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 4:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_ANSI) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 5:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_VT100) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 6:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_WIMPY) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 7:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_PAGER) ? "$B$4off$R" : "$B$2on$R");
        break;

      case 8:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_BARD_SONG) ? "$B$2on$R (brief)" : "$B$4off$R (verbose)");
        break;

      case 9:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_AUTOEAT) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 10:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_SUMMONABLE) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 11:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_LFG) ? "$B$2on$R" : "$B$4off$R");
        break;

      case 12:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_CHARMIEJOIN) ? "$B$2on$R" : "$B$4off$R");
        break;
      case 13:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_NOTAX) ? "$B$2on$R" : "$B$4off$R");
        break;
      case 14:
        if (IS_SET(ch->player->toggles, PLR_GUIDE))
        {
          sprintf(buf + strlen(buf), "%s\n\r",
                  IS_SET(ch->player->toggles, PLR_GUIDE_TOG) ? "$B$2on$R" : "$B$4off$R");
        }
        break;
      case 15:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_NEWS) ? "$B$2on$R" : "$B$4off$R");
        break;
      case 16:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_ASCII) ? "$B$4off$R" : "$B$2on$R");
        break;
      case 17:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_DAMAGE) ? "$B$2on$R" : "$B$4off$R");
        break;
      case 18:
        sprintf(buf + strlen(buf), "%s\n\r",
                IS_SET(ch->player->toggles, PLR_NODUPEKEYS) ? "$B$2on$R" : "$B$4off$R");
        break;
      default:
        break;
      }
    }

    send_to_char(buf, ch);
    return eSUCCESS;
  }

  for (x = 0; toggle_txt[x][0] != '\0'; x++)
    if (is_abbrev(buf, toggle_txt[x]))
      break;

  if (toggle_txt[x][0] == '\0')
  {
    send_to_char("Bad option.  Type toggle with no arguments for a list of "
                 "good ones.\r\n",
                 ch);
    return eFAILURE;
  }

  switch (x)
  {
  case 0:
    do_brief(ch, "", CMD_DEFAULT);
    break;

  case 1:
    do_compact(ch, "", CMD_DEFAULT);
    break;

  case 2:
    do_beep_set(ch, "", CMD_DEFAULT);
    break;

  case 3:
    do_anonymous(ch, "", CMD_DEFAULT);
    break;

  case 4:
    do_ansi(ch, "", CMD_DEFAULT);
    break;

  case 5:
    do_vt100(ch, "", CMD_DEFAULT);
    break;

  case 6:
    do_wimpy(ch, "", CMD_DEFAULT);
    break;

  case 7:
    do_pager(ch, "", CMD_DEFAULT);
    break;

  case 8:
    do_bard_song_toggle(ch, "", CMD_DEFAULT);
    break;

  case 9:
    do_autoeat(ch, "", CMD_DEFAULT);
    break;

  case 10:
    do_summon_toggle(ch, "", CMD_DEFAULT);
    break;

  case 11:
    do_lfg_toggle(ch, "", CMD_DEFAULT);
    break;

  case 12:
    do_charmiejoin_toggle(ch, "", CMD_DEFAULT);
    break;

  case 13:
    do_notax_toggle(ch, "", CMD_DEFAULT);
    break;

  case 14:
    if (IS_SET(ch->player->toggles, PLR_GUIDE))
      do_guide_toggle(ch, "", CMD_DEFAULT);
    else
      send_to_char("You're not a guide!\r\n", ch);
    break;

  case 15:
    do_news_toggle(ch, "", CMD_DEFAULT);
    break;

  case 16:
    do_ascii_toggle(ch, "", CMD_DEFAULT);
    break;
  case 17:
    do_damage_toggle(ch, "", CMD_DEFAULT);
    break;
  case 18:
    do_nodupekeys_toggle(ch, "", CMD_DEFAULT);
    break;
  default:
    send_to_char("A bad thing just happened.  Tell the gods.\r\n", ch);
    break;
  }
  return eSUCCESS;
}

int Character::do_config(QStringList arguments, int cmd)
{
  if (player->config == nullptr)
  {
    player->config = new PlayerConfig();
  }

  if (arguments.isEmpty())
  {
    for (auto setting = player->config->constBegin(); setting != player->config->constEnd(); ++setting)
    {
      send(QString("%1=%2\r\n").arg(setting.key()).arg(setting.value()));
    }
    return eSUCCESS;
  }

  QMap<QString, QString> colors;
  // colors["black"]="$0";
  colors["blue"] = "$1";
  colors["green"] = "$2";
  colors["cyan"] = "$3";
  colors["red"] = "$4";
  colors["yellow"] = "$5";
  colors["magenta"] = "$6";
  colors["white"] = "$7";
  colors["gray"] = "$B$0";
  colors["bright blue"] = "$B$1";
  colors["bright green"] = "$B$2";
  colors["bright cyan"] = "$B$3";
  colors["bright red"] = "$B$4";
  colors["bright yellow"] = "$B$5";
  colors["bright magenta"] = "$B$6";
  colors["bright white"] = "$B$7";

  if (arguments.size() > 0 && arguments.at(0) == "help")
  {
    send("Usage:\r\n");
    send("config                       - Show all currently set configuration options.\r\n");
    send("config color.good=color name - Set color to use for \"good\" values in game.\r\n");
    send("config color.bad=color name  - Set color to use for \"bad\" values in game.\r\n");
    send("                               Use ? as color name to see valid colors.\r\n");
    send("config color.good=           - Unset color.good. Will use default \"good\" color.\r\n");
    send("config color.bad=            - Unset color.bad. Will use default \"bad\" color.\r\n\r\n");
    return eSUCCESS;
  }

  QString argument1 = arguments.at(0);
  QStringList setting = argument1.split('=');
  QString key, value;
  if (setting.size() >= 1)
  {
    key = setting.at(0);
  }
  if (setting.size() == 2)
  {
    value = setting.at(1);
  }

  // config
  // config key
  if (setting.size() < 2)
  {
    bool found = false;
    for (auto i = player->config->begin(); i != player->config->end(); ++i)
    {
      if (key == i.key() || key.isEmpty() || i.key().startsWith(key))
      {
        found = true;
        send(QString("%1=%2\r\n").arg(i.key()).arg(i.value()));
      }
    }

    if (found == false)
    {
      if (key.isEmpty() == false)
      {
        send(QString("%1 not found.\r\n").arg(key));
      }
      else
      {
        send("No config options set.\r\n");
      }
      return eFAILURE;
    }

    return eSUCCESS;
  }

  // config key=
  if (value.isEmpty() && key.isEmpty() == false && value.isEmpty() == true)
  {
    if (player->config->find(key) != player->config->end())
    {
      send(QString("%1 unset.\r\n").arg(key));
      player->config->insert(key, QString());
      return eSUCCESS;
    }
    send(QString("%1 not found.\r\n").arg(key));
    return eFAILURE;
  }

  // config key=value
  if (key.isEmpty() == false && value.isEmpty() == false)
  {
    if (key == "color.good" || key == "color.bad")
    {
      if (colors.find(value) == colors.end())
      {
        if (value == "?")
        {
          send("Valid colors:\r\n");
        }
        else
        {
          send("Invalid color specified. Valid colors:\r\n");
        }

        for (auto color = colors.constBegin(); color != colors.constEnd(); ++color)
        {
          if (color.key() == "black")
          {
            send(QString("%1\r\n").arg(color.key()));
          }
          else
          {
            send(QString("%1 - %2Example$R\r\n").arg(color.key(), -15).arg(color.value()));
          }
        }

        return eFAILURE;
      }
    }
    else if (key == "mode")
    {
      if (value.startsWith("char"))
      {
        telnet_sga(desc);
        telnet_echo_off(desc);
      }
      else if (value.startsWith("line") == false)
      {
        send("Valid telnet modes are line for linemode or char for character mode.\r\n");
        return eFAILURE;
      }
    }
    else if (key != "tell.history.timestamp" && key != "locale")
    {
      send("Invalid config option.\r\n");
      return eFAILURE;
    }

    player->config->insert(key, value);

    send(QString("Setting %1=%2\r\n").arg(key).arg(value));
    return eSUCCESS;
  }

  return eFAILURE;
}

int do_brief(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_BRIEF))
  {
    send_to_char("Brief mode $B$4off$R.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_BRIEF);
  }
  else
  {
    send_to_char("Brief mode $B$2on$R.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_BRIEF);
  }
  return eSUCCESS;
}

int do_ansi(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_ANSI))
  {
    send_to_char("ANSI COLOR $B$4off$R.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_ANSI);
  }
  else
  {
    send_to_char("ANSI COLOR $B$2on$R.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_ANSI);
  }
  return eSUCCESS;
}

int do_vt100(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_VT100))
  {
    send_to_char("VT100 $B$4off$R.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_VT100);
  }
  else
  {
    send_to_char("VT100 $B$2on$R.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_VT100);
  }
  return eSUCCESS;
}

int do_compact(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_COMPACT))
  {
    send_to_char("Compact mode $B$4off$R.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_COMPACT);
  }
  else
  {
    send_to_char("Compact mode $B$2on$R.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_COMPACT);
  }
  return eSUCCESS;
}

int do_summon_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_SUMMONABLE))
  {
    send_to_char("You may no longer be summoned by other players.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_SUMMONABLE);
  }
  else
  {
    send_to_char("You may now be summoned by other players.\r\n"
                 "Make _sure_ you want this...they could summon you to your death!\r\n",
                 ch);
    SET_BIT(ch->player->toggles, PLR_SUMMONABLE);
  }
  return eSUCCESS;
}

int do_lfg_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_LFG))
  {
    send_to_char("You are no longer Looking For Group.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_LFG);
  }
  else
  {
    send_to_char("You are now Looking For Group.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_LFG);
  }
  return eSUCCESS;
}

int do_guide_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (!IS_SET(ch->player->toggles, PLR_GUIDE))
  {
    send_to_char("You must be assigned as a $BGuide$R by the gods before you can toggle it.\r\n", ch);
    return eFAILURE;
  }

  if (IS_SET(ch->player->toggles, PLR_GUIDE_TOG))
  {
    send_to_char("You have hidden your $B(Guide)$R tag.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_GUIDE_TOG);
  }
  else
  {
    send_to_char("You will now show your $B(Guide)$R tag.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_GUIDE_TOG);
  }

  return eSUCCESS;
}
int do_news_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_NEWS))
  {
    send_to_char("You now view news in an up-down fashion.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_NEWS);
  }
  else
  {
    send_to_char("You now view news in a down-up fashion..\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_NEWS);
  }

  return eSUCCESS;
}

int do_ascii_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_ASCII))
  {
    REMOVE_BIT(ch->player->toggles, PLR_ASCII);
    send_to_char("Cards are now displayed through ASCII.\r\n", ch);
  }
  else
  {
    send_to_char("Cards are no longer dislayed through ASCII.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_ASCII);
  }

  return eSUCCESS;
}

int do_damage_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_DAMAGE))
  {
    REMOVE_BIT(ch->player->toggles, PLR_DAMAGE);
    send_to_char("Damage numbers will no longer be displayed in combat.\r\n", ch);
  }
  else
  {
    send_to_char("Damage numbers will now be displayed in combat.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_DAMAGE);
  }

  return eSUCCESS;
}

int do_notax_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_NOTAX))
  {
    send_to_char("You will now be taxed on all your loot.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_NOTAX);
  }
  else
  {
    send_to_char("You will no longer be taxed.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_NOTAX);
  }

  return eSUCCESS;
}

int do_charmiejoin_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_CHARMIEJOIN))
  {
    send_to_char("Your followers will no longer automatically join you.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_CHARMIEJOIN);
  }
  else
  {
    send_to_char("Your followers will automatically aid you in battle.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_CHARMIEJOIN);
  }

  return eSUCCESS;
}

int do_autoeat(Character *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_AUTOEAT))
  {
    send_to_char("You no longer automatically eat and drink.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_AUTOEAT);
  }
  else
  {
    send_to_char("You now automatically eat and drink when hungry and thirsty.\r\n", ch);
    SET_BIT(ch->player->toggles, PLR_AUTOEAT);
  }
  return eSUCCESS;
}

int do_anonymous(Character *ch, char *argument, int cmd)
{
  if (ch == 0)
  {
    logentry("Null char in do_anonymous.", OVERSEER, LogChannels::LOG_BUG);
    return eFAILURE;
  }
  if (GET_LEVEL(ch) < 40)
  {
    send_to_char("You are too inexperienced to disguise your profession.\r\n", ch);
    return eSUCCESS;
  }
  if (IS_SET(ch->player->toggles, PLR_ANONYMOUS))
  {
    send_to_char("Your class and level information is now public.\r\n", ch);
  }
  else
  {
    send_to_char("Your class and level information is now private.\r\n", ch);
  }

  TOGGLE_BIT(ch->player->toggles, PLR_ANONYMOUS);
  return eSUCCESS;
}

int do_wimpy(Character *ch, char *argument, int cmd)
{
  if (IS_SET(ch->player->toggles, PLR_WIMPY))
  {
    send_to_char("You are no longer a wimp....maybe.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_WIMPY);
    return eFAILURE;
  }

  send_to_char("You are now an official wimp.\r\n", ch);
  SET_BIT(ch->player->toggles, PLR_WIMPY);
  return eSUCCESS;
}

// Remember that his is "no-pager".  So if it's set, we don't page
// If it's not set, we do.
int do_pager(Character *ch, char *argument, int cmd)
{
  if (IS_SET(ch->player->toggles, PLR_PAGER))
  {
    send_to_char("You now page your strings in 24 line chunks.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_PAGER);
    return eFAILURE;
  }

  send_to_char("You no longer page strings in 24 line chunks.\r\n", ch);
  SET_BIT(ch->player->toggles, PLR_PAGER);
  return eSUCCESS;
}

int do_bard_song_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_SET(ch->player->toggles, PLR_BARD_SONG))
  {
    send_to_char("Bard singing now in verbose mode.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_BARD_SONG);
    return eFAILURE;
  }

  send_to_char("Bard singing now in brief mode.\r\n", ch);
  SET_BIT(ch->player->toggles, PLR_BARD_SONG);
  return eSUCCESS;
}

int do_nodupekeys_toggle(Character *ch, char *argument, int cmd)
{
  if (IS_SET(ch->player->toggles, PLR_NODUPEKEYS))
  {
    send_to_char("You will attach duplicate keys to keyrings.\r\n", ch);
    REMOVE_BIT(ch->player->toggles, PLR_NODUPEKEYS);
    return eFAILURE;
  }

  send_to_char("You will not attach duplicate keys to keyrings.\r\n", ch);
  SET_BIT(ch->player->toggles, PLR_NODUPEKEYS);
  return eSUCCESS;
}

int do_beep_set(Character *ch, char *arg, int cmd)
{
  if (IS_NPC(ch))
    return eFAILURE;

  if (IS_SET(ch->player->toggles, PLR_BEEP))
  {
    REMOVE_BIT(ch->player->toggles, PLR_BEEP);
    send_to_char("\nTell is now silent.\n", ch);
    return eFAILURE;
  }

  SET_BIT(ch->player->toggles, PLR_BEEP);
  send_to_char("\nTell now beeps.\a\n", ch);
  return eSUCCESS;
}

int do_stand(Character *ch, char *argument, int cmd)
{
  switch (GET_POS(ch))
  {
  case POSITION_STANDING:
  {
    act("You are already standing.", ch, 0, 0, TO_CHAR, 0);
  }
  break;
  case POSITION_SITTING:
  {
    act("You stand up.", ch, 0, 0, TO_CHAR, 0);
    act("$n clambers on $s feet.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    if (ch->fighting)
      ch->setPOSFighting();
    else
      GET_POS(ch) = POSITION_STANDING;
  }
  break;
  case POSITION_RESTING:
  {
    act("You stop resting, and stand up.", ch, 0, 0, TO_CHAR, 0);
    act("$n stops resting, and clambers on $s feet.",
        ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_STANDING;
  }
  break;
  case POSITION_SLEEPING:
  {
    act("You have to wake up first!", ch, 0, 0, TO_CHAR, 0);
  }
  break;
  case POSITION_FIGHTING:
  {
    act("Do you not consider fighting as standing?",
        ch, 0, 0, TO_CHAR, 0);
  }
  break;
  default:
  {
    act("You stop floating around, and put your feet on the ground.", ch, 0, 0, TO_CHAR, 0);
    act("$n stops floating around, and puts $s feet on the ground.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_STANDING;
  }
  break;
  }
  return eSUCCESS;
}

int do_sit(Character *ch, char *argument, int cmd)
{

  if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  switch (GET_POS(ch))
  {
  case POSITION_STANDING:
  {
    act("You sit down.", ch, 0, 0, TO_CHAR, 0);
    act("$n sits down.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_SITTING;
  }
  break;
  case POSITION_SITTING:
  {
    send_to_char("You're sitting already.\r\n", ch);
  }
  break;
  case POSITION_RESTING:
  {
    act("You stop resting, and sit up.", ch, 0, 0, TO_CHAR, 0);
    act("$n stops resting.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_SITTING;
  }
  break;
  case POSITION_SLEEPING:
  {
    act("You have to wake up first.", ch, 0, 0, TO_CHAR, 0);
  }
  break;
  case POSITION_FIGHTING:
  {
    act("Sit down while fighting? are you MAD?",
        ch, 0, 0, TO_CHAR, 0);
  }
  break;
  default:
  {
    act("You stop floating around, and sit down.",
        ch, 0, 0, TO_CHAR, 0);
    act("$n stops floating around, and sits down.",
        ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_SITTING;
  }
  break;
  }
  return eSUCCESS;
}

int do_rest(Character *ch, char *argument, int cmd)
{

  if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  switch (GET_POS(ch))
  {
  case POSITION_STANDING:
  {
    act("You sit down and rest your tired bones.",
        ch, 0, 0, TO_CHAR, 0);
    act("$n sits down and rests.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_RESTING;
  }
  break;
  case POSITION_SITTING:
  {
    act("You rest your tired bones.", ch, 0, 0, TO_CHAR, 0);
    act("$n rests.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_RESTING;
  }
  break;
  case POSITION_RESTING:
  {
    act("You are already resting.", ch, 0, 0, TO_CHAR, 0);
  }
  break;
  case POSITION_SLEEPING:
  {
    act("You have to wake up first.", ch, 0, 0, TO_CHAR, 0);
  }
  break;
  case POSITION_FIGHTING:
  {
    act("Rest while fighting? are you MAD?", ch, 0, 0, TO_CHAR, 0);
  }
  break;
  default:
  {
    act("You stop floating around, and stop to rest your tired bones.", ch, 0, 0, TO_CHAR, 0);
    act("$n stops floating around, and rests.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_SITTING;
  }
  break;
  }
  return eSUCCESS;
}

int do_sleep(Character *ch, char *argument, int cmd)
{
  struct affected_type *paf;
  if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }
  if (IS_AFFECTED(ch, AFF_INSOMNIA))
  {
    send_to_char("You are far too alert for that.\r\n", ch);
    return eFAILURE;
  }
  if (!IS_SET(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
    if (!check_make_camp(ch->in_room))
    {
      send_to_char("Be careful sleeping out here!  This isn't a safe room, so people can steal your equipment while you sleep!\r\n", ch);
    }

  if ((paf = affected_by_spell(ch, SPELL_SLEEP)) &&
      paf->modifier == 1 && GET_POS(ch) != POSITION_SLEEPING)
    paf->modifier = 0;

  switch (GET_POS(ch))
  {
  case POSITION_STANDING:
    send_to_char("You lie down and go to sleep.\r\n", ch);
    act("$n lies down and falls asleep.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_SLEEPING;
    break;
  case POSITION_SITTING:
  case POSITION_RESTING:
    send_to_char("You lay back and go to sleep.\r\n", ch);
    act("$n lies back and falls asleep.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_SLEEPING;
    break;
  case POSITION_SLEEPING:
  {
    send_to_char("You are already sound asleep.\r\n", ch);
    return eFAILURE; // so we don't set INTERNAL_SLEEPING
  }
  break;
  case POSITION_FIGHTING:
  {
    send_to_char("Sleep while fighting? Are you MAD?\n\r", ch);
    return eFAILURE; // so we don't set INTERNAL_SLEEPING
  }
  break;
  default:
  {
    act("You stop floating around, and lie down to sleep.", ch, 0, 0, TO_CHAR, 0);
    act("$n stops floating around, and lie down to sleep.", ch, 0, 0, TO_ROOM, INVIS_NULL);
    GET_POS(ch) = POSITION_SLEEPING;
  }
  break;
  }

  struct affected_type af;
  af.type = INTERNAL_SLEEPING;
  af.duration = 0;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);

  return eSUCCESS;
}

int do_wake(Character *ch, char *argument, int cmd)
{
  Character *tmp_char;
  char arg[MAX_STRING_LENGTH];
  struct affected_type *af;

  one_argument(argument, arg);
  if (*arg)
  {
    if (GET_POS(ch) == POSITION_SLEEPING)
    {
      act("You can't wake people up if you are asleep yourself!", ch, 0, 0, TO_CHAR, 0);
    }
    else
    {
      tmp_char = get_char_room_vis(ch, arg);
      if (tmp_char)
      {
        if (tmp_char == ch)
        {
          act("If you want to wake yourself up, just type 'wake'", ch, 0, 0, TO_CHAR, 0);
        }
        else
        {
          if (GET_POS(ch) == POSITION_FIGHTING)
          {
            if (GET_POS(tmp_char) == POSITION_SLEEPING)
            {
              if (number(1, 100) > GET_DEX(ch))
              {
                act("You cannot meneuver yourself over to $M!", ch, 0, tmp_char, TO_CHAR, 0);
                act("$n tries to move the flow of battle towards $N but is unable.",
                    ch, 0, tmp_char, TO_ROOM, 0);
                return eSUCCESS;
              }
              if ((af = affected_by_spell(tmp_char, SPELL_SLEEP)) && af->modifier == 1)
              {
                act("You can not wake $M up!", ch, 0, tmp_char, TO_CHAR, 0);
              }
              else
              {
                act("You manage to give $M a swift kick in the ribs.", ch, 0, tmp_char, TO_CHAR, 0);
                GET_POS(tmp_char) = POSITION_SITTING;
                act("$n awakens $N.", ch, 0, tmp_char, TO_ROOM, NOTVICT);
                act("$n wakes you up with a sharp kick to the ribs.  The sounds of battle ring in your ears.", ch, 0, tmp_char, TO_VICT, 0);
                affect_from_char(tmp_char, INTERNAL_SLEEPING);
              }
            }
            else
            {
              act("$N is already awake.", ch, 0, tmp_char, TO_CHAR, 0);
            }
          }
          else
          {
            if (GET_POS(tmp_char) == POSITION_SLEEPING)
            {
              if ((af = affected_by_spell(tmp_char, SPELL_SLEEP)) && af->modifier == 1)
              {
                act("You can not wake $M up!", ch, 0, tmp_char, TO_CHAR, 0);
              }
              else
              {
                act("You wake $M up.", ch, 0, tmp_char, TO_CHAR, 0);
                act("$n awakens $N.", ch, 0, tmp_char, TO_ROOM, NOTVICT);
                GET_POS(tmp_char) = POSITION_SITTING;
                act("You are awakened by $n.", ch, 0, tmp_char, TO_VICT, 0);
                affect_from_char(tmp_char, INTERNAL_SLEEPING);
              }
            }
            else
            {
              act("$N is already awake.", ch, 0, tmp_char, TO_CHAR, 0);
            }
          }
        }
      }
      else
      {
        send_to_char("You do not see that person here.\r\n", ch);
      }
    }
  }
  else
  {
    if (GET_POS(ch) > POSITION_SLEEPING)
      send_to_char("You are already awake...\r\n", ch);
    else if ((af = affected_by_spell(ch, SPELL_SLEEP)) && af->modifier == 1)
    {
      send_to_char("You can't wake up!\n\r", ch);
      //        } else if ((af = affected_by_spell(ch, INTERNAL_SLEEPING))) {
      //            send_to_char("You just went to sleep!  Your body is still too tired.  Your dreaming continues...\r\n", ch);
    }
    else
    {
      //            else {
      send_to_char("You wake, and stand up.\r\n", ch);
      act("$n awakens.", ch, 0, 0, TO_ROOM, 0);
      GET_POS(ch) = POSITION_STANDING;
      //          }
    }
  }
  return eSUCCESS;
}

// global tag var
Character *tagged_person;

int do_tag(Character *ch, char *argument, int cmd)
{
  char name[MAX_INPUT_LENGTH];
  Character *victim;

  one_argument(name, argument);

  if (!*name || !(victim = get_char_room_vis(ch, name)))
  {
    send_to_char("Tag who?\r\n", ch);
    return eFAILURE;
  }

  return eSUCCESS;
}

void CVoteData::DisplayVote(Character *ch)
{
  char buf[MAX_STRING_LENGTH];
  std::vector<SVoteData>::iterator answer_it;
  int i = 1;
  if (vote_question.empty())
  {
    ch->send("\n\rSorry! There are no active votes right now!\n\r\n\r");
    return;
  }
  ch->send("\n\r--Current Vote Infortmation--\n\rTo vote, type \"vote #\".\r\n"
           "Enter \"vote results\" to see the current voting demographics.\n\r\n\r");
  strncpy(buf, vote_question.c_str(), MAX_STRING_LENGTH);
  ch->send(buf);
  ch->send("\n\r");
  for (answer_it = answers.begin(); answer_it != answers.end(); answer_it++)
    ch->send(QString("%1: %2\r\n").arg(i++, 2).arg(answer_it->answer.c_str()));
  ch->send("\n\r");
}

void CVoteData::RemoveAnswer(Character *ch, unsigned int answer)
{
  if (active)
  {
    send_to_char("You have to end the current vote before you can remove answers.\r\n", ch);
    return;
  }
  if (answers.empty())
  {
    send_to_char("That answer doesn't exist!\n\r", ch);
    return;
  }
  if (answer > answers.size())
  {
    send_to_char("That answer doesn't exist!\n\r", ch);
    return;
  }
  std::vector<SVoteData>::iterator answer_it = answers.begin();
  answers.erase(answer_it + answer - 1); // need to offset by 1
  send_to_char("Answer removed!\n\r", ch);
}

void CVoteData::StartVote(Character *ch)
{
  if (active)
  {
    send_to_char("There is already an active vote, you can't start another\n\r", ch);
    return;
  }
  if (vote_question.empty())
  {
    send_to_char("You can't start a vote without a topic to vote on!\n\r", ch);
    return;
  }
  if (answers.empty())
  {
    send_to_char("You can't start a vote without any answers!\n\r", ch);
    return;
  }

  send_to_char("$4**MAKE SURE YOU VOTESET CLEAR IF THIS IS A NEW VOTE!**$R\r\n", ch);
  send_info("\n\r##Attention! There is now a vote in progress!\n\r##Type Vote for more information!\n\r");

  active = true;
  this->OutToFile();
  return;
}

void CVoteData::EndVote(Character *ch)
{
  if (!active)
  {
    send_to_char("Can't end a vote if there isn't one started.\r\n", ch);
    return;
  }

  active = false;
  this->OutToFile();
  send_info("\n\r##The vote has ended! Type \"Vote Results\" to see the results!\n\r");
}

void CVoteData::AddAnswer(Character *ch, std::string answer)
{
  if (active)
  {
    send_to_char("You can't add answers during an active vote!\n\r", ch);
    return;
  }
  send_to_char("Answer added.\r\n", ch);
  SVoteData tmp;
  tmp.votes = 0;
  tmp.answer = answer;
  answers.push_back(tmp);
}

bool CVoteData::HasVoted(Character *ch)
{
  return (ip_voted[ch->desc->getPeerOriginalAddress().toString().toStdString().c_str()] || char_voted[GET_NAME(ch)]);
}

bool CVoteData::Vote(Character *ch, unsigned int vote)
{
  if (!ch->desc)
  {
    send_to_char("Monsters don't get to vote!\n\r", ch);
    return false;
  }

  if (this->HasVoted(ch))
  {
    send_to_char("You have already voted!\n\r", ch);
    return false;
  }

  if (vote > answers.size() || vote == 0)
  {
    send_to_char("That answer doesn't exist.\r\n", ch);
    return false;
  }

  ip_voted[ch->desc->getPeerOriginalAddress().toString().toStdString().c_str()] = true;
  char_voted[GET_NAME(ch)] = true;
  total_votes++;
  answers.at(vote - 1).votes++;

  send_to_char("Vote sent!\n\r", ch);
  OutToFile();
  return true;
}

void CVoteData::DisplayResults(Character *ch)
{
  if (active && GET_LEVEL(ch) > 39 && !ip_voted[ch->desc->getPeerOriginalAddress().toString().toStdString().c_str()] && GET_LEVEL(ch) < IMMORTAL)
  {
    send_to_char("Sorry, but you have to cast a vote before you can see the results.\r\n", ch);
    return;
  }
  if (!total_votes)
  {
    send_to_char("There hasn't been any votes to view the results of.\r\n", ch);
    return;
  }
  char buf[MAX_STRING_LENGTH];
  std::vector<SVoteData>::iterator answer_it;
  csendf(ch, "--Current Vote Results--\n\r");
  int percent;
  strncpy(buf, vote_question.c_str(), MAX_STRING_LENGTH);
  csendf(ch, buf);
  csendf(ch, "\n\r");
  for (answer_it = answers.begin(); answer_it != answers.end(); answer_it++)
  {
    if (GET_LEVEL(ch) < IMMORTAL)
    {
      percent = (answer_it->votes * 100) / total_votes;
      csendf(ch, "%3d\%: %s\n\r", percent, answer_it->answer.c_str());
    }
    else
      csendf(ch, "%3d: %s\n\r", answer_it->votes, answer_it->answer.c_str());
  }
  csendf(ch, "\n\r");
}

void CVoteData::Reset(Character *ch)
{
  if (active)
  {
    if (ch) // this can be called with null
      send_to_char("Can't reset a vote while one is active.\r\n", ch);
    return;
  }
  if (ch)
    send_to_char("Ok. Vote cleared.\r\n", ch);

  total_votes = 0;
  vote_question.clear();
  answers.clear();
  ip_voted.clear();
  char_voted.clear();
}

void CVoteData::OutToFile()
{

  FILE *the_file;

  the_file = fopen("vote_data", "w");

  if (!the_file)
  {
    logentry("Unable to open/create save file for vote data", ANGEL,
             LogChannels::LOG_BUG);
    return;
  }

  fprintf(the_file, "%d\n", active);
  fprintf(the_file, "%d\n", total_votes);

  fprintf(the_file, "%s\n", vote_question.c_str());

  fprintf(the_file, "%d\n", answers.size());

  std::vector<SVoteData>::iterator answer_it;

  for (answer_it = answers.begin(); answer_it != answers.end(); answer_it++)
  {
    fprintf(the_file, "%d\n", answer_it->votes);
    fprintf(the_file, "%s\n", answer_it->answer.c_str());
  }

  std::map<std::string, bool>::iterator ip_it;

  fprintf(the_file, "%d\n", ip_voted.size());
  for (ip_it = ip_voted.begin(); ip_it != ip_voted.end(); ip_it++)
  {
    fprintf(the_file, "%s\n", ip_it->first.c_str());
  }

  fprintf(the_file, "%d\n", char_voted.size());
  for (ip_it = char_voted.begin(); ip_it != char_voted.end(); ip_it++)
  {
    fprintf(the_file, "%s\n", ip_it->first.c_str());
  }

  fclose(the_file);
  return;
}

void CVoteData::SetQuestion(Character *ch, std::string question)
{
  if (active)
  {
    send_to_char("Can't change the question while the vote is active.\r\n", ch);
    return;
  }
  send_to_char("Ok. Question changed.\r\n", ch);
  vote_question = question;
}

CVoteData::CVoteData()
{
  char buf[MAX_STRING_LENGTH];
  FILE *the_file = nullptr;
  ;
  int num = 0;
  int is_active = 0;
  int i = 0;
  SVoteData tmp_vote_data;
  active = false;

  the_file = fopen("../lib/vote_data", "r");
  if (!the_file)
  {
    this->Reset(nullptr);
    return;
  }

  // save is_active for later
  fscanf(the_file, "%d\n", &is_active);

  if (feof(the_file))
  {
    fclose(the_file);
    this->Reset(nullptr);
    return;
  }

  fscanf(the_file, "%d\n", &num);
  total_votes = num;

  if (!fgets(buf, MAX_STRING_LENGTH, the_file))
  {
    fclose(the_file);
    this->Reset(nullptr);
    logentry("Error reading question from vote file.", 0, LogChannels::LOG_MISC);
    return;
  }
  buf[strlen(buf) - 1] = 0;
  vote_question = buf;

  // ANSWERS
  fscanf(the_file, "%d\n", &i);
  for (; i > 0; i--)
  {
    fscanf(the_file, "%d\n", &num);
    if (!fgets(buf, MAX_STRING_LENGTH, the_file))
    {
      fclose(the_file);
      logentry("Error reading answers from vote file.", 0, LogChannels::LOG_MISC);
      this->Reset(nullptr);
      return;
    }

    buf[strlen(buf) - 1] = 0;
    tmp_vote_data.votes = num;
    tmp_vote_data.answer = buf;
    answers.push_back(tmp_vote_data);
  }

  // IP ADDRESSES
  fscanf(the_file, "%d\n", &i);
  for (; i > 0; i--)
  {
    if (!fgets(buf, MAX_STRING_LENGTH, the_file))
    {
      fclose(the_file);
      logentry("Error reading ip addresses from vote file.", 0, LogChannels::LOG_MISC);
      this->Reset(nullptr);
      return;
    }
    buf[strlen(buf) - 1] = 0;
    ip_voted[buf] = true;
  }

  // CHAR NAMES
  fscanf(the_file, "%d\n", &i);
  for (; i > 0; i--)
  {
    if (!fgets(buf, MAX_STRING_LENGTH, the_file))
    {
      fclose(the_file);
      logentry("Error reading char names from vote file.", 0, LogChannels::LOG_MISC);
      this->Reset(nullptr);
      return;
    }
    buf[strlen(buf) - 1] = 0;
    char_voted[buf] = true;
  }

  // everything must have been correct, activate it here
  active = (bool)is_active;

  fclose(the_file);
}

CVoteData::~CVoteData()
{
}

int do_vote(Character *ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int vote;
  arg = one_argument(arg, buf);

  if (!strcmp(buf, "results"))
  {
    DCVote->DisplayResults(ch);
    return eSUCCESS;
  }

  if (!DCVote->IsActive())
  {
    send_to_char("Sorry, there is nothing to vote on right now.\r\n", ch);
    return eSUCCESS;
  }
  if (!strlen(buf))
  {
    DCVote->DisplayVote(ch);
    return eSUCCESS;
  }

  if (GET_LEVEL(ch) < 40)
  {
    send_to_char("Sorry, you must be at least level 40 to vote.\r\n", ch);
    return eSUCCESS;
  }

  vote = atoi(buf);
  if (true == DCVote->Vote(ch, vote))
    logf(IMMORTAL, LogChannels::LOG_PLAYER, "%s just voted %d\n\r", GET_NAME(ch), vote);

  return eSUCCESS;
}

int do_random(Character *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int i = 0;

  if (IS_SET(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
  {
    send_to_char("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return eFAILURE;
  }

  i = number(1, 100);
  csendf(ch, "You roll a random number between 1 and 100 resulting in: $B%u$R.\r\n", i);
  sprintf(buf, "$n rolls a number between 1 and 100 resulting in: $B%u$R.\r\n", i);
  act(buf, ch, 0, 0, TO_ROOM, 0);
  return eSUCCESS;
}
