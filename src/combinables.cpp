// This file takes care of all the skills that make stuff by combining it
// in it's container type.  For example, poison making.

#include <iostream>
#include <fstream>
#include <utility>

#include "DC/obj.h"
#include "DC/db.h"

#include "DC/DC.h"

#include "DC/handler.h"
#include "DC/db.h"
#include "DC/player.h"
#include "DC/interp.h"
#include "DC/magic.h"
#include "DC/act.h"

#include "DC/spells.h"
#include "DC/combinables.h"
#include "DC/const.h"
#include "DC/utility.h"

using namespace Combinables;

////////////////////////////////////////////////////////////////////////////
// local definitions

constexpr auto TRADE_SKILL_POISON_CONT = 698; // mortar & pestle

const QStringList tradeskills =
    {
        "poisonmaking",
        "\n"};

constexpr auto MAX_INGREDIANTS = 10;

class trade_data_type
{
public:
  qint32 pieces[MAX_INGREDIANTS];
  qint32 result; // last item in array must be -1 for this
  qint32 trivial;
};
void determine_trade_skill_increase(CharacterPtr ch, qint32 skillnum, qint32 learned, qint32 trivial);
qint32 determine_trade_skill_chance(qint32 learned, qint32 trivial);
qint32 valid_trade_skill_combine(ObjectPtr container, class trade_data_type *data, CharacterPtr ch);

