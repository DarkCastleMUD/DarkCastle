// This file takes care of all innate race abilities

#include "DC/innate.h"
#include "DC/race.h"
#include "DC/db.h"
#include "DC/fight.h"
#include "DC/room.h"
#include "DC/obj.h"
#include "DC/connect.h"
#include "DC/utility.h"
#include "DC/character.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/player.h"
#include "DC/levels.h"
#include "DC/interp.h"
#include "DC/magic.h"
#include "DC/act.h"
#include "DC/mobile.h"
#include "DC/spells.h"
#include <cstring> // strstr()
#include "DC/returnvals.h"
#include "DC/interp.h"

////////////////////////////////////////////////////////////////////////////
// external vars

////////////////////////////////////////////////////////////////////////////
// external functs

////////////////////////////////////////////////////////////////////////////
// local function declarations

int innate_powerwield(Character *ch, char *argument, int cmd);
int innate_regeneration(Character *ch, char *argument, int cmd);
int innate_illusion(Character *ch, char *argument, int cmd);
int innate_repair(Character *ch, char *argument, int cmd);
int innate_focus(Character *ch, char *argument, int cmd);
int innate_evasion(Character *ch, char *argument, int cmd);
int innate_shadowslip(Character *ch, char *argument, int cmd);
int innate_bloodlust(Character *ch, char *argument, int cmd);
int innate_fly(Character *ch, char *argument, int cmd);

////////////////////////////////////////////////////////////////////////////
// local definitions
struct in_skills
{
  char *name;
  int race;
  DO_FUN *func;
};

const struct in_skills innates[] = {
    {"powerwield", RACE_GIANT, innate_powerwield},
    {"regeneration", RACE_TROLL, innate_regeneration},
    {"illusion", RACE_GNOME, innate_illusion},
    {"bloodlust", RACE_ORC, innate_bloodlust},
    {"repair", RACE_DWARVEN, innate_repair},
    {"focus", RACE_ELVEN, innate_focus},
    {"evasion", RACE_PIXIE, innate_evasion},
    {"shadowslip", RACE_HOBBIT, innate_shadowslip},
    {"fly", RACE_PIXIE, innate_fly},
    {"\n", 0, nullptr}};

char *innate_skills[] =
    {
        "powerwield",
        "focus",
        "regeneration",
        "bloodlust",
        "illusion",
        "evasion",
        "shadowslip",
        "!repair!",
        "innate skill timer",
        "fly",
        "\n"};

////////////////////////////////////////////////////////////////////////////
// command functions
int do_innate(Character *ch, char *arg, int cmd)
{
  auto &arena = DC::getInstance()->arena_;
  if (ch && ch->in_room > 0 &&
      ch->room().isArena() && arena.isPotato())
  {
    ch->sendln("Cannot use innate skills within a potato arena.");
    return eFAILURE;
  }

  bool found = false;
  int i;
  char buf[512];
  arg = one_argument(arg, buf);
  for (i = 0; *innates[i].name != '\n'; i++)
  {
    if (innates[i].race == GET_RACE(ch))
    {
      if (buf[0] == '\0')
      {
        csendf(ch, "Your race has access to the $B%s$R innate ability.\r\n", innates[i].name);
        found = true;
      }
      else if (!str_cmp(innates[i].name, buf))
      {
        if (str_cmp(buf, "fly") && ch->affected_by_spell(SKILL_INNATE_TIMER))
        {
          ch->sendln("You cannot use that yet.");
          return eFAILURE;
        }
        if (GET_POS(ch) == position_t::SLEEPING &&
            i != 1)
        {
          ch->sendln("In your dreams, or what?");
          return eFAILURE;
        }
        int retval = (*(innates[i].func))(ch, arg, cmd);
        if (retval & eSUCCESS)
        {
          struct affected_type af;
          af.type = SKILL_INNATE_TIMER;

          if (!str_cmp(buf, "fly"))
            return retval;
          else if (!str_cmp(buf, "repair") || !str_cmp(buf, "bloodlust"))
          {
            af.duration = 12;
          }
          else
          {
            af.duration = 18;
          }
          // repair is every 12 ticks
          af.modifier = 0;
          af.location = 0;
          af.bitvector = -1;
          affect_to_char(ch, &af);
        }
        return retval;
      }
    }
  }
  if (!found)
  {
    if (buf[0] == 0)
    {
      ch->sendln("Your race has no innate abilities.");
    }
    else
    {
      ch->sendln("You do not have access to any such ability.");
    }
    return eFAILURE;
  }
  else
  {
    return eSUCCESS;
  }
}

int innate_regeneration(Character *ch, char *arg, int cmd)
{
  struct affected_type af;
  af.type = SKILL_INNATE_REGENERATION;
  af.duration = 6;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_REGENERATION;
  affect_to_char(ch, &af);
  ch->sendln("Your innate regenerative abilities allow you to heal quickly.");
  return eSUCCESS;
}

int innate_powerwield(Character *ch, char *arg, int cmd)
{
  struct affected_type af;
  af.type = SKILL_INNATE_POWERWIELD;
  af.duration = 3;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_POWERWIELD;
  affect_to_char(ch, &af);
  ch->sendln("You gather your energy in an effort to wield two mighty weapons.");
  act("$n gathers his strength in order to wield two mighty weapons.", ch, nullptr, nullptr, TO_ROOM, 0);
  return eSUCCESS;
}

