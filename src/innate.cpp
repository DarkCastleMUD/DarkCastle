// This file takes care of all innate race abilities

#include "DC/DC.h"

////////////////////////////////////////////////////////////////////////////
// external vars

////////////////////////////////////////////////////////////////////////////
// external functs

////////////////////////////////////////////////////////////////////////////
// local function declarations

qint32 innate_powerwield(CharacterPtr ch, QString argument, cmd_t cmd);
qint32 innate_regeneration(CharacterPtr ch, QString argument, cmd_t cmd);
qint32 innate_illusion(CharacterPtr ch, QString argument, cmd_t cmd);
qint32 innate_repair(CharacterPtr ch, QString argument, cmd_t cmd);
qint32 innate_focus(CharacterPtr ch, QString argument, cmd_t cmd);
qint32 innate_evasion(CharacterPtr ch, QString argument, cmd_t cmd);
qint32 innate_shadowslip(CharacterPtr ch, QString argument, cmd_t cmd);
qint32 innate_bloodlust(CharacterPtr ch, QString argument, cmd_t cmd);
qint32 innate_fly(CharacterPtr ch, QString argument, cmd_t cmd);

////////////////////////////////////////////////////////////////////////////
// local definitions
class in_skills
{
public:
  const QString name;
  qint32 race;
  DO_FUN *func;
};