// POISON DEFINES
trade_data_type poison_vial_data[] =
    {
        {
            {600, 697, -1, -1, -1, -1, -1, -1, -1, -1}, // bee stinger
            696,
            15 // a vial of bee stinger poison
        },

        {
            {3175, 697, -1, -1, -1, -1, -1, -1, -1, -1}, // foraged apple
            695,
            10 // a small vial of low quality cyanide
        },

        {
            {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // not makeable
            694,
            10 // a vial of crude strychnine
        },

        {
            {697, 6447, -1, -1, -1, -1, -1, -1, -1, -1}, // vial, albino bat blood
            693,
            25 // a vial of vampire kiss
        },

        {
            {697, 1569, -1, -1, -1, -1, -1, -1, -1, -1}, // vial, flaxen vine
            692,
            30 // a vial of flaxen veleno
        },

        // This must come last
        {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
         -1,
         -1}};

class thief_poison_data
{
public:
  const QString poison_type;
};

thief_poison_data poison_vial_combat_data[] =
    {
        {"bee stinger poison"},

        {"low quality cyanide"},

        {"crude strychnine"},

        {"vampire kiss poison"},

        {"flaxen valeno"},

        {"invalid poison type"}};

////////////////////////////////////////////////////////////////////////////
// command functions

command_return_t do_poisonmaking(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 learned = ch->has_skill(SKILL_TRADE_POISON);

  if (ch->getLevel() > IMMORTAL)
    learned = 500;

  if (!learned)
  {
    ch->sendln("You do not know how to make poisons.");
    return ReturnValue::eFAILURE;
  }

  // TODO search for mortar and pestle
  ObjectPtr container = get_obj_in_list_vis(ch, TRADE_SKILL_POISON_CONT, ch->carrying);

  if (!container)
  {
    ch->sendln("You have nothing to make poisons in.");
    return ReturnValue::eFAILURE;
  }

  // search if the items in mortar and pestle match any poison vial types
  qint32 index = valid_trade_skill_combine(container, poison_vial_data, ch);

  if (index == -2)
    return ReturnValue::eFAILURE;

  ch->sendln("You mix the ingrediants together in an attempt to make a poison..");

  // Clear the items out of the container
  while (container->contains)
    extract_obj(container->contains);

  qint32 failure = {};
  qint32 chance = {};
  qint32 percent;

  // determine success/failure

  if (index == -1)
    failure = 1;
  else
  {
    percent = dc_->number(1, 101);
    chance = determine_trade_skill_chance(learned, poison_vial_data[index].trivial);

    determine_trade_skill_increase(ch, SKILL_TRADE_POISON, learned, poison_vial_data[index].trivial);

    if (percent > chance)
      failure = 1;
  }

  if (failure)
  {
    ch->sendln("Ahh crap, something got screwed up.  You are forced to throw away this attempt.");
    act_to_room("$n fiddles with a mortar and pestle and looks unhappy.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  // give poison to player
  qint32 rewardnum = real_object(poison_vial_data[index].result);
  if (rewardnum < 0)
  {
    ch->sendln("That poison is broken.  Tell a god.");
    return (ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR);
  }

  ObjectPtr reward = clone_object(rewardnum);
  obj_to_char(reward, ch);
  ch->send(u"You succesfully make a %1!\r\n"_s.arg(reward->short_description()));
  act_to_room("$n successfully makes a $p.", ch, reward, 0, 0);

  return ReturnValue::eSUCCESS;
}

command_return_t do_poisonweapon(CharacterPtr ch, QString argument, cmd_t cmd)
{
  if (GET_CLASS(ch) != CLASS_THIEF && ch->getLevel() <= IMMORTAL)
  {
    ch->sendln("Only thieves are trained enough to poison their weapons.");
    return ReturnValue::eFAILURE;
  }

  QString vialarg;

  one_argument(argument, vialarg);

  if (vialarg.isEmpty())
  {
    ch->sendln("Poison your weapon with what?");
    return ReturnValue::eFAILURE;
  }

  if (ch->fighting)
  {
    ch->sendln("You can't do that while fighting!");
    return ReturnValue::eFAILURE;
  }

  // find weapon
  ObjectPtr weapon = ch->equipment[WEAR_WIELD];
  if (!weapon)
  {
    ch->sendln("You aren't wielding a weapon to poison.");
    return ReturnValue::eFAILURE;
  }

  // find vial and verify it's a valid poison vial
  ObjectPtr vial = get_obj_in_list_vis(ch, vialarg, ch->carrying);
  if (!vial)
  {
    ch->send(u"You don't seem to have any %1.\r\n"_s.arg(vialarg));
    return ReturnValue::eFAILURE;
  }

  qint32 found = -1;
  for (qint32 i = {}; poison_vial_data[i].result != -1; i++)
    if (poison_vial_data[i].result == dc_->obj_index[vial->item_number].vnum())
    {
      found = i;
      break;
    }

  if (found < 0)
  {
    ch->send(u"The %1 is not a valid weapon poison.\r\n"_s.arg(vial->short_description()));
    return ReturnValue::eFAILURE;
  }

  for (qint32 j = {}; j < weapon->num_affects; j++)
    if (weapon->affected[j].location == WEP_THIEF_POISON)
    {
      ch->sendln("Your weapon is already poisoned.");
      return ReturnValue::eFAILURE;
    }

  // poison weapon
  add_obj_affect(weapon, WEP_THIEF_POISON, found);

  // remove vial
  extract_obj(vial);

  return ReturnValue::eSUCCESS;
}

////////////////////////////////////////////////////////////////////////////
// Utility functions

// Search through the different types of data looking for a match
// Return index of match on successful find
// Return -1 on failure
// Return -2 if there's nothing in the container
qint32 valid_trade_skill_combine(ObjectPtr container, trade_data_type *data, CharacterPtr ch)
{
  if (!(container->contains))
  {
    ch->send(u"Your %1 appears to be empty.\r\n"_s.arg(container->short_description()));
    return -2;
  }

  QList<qint32> current;

  // take all the items in our container and put them in an array by vnum
  for (ObjectPtr j = container->contains; j; j = j->next_content)
    if (j->item_number >= 0)
      current.push_back(dc_->obj_index[j->item_number].vnum());
    else
      return -1; // only valid object ingrediants will match

  sort(current.begin(), current.end());

  QList<qint32> valid;

  // loop through the valid combinations, and try to find a match
  for (qint32 i = {}; data[i].result != -1; i++)
  {
    valid.clear();
    for (qint32 k = {}; k < MAX_INGREDIANTS && data[i].pieces[k] != -1; k++)
      valid.push_back(data[i].pieces[k]);

    sort(valid.begin(), valid.end());

    if (!valid.size()) // empty recipe (unmakeable)
      return -1;

    if (valid == current) // if vectors match, then we have a valid combination
      return i;
  }

  return -1; // didn't match any
}

qint32 determine_trade_skill_chance(qint32 learned, qint32 trivial)
{
  qint32 chance = 50;

  chance -= trivial;
  chance += learned;

  chance = MIN(chance, 90);
  chance = MAX(chance, 10);

  return chance;
}

void determine_trade_skill_increase(CharacterPtr ch, qint32 skillnum, qint32 learned, qint32 trivial)
{
  // can't learn past item's trivial value
  if (learned >= trivial)
  {
    ch->sendln("You have learned all you can from making this item.");
    return;
  }

  // TODO - add other requirements when done debugging

  // learn a point
  ch->learn_skill(skillnum, 1, 500);
}

qint32 handle_poisoned_weapon_attack(CharacterPtr ch, CharacterPtr vict, qint32 type)
{
  qint32 retval = ReturnValue::eSUCCESS;
  // unused   qint32 dam;

  if (!ch->equipment[WEAR_WIELD])
  {
    ch->sendln("In handle_poisoned_weapon_atack() with null wield.  Tell a god.");
    return (ReturnValue::eFAILURE | ReturnValue::eINTERNAL_ERROR);
  }

  /*   switch(type)
     {
        case 0: // bee stinger poison
           if(saves_spell(ch, vict, 1, SAVE_TYPE_POISON) < 0)
             dam = 25;
           else dam = 15;
           retval = damage(ch, vict, dam, TYPE_POISON, POISON_MESSAGE_BASE+type);
           break;

        case 1: // low quality cyanide
           if(saves_spell(ch, vict, 10, SAVE_TYPE_POISON) < 0)
             dam = 35;
           else dam = 25;
           retval = damage(ch, vict, dam, TYPE_POISON, POISON_MESSAGE_BASE+type);
           break;

        case 2: // crude strychnine
           if(saves_spell(ch, vict, 10, SAVE_TYPE_POISON) < 0)
           {
             SET_BIT(vict->combat, COMBAT_MISS_AN_ATTACK);
             dam = 10;
           }
           else dam = {};
           retval = damage(ch, vict, dam, TYPE_POISON, POISON_MESSAGE_BASE+type);
           break;

        case 3: // vampire kiss
           retval = cast_vampiric_touch(6, ch, "", SPELL_TYPE_SPELL, vict, 0, 35);
           break;

        case 4: // flaxen veleno
           if(saves_spell(ch, vict, 1, SAVE_TYPE_POISON) < 0)
             dam = 40;
           else dam = 20;
           retval = damage(ch, vict, dam, TYPE_POISON, POISON_MESSAGE_BASE+type);
           break;

        default:
           ch->send(u"Unknown poison type %1.  Let a god know.\r\n"_s.arg(type));
           break;
     }

     // you can do this even if the mob died, because the weapon is still valid
     remove_obj_affect_by_type(ch->equipment[WEAR_WIELD], WEP_THIEF_POISON);
  */
  return retval;
}

command_return_t do_brew(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1, liquid, container, buffer;
  ObjectPtr herbobj, *liquidobj, containerobj;
  affected_type af;
  Brew b;

  qint32 learned = ch->has_skill(SKILL_BREW);

  if (ch->isMortalPlayer() && !learned)
  {
    ch->sendln("You just don't have the mind for potion brewing.");
    return ReturnValue::eFAILURE;
  }

  if (argument.isEmpty())
  {
    send_to_char("Brew what?\r\n"
                 "$3Syntax:$R brew <herb> <liquid> <container>\r\n",
                 ch);
    if (ch->getLevel() >= 106)
    {
      send_to_char("        brew load\r\n"
                   "        brew save\r\n"
                   "        brew list\r\n"
                   "        brew add [herb_vnum] [liquid_type] [container_vnum] [spell_num]\r\n"
                   "        brew remove [recipe_num]\r\n\r\n",
                   ch);
    }
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, arg1);

  if (ch->isPlayer() && ch->getLevel() >= 106)
  {
    if (!str_cmp(arg1, "load"))
    {
      b.load();
      dc_->logf(108, DC::LogChannel::LOG_WORLD, "Loaded %d brew recipes.", b.size());
      return ReturnValue::eSUCCESS;
    }
    else if (!str_cmp(arg1, "save"))
    {
      b.save();
      dc_->logf(108, DC::LogChannel::LOG_WORLD, "Saved %d brew recipes.", b.size());
      return ReturnValue::eSUCCESS;
    }
    else if (!str_cmp(arg1, "list"))
    {
      b.list(ch);
      return ReturnValue::eSUCCESS;
    }
    else if (!str_cmp(arg1, "add"))
    {
      return b.add(ch, argument);
    }
    else if (!str_cmp(arg1, "remove"))
    {
      return b.remove(ch, argument);
    }
  }

  if (ch->affected_by_spell(SKILL_BREW_TIMER))
  {
    ch->sendln("You aren't ready to brew anything again.");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, liquid);
  one_argument(argument, container);

  if (liquid.isEmpty())
  {
    send_to_char("You'll need to choose a liquid type and container.\r\n"
                 "$3Syntax:$R brew <herb> <liquid> <container>\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (container.isEmpty())
  {
    send_to_char("You'll need to select a container.\r\n"
                 "$3Syntax:$R brew <herb> <liquid> <container>\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  herbobj = get_obj_in_list_vis(ch, arg1, ch->carrying);
  liquidobj = get_obj_in_list_vis(ch, liquid, ch->carrying);
  containerobj = get_obj_in_list_vis(ch, container, ch->carrying);

  if (!herbobj)
  {
    ch->sendln("You do not have that type of herb.");
    return ReturnValue::eFAILURE;
  }
  if (herbobj->obj_flags.type_flag != ITEM_OTHER)
  {
    ch->sendln("That is not an herb.");
    return ReturnValue::eFAILURE;
  }

  if (!liquidobj)
  {
    ch->sendln("You do not have that type of liquid.");
    return ReturnValue::eFAILURE;
  }
  if (liquidobj->obj_flags.type_flag != ITEM_DRINKCON)
  {
    ch->sendln("That is not a liquid container.");
    return ReturnValue::eFAILURE;
  }

  if (!containerobj)
  {
    ch->sendln("You do not have that type of container.");
    return ReturnValue::eFAILURE;
  }

  if (containerobj->obj_flags.type_flag != ITEM_POTION && containerobj->obj_flags.type_flag != ITEM_DRINKCON)
  {
    ch->sendln("That is not a target container.");
    return ReturnValue::eFAILURE;
  }

  if (isSet(containerobj->obj_flags.more_flags, ITEM_CUSTOM))
  {
    ch->sendln("That container is already a brewed potion.");
    return ReturnValue::eFAILURE;
  }

  if (containerobj == liquidobj)
  {
    ch->sendln("Your liquid and target container cannot be the same object!");
    return ReturnValue::eFAILURE;
  }

  if (liquidobj->obj_flags.value[1] < 1)
  {
    ch->sendln("There is no liquid left in that container.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BREW))
  {
    return ReturnValue::eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2.5);

  const QString potion_color;
  // Determine color to use in message based on herb used
  switch (dc_->obj_index[herbobj->item_number].vnum())
  {
  case 6301:
    potion_color = "$B$2green$R and $B$4red$R";
    break;
  case 6302:
    potion_color = "$B$2green$R and $6purple$R";
    break;
  case 6303:
    potion_color = "$B$1blue$R";
    break;
  case 6304:
    potion_color = "$5brown$R";
    break;
  case 6305:
    potion_color = "$Bcrystalline$R";
    break;
  case 6306:
    potion_color = "$2dark green$R";
    break;
  case 6307:
    potion_color = "$B$5bright yellow$R";
    break;
  case 6308:
    potion_color = "$B$6pink$R";
    break;
  case 6309:
    potion_color = "$4vermillion$R";
    break;
  case 6310:
    potion_color = "fuschia";
    break;
  case 6311:
    potion_color = "$B$0jet black$R";
    break;
  case 6312:
    potion_color = "$6indigo$R";
    break;
  default:
    potion_color = "dark and murky";
    break;
  }

  // Search for the current combination as a recipe
  Brew::recipe r = {dc_->obj_index[herbobj->item_number].vnum(),
                    liquidobj->obj_flags.value[2],
                    dc_->obj_index[containerobj->item_number].vnum()};
  qint32 spell = b.find(r);

  //  ch->sendln(u"Searching for herb: %d(%s)\nliquid: %d(%s)\ncontainer: %d(%s).....%d\n"_s,
  //	 dc_->obj_index[herbobj->item_number].vnum(), GET_OBJ_SHORT(herbobj),
  //	 liquidobj->obj_flags.value[2], GET_OBJ_SHORT(liquidobj),
  //	 dc_->obj_index[containerobj->item_number].vnum(), GET_OBJ_SHORT(containerobj), spell);

  if (spell == 0)
  {
    act_to_character("As you finish, nothing special seems to happen.", ch, 0, 0, 0);
    act_to_room("As $e finishes, nothing special seems to happen.", ch, 0, 0, 0);

    af.type = SKILL_BREW_TIMER;
    af.location = APPLY_NONE;
    af.modifier = {};
    af.duration = 1;
    af.bitvector = -1;
    affect_to_char(ch, &af);
  }
  else if (skill_success(ch, 0, SKILL_BREW))
  {
    af.type = SKILL_BREW_TIMER;
    af.location = APPLY_NONE;
    af.modifier = {};
    af.duration = 1;
    af.bitvector = -1;
    affect_to_char(ch, &af);

    act_to_character("You sit down and carefully pour the ingredients into $o and give it a gentle shake to mix them.", ch, containerobj, 0, 0);
    dc_snprintf(buffer, MAX_STRING_LENGTH, "As the $o disolves, the liquid turns %s.", potion_color);
    act_to_character(buffer, ch, herbobj, 0, 0);

    act_to_room("$n sits down and carefully pours ingredients into $o and gives it a gentle shake to mix them.", ch, containerobj, 0, 0);
    dc_snprintf(buffer, MAX_STRING_LENGTH, "As $e finishes, the liquid turns %s.", potion_color);
    act_to_room(buffer, ch, containerobj, 0, 0);

    // Find container key (crude, plain, etc)
    auto container_key = containerobj->name().split(' ').value(2);

    // Find liquid key (salty, milk, strong)
    const QString liquid_key;
    switch (liquidobj->obj_flags.value[2])
    {
    case LIQ_MILK:
      liquid_key = "milky";
      break;
    case LIQ_WINE:
      liquid_key = "strong";
      break;
    case LIQ_SALTWATER:
      liquid_key = "salty";
      break;
    default:
      liquid_key = "unknown";
      break;
    }

    // Put it all together into the new name

    auto potionname = u"potion %1 %2"_s.arg(container_key).arg(liquid_key);
    auto potionshort = u"a %1 %2 %3 potion"_s.arg(container_key).arg(liquid_key).arg(potion_color);
    auto potionlong = u"a %1 %2 %3 potion lies here."_s.arg(container_key).arg(liquid_key).arg(potion_color);

    containerobj->obj_flags.type_flag = ITEM_POTION;
    containerobj->obj_flags.value[0] = (learned / 2 - 5) + GET_WIS(ch) / 2 + GET_INT(ch) / 2;
    containerobj->obj_flags.value[1] = spell;
    containerobj->obj_flags.value[2] = {};
    containerobj->obj_flags.value[3] = {};
    containerobj->name(potionname);
    containerobj->short_description(potionshort);
    containerobj->long_description(potionlong);
    // We set the item to custom so that it will save everytime uniquely
    SET_BIT(containerobj->obj_flags.more_flags, ITEM_CUSTOM);

    extract_obj(herbobj);
  }
  else
  {
    act_to_character("You sit down and carefully pour the ingredients into $o and give it a gentle shake to mix them.", ch, containerobj, 0, 0);
    act_to_character("As you finish, the liquid begins to bubble furiously, cracking the $o and rendering your work useless!", ch, containerobj, 0, 0);

    act_to_room("$n sits down and carefully pours ingredients into $o and gives it a gentle shake to mix them.", ch, containerobj, 0, 0);
    act_to_room("As $e finishes, the liquid begins to bubble furiously, cracking the $o and rendering $s work useless!", ch, containerobj, 0, 0);

    extract_obj(containerobj);
    extract_obj(herbobj);
    if (liquidobj->obj_flags.value[1] > 0)
    {
      liquidobj->obj_flags.value[1]--;
    }
  }

  return ReturnValue::eSUCCESS;
}

Brew::Brew(void)
{
  if (initialized == false)
  {
    load();
    initialized = true;
  }
}

Brew::~Brew()
{
}

void Brew::load(void)
{
  std::ifstream ifs(RECIPES_FILENAME, std::ios_base::in);
  if (!ifs.is_open())
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Unable to open %s.", RECIPES_FILENAME);
    return;
  }

  recipes.clear();

  try
  {
    while (!ifs.eof())
    {
      qint32 spell = {};
      recipe r = {0, 0, 0};

      ifs >> r.herb;
      ifs >> r.liquid;
      ifs >> r.container;
      ifs >> spell;

      // Don't insert empty entries
      if (r.container && r.liquid && r.herb && spell)
      {
        recipes.insert(std::make_pair(r, spell));
      }
    }
  }
  catch (loadError &)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Error loading %s.", RECIPES_FILENAME);
  }
}

void Brew::save(void)
{
  std::ofstream ofs(RECIPES_FILENAME, std::ios_base::trunc);
  if (!ofs.is_open())
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Unable to open %s.", RECIPES_FILENAME);
    return;
  }

  try
  {
    for (QMap<recipe, qint32>::iterator iter = recipes.begin(); iter != recipes.end(); ++iter)
    {
      std::pair<recipe, qint32> p = *iter;
      recipe r = p.first;
      qint32 spell = p.second;

      ofs << r.herb << " " << r.liquid << " " << r.container << " " << spell << std::endl;
    }
  }
  catch (...)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Error saving %s.", RECIPES_FILENAME);
  }
}

void Brew::list(CharacterPtr ch)
{
  QString buffer = {};
  qint32 i = {};

  if (!ch)
  {
    return;
  }

  ch->sendln("[# ] [herb #] [liquid] [container] Spell Name\r\n");
  for (QMap<recipe, qint32>::reverse_iterator iter = recipes.rbegin(); iter != recipes.rend(); ++iter)
  {
    recipe r = iter->first;
    qint32 spell = iter->second;

    sprinttype(spell - 1, spells, buffer);
    ch->send(u"[%2d] [%6llu] [%6llu] [%9llu] %s (%d)\r\n"_s.arg(++i, r.herb, r.liquid).arg(r.container).arg(buffer).arg(spell));
  }
}

qint32 Brew::add(CharacterPtr ch, QString argument)
{
  vnum_t herb_vnum = {}, container_vnum = {};
  qint32 liquid_type = {}, spell = {};
  QString arg1 = {}, arg2 = {}, arg3 = {}, arg4 = {};

  if (!ch)
  {
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  argument = one_argument(argument, arg4);

  if (arg.isEmpty() 1 || arg.isEmpty() 2 || arg.isEmpty() 3 || arg.isEmpty() 4)
  {
    ch->sendln("Syntax: brew add [herb_vnum] [liquid_type] [container_vnum] [spell_num]");
    return ReturnValue::eFAILURE;
  }

  herb_vnum = atoll(arg1);
  liquid_type = atoi(arg2);
  container_vnum = atoll(arg3);

  if (herb_vnum < 6301 || herb_vnum > 6312)
  {
    ch->sendln("Only vnums 6301-6312 are valid herbs.");
    return ReturnValue::eFAILURE;
  }

  switch (liquid_type)
  {
  case LIQ_MILK:
  case LIQ_WINE:
  case LIQ_SALTWATER:
    break;
  default:
    ch->send(u"Invalid liquid type. Only Milk (%d), Wine (%d), Salt Water (%d) allowed.\r\n"_s.arg(LIQ_MILK).arg(LIQ_WINE).arg(LIQ_SALTWATER));
    return ReturnValue::eFAILURE;
    break;
  }

  if (container_vnum < 6320 || container_vnum > 6324)
  {
    ch->sendln("Only vnums 6320-6324 are valid containers.");
    return ReturnValue::eFAILURE;
  }

  if ((spell = find_skill_num(arg4)) < 0)
  {
    ch->send(u"Cannot find spell '%s' in master spell list.\r\n"_s.arg(arg4));
    return ReturnValue::eFAILURE;
  }

  recipe r = {herb_vnum, liquid_type, container_vnum};
  recipes.insert(std::make_pair(r, spell));

  ch->sendln("New brew recipe added.");

  return ReturnValue::eSUCCESS;
}

qint32 Brew::remove(CharacterPtr ch, QString argument)
{
  if (!ch)
  {
    return ReturnValue::eFAILURE;
  }

  if (argument.isEmpty())
  {
    ch->sendln("Syntax: brew remove [recipe_num]");
    return ReturnValue::eFAILURE;
  }

  qint32 i = {};
  qint32 target = atoi(argument);

  for (QMap<recipe, qint32>::reverse_iterator iter = recipes.rbegin(); iter != recipes.rend(); ++iter)
  {
    if (++i == target)
    {
      recipes.erase((*iter).first);
      ch->send(u"Recipe # %1 has been removed.\r\n"_s.arg(target));

      return ReturnValue::eSUCCESS;
    }
  }

  ch->send(u"Recipe # %1 not found.\r\n"_s.arg(target));
  return ReturnValue::eFAILURE;
}

qint32 Brew::size(void)
{
  return recipes.size();
}

qint32 Brew::find(Brew::recipe r)
{
  qint32 spell = {};

  QMap<recipe, qint32>::iterator result = recipes.find(r);
  if (result != recipes.end())
  {
    spell = result->second;
  }

  return spell;
}

command_return_t do_scribe(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg1, dust, pen, paper;
  ObjectPtr inkobj, *dustobj, *penobj, paperobj;
  affected_type af;
  Scribe s;

  qint32 learned = ch->has_skill(SKILL_SCRIBE);

  if (ch->isMortalPlayer() && !learned)
  {
    ch->sendln("You just don't have the mind for scribing.");
    return ReturnValue::eFAILURE;
  }

  if (argument.isEmpty())
  {
    send_to_char("Scribe what?\r\n"
                 "$3Syntax:$R scribe <ink> <dust> <pen> <paper>\r\n",
                 ch);
    if (ch->getLevel() >= 106)
    {
      send_to_char("        scribe load\r\n"
                   "        scribe save\r\n"
                   "        scribe list\r\n"
                   "        scribe add <ink_vnum> <dust_vnum> <pen_vnum> <paper_vnum> <spell_num>\r\n"
                   "        scribe remove [recipe_num]\r\n\r\n",
                   ch);
    }
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, arg1);

  if (ch->isPlayer() && ch->getLevel() >= 106)
  {
    if (!str_cmp(arg1, "load"))
    {
      s.load();
      dc_->logf(108, DC::LogChannel::LOG_WORLD, "Loaded %d scribe recipes.", s.size());
      return ReturnValue::eSUCCESS;
    }
    else if (!str_cmp(arg1, "save"))
    {
      s.save();
      dc_->logf(108, DC::LogChannel::LOG_WORLD, "Saved %d scribe recipes.", s.size());
      return ReturnValue::eSUCCESS;
    }
    else if (!str_cmp(arg1, "list"))
    {
      s.list(ch);
      return ReturnValue::eSUCCESS;
    }
    else if (!str_cmp(arg1, "add"))
    {
      return s.add(ch, argument);
    }
    else if (!str_cmp(arg1, "remove"))
    {
      return s.remove(ch, argument);
    }
  }

  if (ch->affected_by_spell(SKILL_SCRIBE_TIMER))
  {
    ch->sendln("You aren't ready to scribe anything again.");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, dust);
  argument = one_argument(argument, pen);
  argument = one_argument(argument, paper);

  if (dust.isEmpty())
  {
    send_to_char("You'll need to choose a dust, pen and paper.\r\n"
                 "$3Syntax:$R scribe <ink> <dust> <pen> <paper>\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (pen.isEmpty())
  {
    send_to_char("You'll need to choose a pen and paper.\r\n"
                 "$3Syntax:$R scribe <ink> <dust> <pen> <paper>\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  if (paper.isEmpty())
  {
    send_to_char("You'll need to select a paper.\r\n"
                 "$3Syntax:$R scribe <ink> <dust> <pen> <paper>\r\n",
                 ch);
    return ReturnValue::eFAILURE;
  }

  inkobj = get_obj_in_list_vis(ch, arg1, ch->carrying);
  dustobj = get_obj_in_list_vis(ch, dust, ch->carrying);
  penobj = get_obj_in_list_vis(ch, pen, ch->carrying);
  paperobj = get_obj_in_list_vis(ch, paper, ch->carrying);

  if (!inkobj)
  {
    ch->sendln("You do not have that type of ink.");
    return ReturnValue::eFAILURE;
  }
  if (inkobj->obj_flags.type_flag != ITEM_DRINKCON || inkobj->obj_flags.value[2] != LIQ_INK)
  {
    ch->sendln("That is not ink.");
    return ReturnValue::eFAILURE;
  }

  if (!dustobj)
  {
    ch->sendln("You do not have that type of dust.");
    return ReturnValue::eFAILURE;
  }
  if (dustobj->obj_flags.type_flag != ITEM_OTHER)
  {
    ch->sendln("That is not dust.");
    return ReturnValue::eFAILURE;
  }

  if (!penobj)
  {
    ch->sendln("You do not have that type of pen.");
    return ReturnValue::eFAILURE;
  }
  if (penobj->obj_flags.type_flag != ITEM_PEN)
  {
    ch->sendln("That is not a pen.");
    return ReturnValue::eFAILURE;
  }

  if (!paperobj)
  {
    ch->sendln("You do not have that type of paper.");
    return ReturnValue::eFAILURE;
  }
  if (paperobj->obj_flags.type_flag != ITEM_SCROLL)
  {
    ch->sendln("That is not paper.");
    return ReturnValue::eFAILURE;
  }

  if (inkobj->obj_flags.value[1] < 1)
  {
    ch->sendln("There is no liquid left in that ink container.");
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_SCRIBE))
  {
    return ReturnValue::eFAILURE;
  }

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 2.5);

  // Search for the current combination as a recipe
  Scribe::recipe r = {dc_->obj_index[inkobj->item_number].vnum(),
                      dc_->obj_index[dustobj->item_number].vnum(),
                      dc_->obj_index[penobj->item_number].vnum(),
                      dc_->obj_index[paperobj->item_number].vnum()};
  qint32 spell = s.find(r);

  act_to_character("You sit down and carefully inscribe the words of the gods onto the parchment.", ch, 0, 0, 0);
  act_to_room("$n sits down and carefully inscribes the words of the gods onto some parchment.", ch, 0, 0, 0);

  if (inkobj->obj_flags.value[1] > 0)
  {
    inkobj->obj_flags.value[1]--;
  }

  if (spell == 0)
  {
    act_to_character("As you finish, nothing special seems to happen.", ch, 0, 0, 0);
    act_to_room("As $e finishes, nothing special seems to happen.", ch, 0, 0, 0);

    af.type = SKILL_SCRIBE_TIMER;
    af.location = APPLY_NONE;
    af.modifier = {};
    af.duration = 1;
    af.bitvector = -1;
    affect_to_char(ch, &af);
  }
  else if (!skill_success(ch, 0, SKILL_SCRIBE))
  {
    act_to_character("As you finish, the letters on the newly minted scroll burst into $B$4flame$R leaving nothing but ash!", ch, 0, 0, 0);
    act_to_room("As $e finishes, the letters on the newly minted scroll burst into $B$4flame$R leaving nothing but ash!", ch, 0, 0, 0);
    extract_obj(paperobj);
    extract_obj(penobj);
    extract_obj(dustobj);
    // 50% of a failure causing 'wild magic' to be cast on self
    if (ch->dc_->number(1, 100) > 50)
    {
      cast_wild_magic(ch->getLevel(), ch, "offense", 0, ch, 0, 0);
    }
  }
  else
  {
    act_to_character("As you finish, the letters on the newly minted scroll $Bglow$R briefly and return to normal.", ch, 0, 0, 0);
    act_to_room("As $e finishes, the letters on the newly minted scroll $Bglow$R briefly and return to normal.", ch, 0, 0, 0);

    af.type = SKILL_SCRIBE_TIMER;
    af.location = APPLY_NONE;
    af.modifier = {};
    af.duration = 1;
    af.bitvector = -1;
    affect_to_char(ch, &af);

    auto ink_key = inkobj->name().split(' ').value(2);
    auto dust_key = dustobj->name().split(' ').value(2);
    auto pen_key = penobj->name().split(' ').value(2);
    auto paper_key = paperobj->name().split(' ').value(2);

    // Put it all together into the new name
    auto scrollname = u"scroll %1 %2 %3 %4"_s.arg(paper_key).arg(dust_key).arg(pen_key).arg(ink_key);
    auto scrollshort = u"a %1 $B%2$R scroll %3 in %4 ink"_s.arg(paper_key).arg(dust_key).arg(pen_key).arg(ink_key);
    auto scrolllong = u"a %1 $B%2$R scroll %3 in %4 ink lies here."_s.arg(paper_key).arg(dust_key).arg(pen_key).arg(ink_key);

    paperobj->obj_flags.type_flag = ITEM_SCROLL;
    paperobj->obj_flags.value[0] = learned / 4 + ch->getLevel() / 2;
    paperobj->obj_flags.value[1] = spell;
    paperobj->obj_flags.value[2] = {};
    paperobj->obj_flags.value[3] = {};
    paperobj->name(scrollname);
    paperobj->short_description(scrollshort);
    paperobj->long_description(scrolllong);

    // We set the item to custom so that it will save uniquely every time
    SET_BIT(paperobj->obj_flags.more_flags, ITEM_CUSTOM);

    extract_obj(penobj);
    extract_obj(dustobj);
  }

  return ReturnValue::eSUCCESS;
}

Scribe::Scribe(void)
{
  if (initialized == false)
  {
    load();
    initialized = true;
  }
}

Scribe::~Scribe()
{
}

void Scribe::load(void)
{
  std::ifstream ifs(RECIPES_FILENAME, std::ios_base::in);
  if (!ifs.is_open())
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Unable to open %s.", RECIPES_FILENAME);
    return;
  }

  recipes.clear();

  try
  {
    while (!ifs.eof())
    {
      qint32 spell = {};
      recipe r = {0, 0, 0, 0};

      ifs >> r.ink;
      ifs >> r.dust;
      ifs >> r.pen;
      ifs >> r.paper;
      ifs >> spell;

      // Don't insert empty entries
      if (r.ink && r.dust && r.pen && r.paper && spell)
      {
        recipes.insert(std::make_pair(r, spell));
      }
    }
  }
  catch (loadError &)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Error loading %s.", RECIPES_FILENAME);
  }
}

void Scribe::save(void)
{
  std::ofstream ofs(RECIPES_FILENAME, std::ios_base::trunc);
  if (!ofs.is_open())
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Unable to open %s.", RECIPES_FILENAME);
    return;
  }

  try
  {
    for (QMap<recipe, qint32>::iterator iter = recipes.begin(); iter != recipes.end(); ++iter)
    {
      std::pair<recipe, qint32> p = *iter;
      recipe r = p.first;
      qint32 spell = p.second;

      ofs << r.ink << " " << r.dust << " " << r.pen << " " << r.paper << " " << spell << std::endl;
    }
  }
  catch (...)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Error saving %s.", RECIPES_FILENAME);
  }
}

void Scribe::list(CharacterPtr ch)
{
  QString buffer;
  qint32 i = {};

  if (ch == 0)
  {
    return;
  }

  ch->sendln("[# ] [ink #] [dust #] [pen #] [paper #] Spell Name\r\n");
  for (QMap<recipe, qint32>::reverse_iterator iter = recipes.rbegin(); iter != recipes.rend(); ++iter)
  {
    recipe r = iter->first;
    qint32 spell = iter->second;

    sprinttype(spell - 1, spells, buffer);
    ch->send(u"[%2d] [%5d] [%6d] [%5d] [%7d] %s (%d)\r\n"_s.arg(++i, r.ink, r.dust, r.pen).arg(r.paper).arg(buffer).arg(spell));
  }
}

qint32 Scribe::add(CharacterPtr ch, QString argument)
{
  vnum_t ink_vnum = {}, dust_vnum = {}, pen_vnum = {}, paper_vnum = {};
  qint32 spell = {};
  QString arg1, arg2, arg3, arg4, arg5;

  if (!ch)
  {
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  argument = one_argument(argument, arg4);
  argument = one_argument(argument, arg5);

  if (arg.isEmpty() 1 || arg.isEmpty() 2 || arg.isEmpty() 3 || arg.isEmpty() 4 || arg.isEmpty() 5)
  {
    ch->sendln("Syntax: scribe add [ink_vnum] [dust_vnum] [pen_vnum] [paper_vnum] [spell_num]");
    return ReturnValue::eFAILURE;
  }

  ink_vnum = atoi(arg1);
  dust_vnum = atoi(arg2);
  pen_vnum = atoi(arg3);
  paper_vnum = atoi(arg4);

  if (ink_vnum < 6326 || ink_vnum > 6328)
  {
    ch->sendln("Only vnums 6326-6328 are valid inks.");
    return ReturnValue::eFAILURE;
  }

  if (dust_vnum < 6335 || dust_vnum > 6337)
  {
    ch->sendln("Only vnums 6335-6337 are valid dusts.");
    return ReturnValue::eFAILURE;
  }

  if (pen_vnum < 6329 || pen_vnum > 6334)
  {
    ch->sendln("Only vnums 6329-6334 are valid pens.");
    return ReturnValue::eFAILURE;
  }

  if (paper_vnum < 6338 || paper_vnum > 6342)
  {
    ch->sendln("Only vnums 6338-6342 are valid papers.");
    return ReturnValue::eFAILURE;
  }

  if ((spell = find_skill_num(arg5)) < 0)
  {
    ch->send(u"Cannot find spell '%s' in master spell list.\r\n"_s.arg(arg4));
    return ReturnValue::eFAILURE;
  }

  recipe r = {ink_vnum, dust_vnum, pen_vnum, paper_vnum};
  recipes.insert(std::make_pair(r, spell));

  ch->sendln("New scribe recipe added.");

  return ReturnValue::eSUCCESS;
}

qint32 Scribe::remove(CharacterPtr ch, QString argument)
{
  if (!ch)
  {
    return ReturnValue::eFAILURE;
  }

  if (argument.isEmpty())
  {
    ch->sendln("Syntax: scribe remove [recipe_num]");
    return ReturnValue::eFAILURE;
  }

  qint32 i = {};
  qint32 target = atoi(argument);

  for (QMap<recipe, qint32>::reverse_iterator iter = recipes.rbegin(); iter != recipes.rend(); ++iter)
  {
    if (++i == target)
    {
      recipes.erase((*iter).first);
      ch->send(u"Recipe # %1 has been removed.\r\n"_s.arg(target));

      return ReturnValue::eSUCCESS;
    }
  }

  ch->send(u"Recipe # %1 not found.\r\n"_s.arg(target));
  return ReturnValue::eFAILURE;
}

qint32 Scribe::size(void)
{
  return recipes.size();
}

qint32 Scribe::find(Scribe::recipe r)
{
  qint32 spell = {};

  QMap<recipe, qint32>::iterator result = recipes.find(r);
  if (result != recipes.end())
  {
    spell = result->second;
  }

  return spell;
}

const QString Brew::RECIPES_FILENAME = "brewables.dat";
QMap<Brew::recipe, qint32> Brew::recipes;
bool Brew::initialized = false;