int innate_focus(Character *ch, char *arg, int cmd)
{
  if (IS_AFFECTED(ch, AFF_FOCUS))
  {
    ch->sendln("But you are already focusing!  Why waste it?");
    return eFAILURE;
  }

  ch->sendln("You enter a trance and find yourself able to concentrate much better.");

  struct affected_type af;
  af.type = SKILL_INNATE_FOCUS;
  af.duration = 4;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_FOCUS;
  affect_to_char(ch, &af);

  return eSUCCESS;
}

int innate_illusion(Character *ch, char *arg, int cmd)
{
  if (IS_AFFECTED(ch, AFF_INVISIBLE))
  {
    ch->sendln("But you're already invisible!");
    return eFAILURE;
  }
  struct affected_type af;
  af.type = SKILL_INNATE_ILLUSION;
  af.duration = 4;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_ILLUSION;
  affect_to_char(ch, &af);
  af.type = SPELL_INVISIBLE;
  af.bitvector = AFF_INVISIBLE;
  affect_to_char(ch, &af);
  ch->sendln("You use your race's innate illusion powers, and fade out of existence.");
  act("$n chants something incoherent and fades out of existence.", ch, nullptr, nullptr, TO_ROOM, 0);
  return eSUCCESS;
}

int innate_bloodlust(Character *ch, char *arg, int cmd)
{
  if (!ch->fighting)
  {
    ch->sendln("You need to be fighting to use that.");
    return eFAILURE;
  }
  SET_BIT(ch->combat, COMBAT_ORC_BLOODLUST1);
  ch->sendln("Your blood boils as you drive yourself into a war-like state.");
  act("$n's blood boils has $e drives $mself into warlike rage.", ch, nullptr, nullptr, TO_ROOM, 0);
  return eSUCCESS;
}

int innate_repair(Character *ch, char *arg, int cmd)
{
  class Object *obj;
  char buf[MAX_STRING_LENGTH];
  int i, chance = 60 - ch->getLevel();
  bool found = false;
  arg = one_argument(arg, buf);
  if ((obj = get_obj_in_list_vis(ch, buf, ch->carrying)) == nullptr)
  {
    ch->sendln("You are not carrying anything like that.");
    return eFAILURE;
  }
  if (ch->getLevel() < obj->obj_flags.eq_level)
  {
    ch->sendln("This item is beyond your skill.");
    return eFAILURE;
  }
  if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
  {
    ch->sendln("This item is unrepairable.");
    return eFAILURE;
  }
  for (i = 0; i < obj->num_affects; i++)
  {
    if (obj->affected[i].location == APPLY_DAMAGED)
    {
      if (number(1, 101) < chance)
      {
        ch->sendln("You failed to repair it!");
        act("$n fails to repair $p.", ch, obj, obj, TO_ROOM, 0);
        return eSUCCESS;
      }
      found = true;
      obj->num_affects--;
    }
    else if (found)
    {
      obj->affected[i - 1] = obj->affected[i];
    }
  }
  if (found)
  {
    act("Your knowledge of weapons and armour allow you to quickly repair $p.", ch, obj, obj, TO_CHAR, 0);
    act("$n quickly repairs their $p.", ch, obj, obj, TO_ROOM, 0);
    return eSUCCESS;
  }
  else
  {
    ch->sendln("That item is already in excellent condition!");
    return eFAILURE;
  }
}

int innate_evasion(Character *ch, char *arg, int cmd)
{
  struct affected_type af;
  af.type = SKILL_INNATE_EVASION;
  af.duration = 4;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  ch->sendln("You bring up an aura, blocking all forms of scrying your location.");
  return eSUCCESS;
}

int innate_shadowslip(Character *ch, char *arg, int cmd)
{
  struct affected_type af;
  af.type = SKILL_INNATE_SHADOWSLIP;
  af.duration = 4;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_SHADOWSLIP;
  affect_to_char(ch, &af);
  ch->sendln("You blend with the shadows, preventing people from reaching you magically.");
  return eSUCCESS;
}

int innate_fly(Character *ch, char *arg, int cmd)
{
  if (ch->affected_by_spell(SKILL_INNATE_FLY))
  {
    affect_from_char(ch, SKILL_INNATE_FLY);
    ch->sendln("You fold your wings smoothly behind you and settle gently to the ground.");
    act("$n folds $s wings smoothly behind $m and settles gently to the ground.", ch, nullptr, nullptr, TO_ROOM, 0);
  }
  else
  {
    if (ISSET(ch->affected_by, AFF_FLYING))
    {
      ch->sendln("You are already flying.");
      return eFAILURE;
    }

    struct affected_type af;
    af.type = SKILL_INNATE_FLY;
    af.duration = -1;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char(ch, &af);
    ch->sendln("You spread your delicate wings and lift lightly into the air.");
    act("$n spreads $s delicate wings and lifts lightly into the air.", ch, nullptr, nullptr, TO_ROOM, 0);
  }

  return eSUCCESS;
}