const in_skills innates[] = {
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

const QStringList innate_skills =
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
command_return_t do_innate(CharacterPtr ch, QString arg, cmd_t cmd)
{
  auto &arena = dc_->arena_;
  if (ch && ch->in_room > 0 &&
      ch->room().isArena() && arena.isPotato())
  {
    ch->sendln("Cannot use innate skills within a potato arena.");
    return ReturnValue::eFAILURE;
  }

  bool found = false;
  qint32 i;
  QString buf;
  arg = one_argument(arg, buf);
  for (i = {}; *innates[i].name != '\n'; i++)
  {
    if (innates[i].race == ch->race)
    {
      if (buf[0] == '\0')
      {
        ch->send(u"Your race has access to the $B%s$R innate ability.\r\n"_s.arg(innates[i].name));
        found = true;
      }
      else if (!str_cmp(innates[i].name, buf))
      {
        if (str_cmp(buf, "fly") && ch->affected_by_spell(SKILL_INNATE_TIMER))
        {
          ch->sendln("You cannot use that yet.");
          return ReturnValue::eFAILURE;
        }
        if (GET_POS(ch) == position_t::SLEEPING &&
            i != 1)
        {
          ch->sendln("In your dreams, or what?");
          return ReturnValue::eFAILURE;
        }
        qint32 retval = (*(innates[i].func))(ch, arg, cmd);
        if (retval & ReturnValue::eSUCCESS)
        {
          affected_type af;
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
          af.modifier = {};
          af.location = {};
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
    return ReturnValue::eFAILURE;
  }
  else
  {
    return ReturnValue::eSUCCESS;
  }
}

qint32 innate_regeneration(CharacterPtr ch, QString arg, cmd_t cmd)
{
  affected_type af;
  af.type = SKILL_INNATE_REGENERATION;
  af.duration = 6;
  af.modifier = {};
  af.location = {};
  af.bitvector = AFF_REGENERATION;
  affect_to_char(ch, &af);
  ch->sendln("Your innate regenerative abilities allow you to heal quickly.");
  return ReturnValue::eSUCCESS;
}

qint32 innate_powerwield(CharacterPtr ch, QString arg, cmd_t cmd)
{
  affected_type af;
  af.type = SKILL_INNATE_POWERWIELD;
  af.duration = 3;
  af.modifier = {};
  af.location = {};
  af.bitvector = AFF_POWERWIELD;
  affect_to_char(ch, &af);
  ch->sendln("You gather your energy in an effort to wield two mighty weapons.");
  act_to_room("$n gathers his strength in order to wield two mighty weapons.", ch, nullptr, nullptr, 0);
  return ReturnValue::eSUCCESS;
}

qint32 innate_focus(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (IS_AFFECTED(ch, AFF_FOCUS))
  {
    ch->sendln("But you are already focusing!  Why waste it?");
    return ReturnValue::eFAILURE;
  }

  ch->sendln("You enter a trance and find yourself able to concentrate much better.");

  affected_type af;
  af.type = SKILL_INNATE_FOCUS;
  af.duration = 4;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = AFF_FOCUS;
  affect_to_char(ch, &af);

  return ReturnValue::eSUCCESS;
}

qint32 innate_illusion(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (IS_AFFECTED(ch, AFF_INVISIBLE))
  {
    ch->sendln("But you're already invisible!");
    return ReturnValue::eFAILURE;
  }
  affected_type af;
  af.type = SKILL_INNATE_ILLUSION;
  af.duration = 4;
  af.modifier = {};
  af.location = {};
  af.bitvector = AFF_ILLUSION;
  affect_to_char(ch, &af);
  af.type = SPELL_INVISIBLE;
  af.bitvector = AFF_INVISIBLE;
  affect_to_char(ch, &af);
  ch->sendln("You use your race's innate illusion powers, and fade out of existence.");
  act_to_room("$n chants something incoherent and fades out of existence.", ch, nullptr, nullptr, 0);
  return ReturnValue::eSUCCESS;
}

qint32 innate_bloodlust(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (!ch->fighting)
  {
    ch->sendln("You need to be fighting to use that.");
    return ReturnValue::eFAILURE;
  }
  SET_BIT(ch->combat, COMBAT_ORC_BLOODLUST1);
  ch->sendln("Your blood boils as you drive yourself into a war-like state.");
  act_to_room("$n's blood boils has $e drives $mself into warlike rage.", ch, nullptr, nullptr, 0);
  return ReturnValue::eSUCCESS;
}

qint32 innate_repair(CharacterPtr ch, QString arg, cmd_t cmd)
{
  ObjectPtr obj;
  QString buf;
  qint32 i, chance = 60 - ch->getLevel();
  bool found = false;
  arg = one_argument(arg, buf);
  if ((obj = get_obj_in_list_vis(ch, buf, ch->carrying)) == nullptr)
  {
    ch->sendln("You are not carrying anything like that.");
    return ReturnValue::eFAILURE;
  }
  if (ch->getLevel() < obj->obj_flags.eq_level)
  {
    ch->sendln("This item is beyond your skill.");
    return ReturnValue::eFAILURE;
  }
  if (IS_OBJ_STAT(obj, ITEM_NOREPAIR))
  {
    ch->sendln("This item is unrepairable.");
    return ReturnValue::eFAILURE;
  }
  for (i = {}; i < obj->num_affects; i++)
  {
    if (obj->affected[i].location == APPLY_DAMAGED)
    {
      if (ch->dc_->number(1, 101) < chance)
      {
        ch->sendln("You failed to repair it!");
        act_to_room("$n fails to repair $p.", ch, obj, obj, 0);
        return ReturnValue::eSUCCESS;
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
    act_to_character("Your knowledge of weapons and armour allow you to quickly repair $p.", ch, obj, obj, 0);
    act_to_room("$n quickly repairs their $p.", ch, obj, obj, 0);
    return ReturnValue::eSUCCESS;
  }
  else
  {
    ch->sendln("That item is already in excellent condition!");
    return ReturnValue::eFAILURE;
  }
}

qint32 innate_evasion(CharacterPtr ch, QString arg, cmd_t cmd)
{
  affected_type af;
  af.type = SKILL_INNATE_EVASION;
  af.duration = 4;
  af.modifier = {};
  af.location = {};
  af.bitvector = -1;
  affect_to_char(ch, &af);
  ch->sendln("You bring up an aura, blocking all forms of scrying your location.");
  return ReturnValue::eSUCCESS;
}

qint32 innate_shadowslip(CharacterPtr ch, QString arg, cmd_t cmd)
{
  affected_type af;
  af.type = SKILL_INNATE_SHADOWSLIP;
  af.duration = 4;
  af.modifier = {};
  af.location = {};
  af.bitvector = AFF_SHADOWSLIP;
  affect_to_char(ch, &af);
  ch->sendln("You blend with the shadows, preventing people from reaching you magically.");
  return ReturnValue::eSUCCESS;
}

qint32 innate_fly(CharacterPtr ch, QString arg, cmd_t cmd)
{
  if (ch->affected_by_spell(SKILL_INNATE_FLY))
  {
    affect_from_char(ch, SKILL_INNATE_FLY);
    ch->sendln("You fold your wings smoothly behind you and settle gently to the ground.");
    act_to_room("$n folds $s wings smoothly behind $m and settles gently to the ground.", ch, nullptr, nullptr, 0);
  }
  else
  {
    if (ISSET(ch->affected_by, AFF_FLYING))
    {
      ch->sendln("You are already flying.");
      return ReturnValue::eFAILURE;
    }

    affected_type af;
    af.type = SKILL_INNATE_FLY;
    af.duration = -1;
    af.modifier = {};
    af.location = {};
    af.bitvector = AFF_FLYING;
    affect_to_char(ch, &af);
    ch->sendln("You spread your delicate wings and lift lightly into the air.");
    act_to_room("$n spreads $s delicate wings and lifts lightly into the air.", ch, nullptr, nullptr, 0);
  }

  return ReturnValue::eSUCCESS;
}
