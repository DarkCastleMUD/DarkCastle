#include <algorithm>

#include "DC/DC.h"
#include "DC/affect.h"
#include "DC/character.h"
#include "DC/common.h"
#include "DC/const.h"
#include "DC/fight.h"
#include "DC/handler.h"
#include "DC/player.h"
#include "DC/returnvals.h"
#include "DC/room.h"
#include "DC/spells.h"
#include "DC/sustain.h"
#include "DC/utility.h"

static const auto SUSTAIN_CONFIG_KEY = u"sustain"_s;

int sustain_interval(int skill)
{
  return 30 - (std::clamp(skill, 1, 100) - 1) * 25 / 99;
}

static int sustain_effect(int spl)
{
  return spl == SPELL_EYES_OF_THE_OWL ? SPELL_INFRAVISION : spl;
}

QList<int> sustained_spells(CharacterPtr ch)
{
  QList<int> spell_list;
  if (!ch->player || !ch->player->config)
  {
    return spell_list;
  }

  for (const auto &entry : ch->player->config->value(SUSTAIN_CONFIG_KEY).split(',', Qt::SkipEmptyParts))
  {
    auto spl = entry.toInt();
    if (spl > 0 && spl <= MAX_SPL_LIST && spell_info[spl].sustainable())
    {
      spell_list.append(spl);
    }
  }
  return spell_list;
}

QList<int> sustained_effects(CharacterPtr ch)
{
  QList<int> effects;
  for (auto spl : sustained_spells(ch))
  {
    effects.append(sustain_effect(spl));
  }
  return effects;
}

static void set_sustained_spells(CharacterPtr ch, const QList<int> &spell_list)
{
  QStringList entries;
  for (auto spl : spell_list)
  {
    entries.append(QString::number(spl));
  }

  if (!ch->player->config)
  {
    ch->player->config = new PlayerConfig;
  }
  ch->player->config->insert(SUSTAIN_CONFIG_KEY, entries.join(','));
}

static int sustain_reserve(CharacterPtr ch, const QList<int> &spell_list)
{
  int reserve = 0;
  for (auto spl : spell_list)
  {
    reserve += SUSTAIN_RESERVE_MULTIPLIER * use_mana(ch, spl);
  }
  return reserve;
}

int sustain_reserve(CharacterPtr ch)
{
  return sustain_reserve(ch, sustained_spells(ch));
}

void sustain_delay(CharacterPtr ch)
{
  if (sustained_spells(ch).isEmpty())
  {
    return;
  }

  if (auto timer = ch->affected_by_spell(SKILL_SUSTAIN_TIMER))
  {
    timer->duration = SUSTAIN_DELAY_TICKS;
    return;
  }

  affected_type af;
  af.type = SKILL_SUSTAIN_TIMER;
  af.duration = SUSTAIN_DELAY_TICKS;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
}

static bool sustain_blocked(CharacterPtr ch)
{
  if (ch->in_room < 1 || ch->fighting || GET_POS(ch) <= position_t::SLEEPING || is_stunned(ch) || IS_AFFECTED(ch, AFF_PARALYSIS))
  {
    return true;
  }

  if (ch->isPlayerCantQuit() || ch->affected_by_spell(SKILL_SUSTAIN_TIMER) || ch->affected_by_spell(SPELL_NO_CAST_TIMER))
  {
    return true;
  }

  const auto &room = DC::getInstance()->world[ch->in_room];
  if (isSet(room.room_flags, NO_MAGIC))
  {
    return true;
  }

  for (auto obj = room.contents; obj; obj = obj->next_content)
  {
    if (DC::getInstance()->obj_index[obj->vnum_].vnum() == SILENCE_OBJ_NUMBER)
    {
      return true;
    }
  }
  return false;
}

static int sustain_cast(CharacterPtr ch, int spl)
{
  auto learned = ch->has_skill(spl);
  if (!learned)
  {
    return ReturnValue::eFAILURE;
  }

  char argument[] = "";
  uint8_t level = ch->getLevel();
  const auto &info = spell_info[spl];
  if (info.spell_pointer())
  {
    return (*info.spell_pointer())(level, ch, argument, SPELL_TYPE_SPELL, ch, nullptr, learned);
  }
  if (info.spell_pointer2())
  {
    return (*info.spell_pointer2())(level, ch, argument, SPELL_TYPE_SPELL, ch, nullptr, learned, use_mana(ch, spl));
  }
  return ReturnValue::eFAILURE;
}

static void sustain_check(CharacterPtr ch)
{
  auto spell_list = sustained_spells(ch);

  if (!ch->has_skill(SKILL_SUSTAIN))
  {
    set_sustained_spells(ch, {});
    return;
  }

  while (!spell_list.isEmpty() && ch->max_mana - sustain_reserve(ch, spell_list) <= 0)
  {
    auto costliest = *std::ranges::max_element(spell_list, {}, [ch](int spl)
                                               { return use_mana(ch, spl); });
    spell_list.removeOne(costliest);
    set_sustained_spells(ch, spell_list);
    ch->sendln(u"You can no longer sustain %1."_s.arg(spells[costliest - 1]));
  }

  if (ch->player->sustain_countdown > 0)
  {
    ch->player->sustain_countdown--;
  }
  if (ch->player->sustain_countdown > 0 || sustain_blocked(ch))
  {
    return;
  }
  ch->player->sustain_countdown = sustain_interval(ch->has_skill(SKILL_SUSTAIN));

  for (auto spl : sustained_spells(ch))
  {
    if (ch->affected_by_spell(sustain_effect(spl)))
    {
      continue;
    }

    ch->sendln(u"Your sustained %1 renews itself."_s.arg(spells[spl - 1]));
    if (SOMEONE_DIED(sustain_cast(ch, spl)))
    {
      return;
    }

    if (ch->affected_by_spell(sustain_effect(spl)))
    {
      ch->skill_increase_check(SKILL_SUSTAIN, ch->has_skill(SKILL_SUSTAIN), SKILL_INCREASE_MEDIUM);
    }
    else
    {
      spell_list.removeOne(spl);
      set_sustained_spells(ch, spell_list);
      ch->sendln(u"Your %1 fails to take hold and you stop sustaining it."_s.arg(spells[spl - 1]));
    }
  }
}

void sustain_update(void)
{
  QList<CharacterPtr> sustainers;
  for (const auto &ch : DC::getInstance()->character_list)
  {
    if (!sustained_spells(ch).isEmpty())
    {
      sustainers.append(ch);
    }
  }

  for (auto ch : sustainers)
  {
    sustain_check(ch);
  }
}

command_return_t Character::do_sustain(QStringList arguments, cmd_t cmd)
{
  if (!player || !has_skill(SKILL_SUSTAIN))
  {
    sendln("You do not know how to sustain spells.");
    return ReturnValue::eFAILURE;
  }

  auto spell_list = sustained_spells(this);
  auto spell_name = arguments.join(' ').trimmed();
  if (spell_name.startsWith('\''))
  {
    spell_name.removeFirst();
  }
  if (spell_name.endsWith('\''))
  {
    spell_name.chop(1);
  }

  if (spell_name.isEmpty())
  {
    if (spell_list.isEmpty())
    {
      sendln("You are not sustaining any spells.\n$3Syntax:$R sustain '<spell>'");
      return ReturnValue::eSUCCESS;
    }

    sendln("You are sustaining:");
    for (auto spl : spell_list)
    {
      sendln(u"  %1 %2 mana reserved%3"_s.arg(spells[spl - 1], -25).arg(SUSTAIN_RESERVE_MULTIPLIER * use_mana(this, spl), 4).arg(affected_by_spell(sustain_effect(spl)) ? "" : " (inactive)"));
    }
    sendln(u"Total mana reserved: %1"_s.arg(sustain_reserve(this)));
    return ReturnValue::eSUCCESS;
  }

  int spl = 0;
  for (int i = 0; *spells[i] != '\n'; i++)
  {
    if (QString(spells[i]).startsWith(spell_name, Qt::CaseInsensitive))
    {
      spl = i + 1;
      break;
    }
  }
  if (!spl)
  {
    sendln("You know of no such spell.");
    return ReturnValue::eFAILURE;
  }

  if (spell_list.removeOne(spl))
  {
    set_sustained_spells(this, spell_list);
    affect_from_char(this, sustain_effect(spl));
    sendln(u"You stop sustaining %1."_s.arg(spells[spl - 1]));
    save(cmd_t::SAVE_SILENTLY);
    return ReturnValue::eSUCCESS;
  }

  if (!has_skill(spl))
  {
    sendln("You do not know that spell.");
    return ReturnValue::eFAILURE;
  }

  if (!spell_info[spl].sustainable())
  {
    sendln("That spell cannot be sustained.");
    return ReturnValue::eFAILURE;
  }

  auto reserve = SUSTAIN_RESERVE_MULTIPLIER * use_mana(this, spl);
  if (max_mana - sustain_reserve(this) - reserve <= 0)
  {
    sendln(u"You do not have enough mana to sustain %1."_s.arg(spells[spl - 1]));
    return ReturnValue::eFAILURE;
  }

  spell_list.append(spl);
  set_sustained_spells(this, spell_list);
  GET_MANA(this) = MIN(GET_MANA(this), mana_limit(this));
  player->sustain_countdown = 0;
  sendln(u"You begin sustaining %1, reserving %2 mana."_s.arg(spells[spl - 1]).arg(reserve));

  if (!affected_by_spell(sustain_effect(spl)) && !sustain_blocked(this))
  {
    if (SOMEONE_DIED(sustain_cast(this, spl)))
    {
      return ReturnValue::eSUCCESS;
    }

    if (!affected_by_spell(sustain_effect(spl)))
    {
      spell_list.removeOne(spl);
      set_sustained_spells(this, spell_list);
      sendln(u"Your %1 fails to take hold and you stop sustaining it."_s.arg(spells[spl - 1]));
      return ReturnValue::eFAILURE;
    }
  }

  save(cmd_t::SAVE_SILENTLY);
  return ReturnValue::eSUCCESS;
}
