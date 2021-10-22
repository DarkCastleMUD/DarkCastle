// This file takes care of all the skills that make stuff by combining it
// in it's container type.  For example, poison making.


#include "db.h"
#include "fight.h"
#include "room.h"
#include "obj.h"
#include "connect.h"
#include "utility.h"
#include "character.h"
#include "handler.h"
#include "db.h"
#include "player.h"
#include "levels.h"
#include "interp.h"
#include "magic.h"
#include "act.h"
#include "mobile.h"
#include "spells.h"
#include <string.h> // strstr()
#include "returnvals.h"
#include "combinables.h"
#include "const.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>
#include <utility>
#include <string>
#include <sstream>

using namespace std;
using namespace Combinables;

extern struct index_data *obj_index; 

////////////////////////////////////////////////////////////////////////////
// local function declarations
void determine_trade_skill_increase(char_data * ch, int skillnum, int learned, int trivial);
int determine_trade_skill_chance(int learned, int trivial);
int valid_trade_skill_combine(struct obj_data * container, struct trade_data_type * data, char_data * ch);

////////////////////////////////////////////////////////////////////////////
// local definitions

#define TRADE_SKILL_POISON_CONT    698    // mortar & pestle

char * tradeskills[] = 
{
  "poisonmaking",
  "\n"
};

#define MAX_INGREDIANTS   10

struct trade_data_type {
   int  pieces[MAX_INGREDIANTS];
   int  result;  // last item in array must be -1 for this
   int  trivial;
};

// POISON DEFINES
struct trade_data_type poison_vial_data[] = 
{
   { { 600, 697, -1, -1, -1, -1, -1, -1, -1, -1 },   // bee stinger
     696,
     15         // a vial of bee stinger poison
   },

   { { 3175, 697, -1, -1, -1, -1, -1, -1, -1, -1 },  // foraged apple
     695,
     10         // a small vial of low quality cyanide
   },

   { { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },  // not makeable
     694,
     10         // a vial of crude strychnine
   },

   { { 697, 6447, -1, -1, -1, -1, -1, -1, -1, -1 },  // vial, albino bat blood
     693,
     25         // a vial of vampire kiss
   },

   { { 697, 1569, -1, -1, -1, -1, -1, -1, -1, -1 },  // vial, flaxen vine
     692,
     30         // a vial of flaxen veleno
   },

   // This must come last
   { { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
     -1,
     -1
   }
};

struct thief_poison_data {
   char * poison_type;
};

struct thief_poison_data poison_vial_combat_data[] =
{
   {
      "bee stinger poison"
   },

   {
      "low quality cyanide"
   },

   {
      "crude strychnine"
   },

   {
      "vampire kiss poison"
   },

   {
      "flaxen valeno"
   },

   {
      "invalid poison type"
   }
};


////////////////////////////////////////////////////////////////////////////
// command functions

int do_poisonmaking(struct char_data *ch, char *argument, int cmd)
{
  int learned = has_skill(ch, SKILL_TRADE_POISON);

  if(GET_LEVEL(ch) > IMMORTAL)
    learned = 500;

  if(!learned) {
    send_to_char("You do not know how to make poisons.\r\n", ch);
    return eFAILURE;
  }

  // TODO search for mortar and pestle
  obj_data * container = get_obj_in_list_vis(ch, TRADE_SKILL_POISON_CONT, ch->carrying);

  if(!container) {
    send_to_char("You have nothing to make poisons in.\r\n", ch);
    return eFAILURE;
  }

  // search if the items in mortar and pestle match any poison vial types
  int index = valid_trade_skill_combine(container, poison_vial_data, ch);

  if(index == -2)
    return eFAILURE;

  send_to_char("You mix the ingrediants together in an attempt to make a poison..\r\n", ch);

  // Clear the items out of the container
  while(container->contains)
     extract_obj( container->contains );

  int failure = 0;
  int chance = 0;
  int percent;

  // determine success/failure

  if(index == -1)
     failure = 1;
  else {
     percent = number(1, 101);
     chance = determine_trade_skill_chance(learned, poison_vial_data[index].trivial);

     determine_trade_skill_increase(ch, SKILL_TRADE_POISON, learned, poison_vial_data[index].trivial);

     if(percent > chance)
        failure = 1;
  }

  if(failure) {
     send_to_char("Ahh crap, something got screwed up.  You are forced to throw away this attempt.\r\n", ch);
     act("$n fiddles with a mortar and pestle and looks unhappy.", ch, 0, 0, TO_ROOM, 0);
     return eFAILURE;
  }
    
  // give poison to player
  int rewardnum = real_object(poison_vial_data[index].result);
  if(rewardnum < 0) {
     send_to_char("That poison is broken.  Tell a god.\r\n", ch);
     return (eFAILURE|eINTERNAL_ERROR);
  }

  obj_data * reward = clone_object(rewardnum);
  obj_to_char(reward, ch);
  csendf(ch, "You succesfully make a %s!\r\n", reward->short_description);
  act("$n successfully makes a $p.", ch, reward, 0, TO_ROOM, 0);

  return eSUCCESS;
}

int do_poisonweapon(struct char_data *ch, char *argument, int cmd)
{
  if(GET_CLASS(ch) != CLASS_THIEF && GET_LEVEL(ch) <= IMMORTAL) {
    send_to_char("Only thieves are trained enough to poison their weapons.\r\n", ch);
    return eFAILURE;
  }

  char vialarg[MAX_INPUT_LENGTH];

  one_argument(argument, vialarg);

  if(!*vialarg) {
    send_to_char("Poison your weapon with what?\r\n", ch);
    return eFAILURE;
  }

  if(ch->fighting) {
    send_to_char("You can't do that while fighting!\r\n", ch);
    return eFAILURE;
  }

  // find weapon
  obj_data * weapon = ch->equipment[WIELD];
  if(!weapon) {
    send_to_char("You aren't wielding a weapon to poison.\r\n", ch);
    return eFAILURE;
  }

  // find vial and verify it's a valid poison vial
  obj_data * vial = get_obj_in_list_vis(ch, vialarg, ch->carrying);
  if(!vial) {
    csendf(ch, "You don't seem to have any %s.\r\n", vialarg);
    return eFAILURE;
  }

  int found = -1;
  for(int i = 0; poison_vial_data[i].result != -1; i++)
    if( poison_vial_data[i].result == obj_index[vial->item_number].virt )
    {
      found = i;
      break;
    }

  if(found < 0) {
    csendf(ch, "The %s is not a valid weapon poison.\r\n", vial->short_description );
    return eFAILURE;
  }

  for(int j = 0; j < weapon->num_affects; j++)
    if(weapon->affected[j].location == WEP_THIEF_POISON)
    {
      send_to_char("Your weapon is already poisoned.\r\n", ch);
      return eFAILURE;
    }

  // poison weapon
  add_obj_affect(weapon, WEP_THIEF_POISON, found);

  // remove vial
  extract_obj(vial);

  return eSUCCESS;
}

////////////////////////////////////////////////////////////////////////////
// Utility functions

// Search through the different types of data looking for a match
// Return index of match on successful find
// Return -1 on failure
// Return -2 if there's nothing in the container
int valid_trade_skill_combine(obj_data * container, trade_data_type * data, char_data * ch)
{
   if(!(container->contains)) {
     csendf(ch, "Your %s appears to be empty.\r\n", container->short_description);
     return -2;
   }

   vector<int> current;

   // take all the items in our container and put them in an array by vnum
   for(obj_data * j = container->contains; j; j = j->next_content)
      if(j->item_number >= 0)
         current.push_back(obj_index[j->item_number].virt);
      else return -1;  // only valid object ingrediants will match

   sort(current.begin(), current.end());

   vector<int> valid;

   // loop through the valid combinations, and try to find a match
   for(int i = 0; data[i].result != -1; i++)
   {
      valid.clear();
      for(int k = 0; k < MAX_INGREDIANTS && data[i].pieces[k] != -1; k++)
         valid.push_back(data[i].pieces[k]);

      sort(valid.begin(), valid.end());

      if(!valid.size()) // empty recipe (unmakeable)
        return -1;

      if(valid == current)  // if vectors match, then we have a valid combination
         return i;
   }

   return -1;  // didn't match any
}


int determine_trade_skill_chance(int learned, int trivial)
{
   int chance = 50;

   chance -= trivial;
   chance += learned;

   chance = MIN(chance, 90);
   chance = MAX(chance, 10);

   return chance;
}

void determine_trade_skill_increase(char_data * ch, int skillnum, int learned, int trivial)
{
   int learn_skill(char_data * ch, int skill, int amount, int maximum);

   // can't learn past item's trivial value
   if(learned >= trivial) {
      send_to_char("You have learned all you can from making this item.\r\n", ch);
      return;
   }

   // TODO - add other requirements when done debugging

   // learn a point
   learn_skill(ch, skillnum, 1, 500);
}

int handle_poisoned_weapon_attack(char_data * ch, char_data * vict, int type)
{
   int retval = eSUCCESS;
   // unused   int dam;

   if(!ch->equipment[WIELD]) {
      send_to_char("In handle_poisoned_weapon_atack() with null wield.  Tell a god.\r\n", ch);
      return (eFAILURE | eINTERNAL_ERROR);
   }

/*   switch(type)
   {
      case 0: // bee stinger poison
         if(saves_spell(ch, vict, 1, SAVE_TYPE_POISON) < 0)
           dam = 25;
         else dam = 15;
         retval = damage(ch, vict, dam, TYPE_POISON, POISON_MESSAGE_BASE+type, 0);         
         break;

      case 1: // low quality cyanide
         if(saves_spell(ch, vict, 10, SAVE_TYPE_POISON) < 0)
           dam = 35;
         else dam = 25;
         retval = damage(ch, vict, dam, TYPE_POISON, POISON_MESSAGE_BASE+type, 0);         
         break;

      case 2: // crude strychnine
         if(saves_spell(ch, vict, 10, SAVE_TYPE_POISON) < 0)
         {
           SET_BIT(vict->combat, COMBAT_MISS_AN_ATTACK);
           dam = 10;
         }
         else dam = 0;
         retval = damage(ch, vict, dam, TYPE_POISON, POISON_MESSAGE_BASE+type, 0);
         break;

      case 3: // vampire kiss
         retval = cast_vampiric_touch(6, ch, "", SPELL_TYPE_SPELL, vict, 0, 35);
         break;

      case 4: // flaxen veleno
         if(saves_spell(ch, vict, 1, SAVE_TYPE_POISON) < 0)
           dam = 40;
         else dam = 20;
         retval = damage(ch, vict, dam, TYPE_POISON, POISON_MESSAGE_BASE+type, 0);         
         break;

      default:
         csendf(ch, "Unknown poison type %d.  Let a god know.\r\n", type);
         break;
   }

   // you can do this even if the mob died, because the weapon is still valid
   remove_obj_affect_by_type(ch->equipment[WIELD], WEP_THIEF_POISON);
*/
   return retval;
}

int do_brew(char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH], liquid[MAX_STRING_LENGTH], container[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];
  OBJ_DATA *herbobj, *liquidobj, *containerobj;
  affected_type af;
  Brew b;

  int learned = has_skill(ch, SKILL_BREW);

  if (IS_PC(ch) && GET_LEVEL(ch) < IMMORTAL && !learned) {
    send_to_char("You just don't have the mind for potion brewing.\r\n", ch);
    return eFAILURE;
  }
  
  if (!*argument) {
      send_to_char("Brew what?\n\r"
		   "$3Syntax:$R brew <herb> <liquid> <container>\n\r", ch);
      if (GET_LEVEL(ch) >= 106) {
	send_to_char("        brew load\n\r"
		     "        brew save\n\r"
		     "        brew list\n\r"
		     "        brew add [herb_vnum] [liquid_type] [container_vnum] [spell_num]\n\r"
		     "        brew remove [recipe_num]\n\r\n\r", ch);
    }
    return eFAILURE;
  }

  argument = one_argument(argument, arg1);

  if (IS_PC(ch) && GET_LEVEL(ch) >= 106) {
    if (!str_cmp(arg1, "load")) {
      b.load();
      logf(108, LOG_WORLD, "Loaded %d brew recipes.", b.size());
      return eSUCCESS;
    } else if (!str_cmp(arg1, "save")) {
      b.save();
      logf(108, LOG_WORLD, "Saved %d brew recipes.", b.size());
      return eSUCCESS;
    } else if (!str_cmp(arg1, "list")) {
      b.list(ch);
      return eSUCCESS;
    } else if (!str_cmp(arg1, "add")) {
      return b.add(ch, argument);
    } else if (!str_cmp(arg1, "remove")) {
      return b.remove(ch, argument);
    }
  }

  if (affected_by_spell(ch, SKILL_BREW_TIMER)) {
    send_to_char("You aren't ready to brew anything again.\n\r", ch);
    return eFAILURE;
  }

  argument = one_argument(argument, liquid);
  one_argument(argument, container);

  if (!*liquid) {
    send_to_char("You'll need to choose a liquid type and container.\n\r"
		 "$3Syntax:$R brew <herb> <liquid> <container>\n\r", ch);
    return eFAILURE;
  }

  if (!*container) {
    send_to_char("You'll need to select a container.\n\r"
		 "$3Syntax:$R brew <herb> <liquid> <container>\n\r", ch);
    return eFAILURE;
  }

  herbobj = get_obj_in_list_vis(ch, arg1, ch->carrying);
  liquidobj = get_obj_in_list_vis(ch, liquid, ch->carrying);
  containerobj = get_obj_in_list_vis(ch, container, ch->carrying);

  if (!herbobj) {
    send_to_char("You do not have that type of herb.\n\r", ch);
    return eFAILURE;
  }
  if (herbobj->obj_flags.type_flag != ITEM_OTHER) {
    send_to_char("That is not an herb.\n\r", ch);
    return eFAILURE;
  }

  if (!liquidobj) {
    send_to_char("You do not have that type of liquid.\n\r", ch);
    return eFAILURE;
  }
  if (liquidobj->obj_flags.type_flag != ITEM_DRINKCON) {
    send_to_char("That is not a liquid container.\n\r", ch);
    return eFAILURE;
  }

  if (!containerobj) {
    send_to_char("You do not have that type of container.\n\r", ch);
    return eFAILURE;
  }

  if (containerobj->obj_flags.type_flag != ITEM_POTION && containerobj->obj_flags.type_flag != ITEM_DRINKCON) {
    send_to_char("That is not a target container.\n\r", ch);
    return eFAILURE;
  }
  
  if (IS_SET(containerobj->obj_flags.more_flags, ITEM_CUSTOM)) {
	  send_to_char("That container is already a brewed potion.\n\r", ch);
	  return eFAILURE;
  }

  if (containerobj == liquidobj) {
    send_to_char("Your liquid and target container cannot be the same object!\n\r", ch);
    return eFAILURE;
  }

  if (liquidobj->obj_flags.value[1] < 1) {
    send_to_char("There is no liquid left in that container.\n\r", ch);
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_BREW)) {
    return eFAILURE;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2.5);

  const char *potion_color;
  // Determine color to use in message based on herb used
  switch (obj_index[herbobj->item_number].virt) {
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
    potion_color = "$5brown$Rt";
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
  Brew::recipe r = { obj_index[herbobj->item_number].virt,
		     liquidobj->obj_flags.value[2],
		     obj_index[containerobj->item_number].virt };
  int spell = b.find(r);

  //  csendf(ch, "Searching for herb: %d(%s)\nliquid: %d(%s)\ncontainer: %d(%s).....%d\n",
  //	 obj_index[herbobj->item_number].virt, GET_OBJ_SHORT(herbobj),
  //	 liquidobj->obj_flags.value[2], GET_OBJ_SHORT(liquidobj),
  //	 obj_index[containerobj->item_number].virt, GET_OBJ_SHORT(containerobj), spell);
  
  if (spell == 0) {
    act("As you finish, nothing special seems to happen.", ch, 0, 0, TO_CHAR, 0);
    act("As $e finishes, nothing special seems to happen.", ch, 0, 0, TO_ROOM, 0);

    af.type = SKILL_BREW_TIMER;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = 1;
    af.bitvector = -1;
    affect_to_char(ch, &af);

  } else if (skill_success(ch, 0, SKILL_BREW)) {
    af.type = SKILL_BREW_TIMER;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = 1;
    af.bitvector = -1;
    affect_to_char(ch, &af);

    act("You sit down and carefully pour the ingredients into $o and give it a gentle shake to mix them.", ch, containerobj, 0, TO_CHAR, 0);
    snprintf(buffer, MAX_STRING_LENGTH, "As the $o disolves, the liquid turns %s.", potion_color);
    act(buffer, ch, herbobj, 0, TO_CHAR, 0);

    act("$n sits down and carefully pours ingredients into $o and gives it a gentle shake to mix them.", ch, containerobj, 0, TO_ROOM, 0);
    snprintf(buffer, MAX_STRING_LENGTH, "As $e finishes, the liquid turns %s.", potion_color);
    act(buffer, ch, containerobj, 0, TO_ROOM, 0);

    // Find container key (crude, plain, etc)
    char container_key[MAX_STRING_LENGTH];
    char *conargs = one_argument(containerobj->name, container_key);
    conargs = one_argument(conargs, container_key);
    one_argument(conargs, container_key);
    
    // Find liquid key (salty, milk, strong)
    const char *liquid_key;
    switch(liquidobj->obj_flags.value[2]) {
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
    stringstream potionname, potionshort, potionlong;
    potionname << "potion " << container_key << " " << liquid_key;
    potionshort << "a " << container_key << " " << liquid_key << " " << potion_color << " potion";
    potionlong << "a " << container_key << " " << liquid_key << " " << potion_color << " potion lies here.";

    containerobj->obj_flags.type_flag=ITEM_POTION;
    containerobj->obj_flags.value[0] = (learned/2 - 5) + GET_WIS(ch)/2 + GET_INT(ch)/2;
    containerobj->obj_flags.value[1] = spell;
    containerobj->obj_flags.value[2] = 0;
    containerobj->obj_flags.value[3] = 0;
    containerobj->name = str_dup(potionname.str().c_str());
    GET_OBJ_SHORT(containerobj) = str_dup(potionshort.str().c_str());
    containerobj->description = str_dup(potionlong.str().c_str());
    // We set the item to custom so that it will save everytime uniquely
    SET_BIT(containerobj->obj_flags.more_flags, ITEM_CUSTOM);

    extract_obj(herbobj);
  } else {
    act("You sit down and carefully pour the ingredients into $o and give it a gentle shake to mix them.", ch, containerobj, 0, TO_CHAR, 0);
    act("As you finish, the liquid begins to bubble furiously, cracking the $o and rendering your work useless!", ch, containerobj, 0, TO_CHAR, 0);

    act("$n sits down and carefully pours ingredients into $o and gives it a gentle shake to mix them.", ch, containerobj, 0, TO_ROOM, 0);
    act("As $e finishes, the liquid begins to bubble furiously, cracking the $o and rendering $s work useless!", ch, containerobj, 0, TO_ROOM, 0);

    extract_obj(containerobj);
    extract_obj(herbobj);
    if (liquidobj->obj_flags.value[1] > 0) {
      liquidobj->obj_flags.value[1]--;
    }
  }

  return eSUCCESS;
}

Brew::Brew(void) {
  if (initialized == false) {
    load();
    initialized = true;
  }
}

Brew::~Brew() {

}

void Brew::load(void) {
  ifstream ifs(RECIPES_FILENAME, ios_base::in);
  if (!ifs.is_open()) {
    logf(IMMORTAL, LOG_BUG, "Unable to open %s.", RECIPES_FILENAME);
    return;
  }

  recipes.clear();
  
  try {
    while(!ifs.eof()) {
      int spell = 0;
      recipe r = {0, 0, 0};
      
      ifs >> r.herb;
      ifs >> r.liquid;
      ifs >> r.container;
      ifs >> spell;
      
      // Don't insert empty entries
      if (r.container && r.liquid && r.herb && spell) {
	recipes.insert(make_pair(r, spell));
      }
    }
  } catch(loadError &) {
    logf(IMMORTAL, LOG_BUG, "Error loading %s.", RECIPES_FILENAME);
  }
}

void Brew::save(void) {
  ofstream ofs(RECIPES_FILENAME, ios_base::trunc);
  if (!ofs.is_open()) {
    logf(IMMORTAL, LOG_BUG, "Unable to open %s.", RECIPES_FILENAME);
    return;
  }  

  try {
    for (map<recipe, int>::iterator iter = recipes.begin(); iter != recipes.end(); ++iter) {
      pair<recipe, int> p = *iter;
      recipe r = p.first;
      int spell = p.second;
      
      ofs << r.herb << " " << r.liquid << " " << r.container << " " << spell << endl;
    }
  } catch(...) {
    logf(IMMORTAL, LOG_BUG, "Error saving %s.", RECIPES_FILENAME);
  }
}

void Brew::list(char_data *ch) {
  char buffer[MAX_STRING_LENGTH];
  int i = 0;

  if (ch == 0) {
    return;
  }

  send_to_char( "[# ] [herb #] [liquid] [container] Spell Name\n\r\n\r", ch);
  for (map<recipe, int>::reverse_iterator iter = recipes.rbegin(); iter != recipes.rend(); ++iter) {
    recipe r = iter->first;
    int spell = iter->second;

    sprinttype(spell-1, spells, buffer);
    csendf(ch, "[%2d] [%6d] [%6d] [%9d] %s (%d)\n\r", ++i, r.herb, r.liquid, r.container, buffer, spell);
  }

}

int Brew::add(char_data *ch, char *argument) {
  int herb_vnum, liquid_type, container_vnum, spell;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH];
  
  if (!ch) {
    return eFAILURE;
  }

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  argument = one_argument(argument, arg4);

  if (!*arg1 || !*arg2 || !*arg3 || !*arg4) {
    send_to_char("Syntax: brew add [herb_vnum] [liquid_type] [container_vnum] [spell_num]\n\r", ch);
    return eFAILURE;
  }

  herb_vnum = atoi(arg1);
  liquid_type = atoi(arg2);
  container_vnum = atoi(arg3);

  if (herb_vnum < 6301 || herb_vnum > 6312) {
    send_to_char("Only vnums 6301-6312 are valid herbs.\n\r", ch);
    return eFAILURE;
  }

  switch(liquid_type) {
  case LIQ_MILK:
  case LIQ_WINE:
  case LIQ_SALTWATER:
    break;
  default:
    csendf(ch, "Invalid liquid type. Only Milk (%d), Wine (%d), Salt Water (%d) allowed.\n\r",
	   LIQ_MILK, LIQ_WINE, LIQ_SALTWATER);
    return eFAILURE;
    break;
  }

  if (container_vnum < 6320 || container_vnum > 6324) {
    send_to_char("Only vnums 6320-6324 are valid containers.\n\r", ch);
    return eFAILURE;
  }

  if((spell = find_skill_num(arg4)) < 0) {
    csendf(ch, "Cannot find spell '%s' in master spell list.\r\n", arg4);
    return eFAILURE;
  }

  recipe r = { herb_vnum, liquid_type, container_vnum }; 
  recipes.insert(make_pair(r, spell));

  send_to_char("New brew recipe added.\n\r", ch);

  return eSUCCESS;
}

int Brew::remove(char_data *ch, char *argument) {
  if (!ch)  {
    return eFAILURE;
  }

  if (!*argument) {
    send_to_char("Syntax: brew remove [recipe_num]\n\r", ch);
    return eFAILURE;
  }

  int i = 0;
  int target = atoi(argument);

  for (map<recipe, int>::reverse_iterator iter = recipes.rbegin(); iter != recipes.rend(); ++iter) {
    if (++i == target) {
      recipes.erase((*iter).first);
      csendf(ch, "Recipe # %d has been removed.\n\r", target);

      return eSUCCESS;
    }
  }

  csendf(ch, "Recipe # %d not found.\n\r", target);
  return eFAILURE;
}

int Brew::size(void) {
  return recipes.size();
}

int Brew::find(Brew::recipe r) {
  int spell = 0;

  map<recipe, int>::iterator result = recipes.find(r);
  if (result != recipes.end()) {
    spell = result->second;
  }

  return spell;
}

int do_scribe(char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_STRING_LENGTH], dust[MAX_STRING_LENGTH], pen[MAX_STRING_LENGTH], paper[MAX_STRING_LENGTH];
  OBJ_DATA *inkobj, *dustobj, *penobj, *paperobj;
  affected_type af;
  Scribe s;

  int learned = has_skill(ch, SKILL_SCRIBE);

  if (IS_PC(ch) && GET_LEVEL(ch) < IMMORTAL && !learned) {
    send_to_char("You just don't have the mind for scribing.\r\n", ch);
    return eFAILURE;
  }
  
  if (!*argument) {
      send_to_char("Scribe what?\n\r"
		   "$3Syntax:$R scribe <ink> <dust> <pen> <paper>\n\r", ch);
      if (GET_LEVEL(ch) >= 106) {
	send_to_char("        scribe load\n\r"
		     "        scribe save\n\r"
		     "        scribe list\n\r"
		     "        scribe add <ink_vnum> <dust_vnum> <pen_vnum> <paper_vnum> <spell_num>\n\r"
		     "        scribe remove [recipe_num]\n\r\n\r", ch);
    }
    return eFAILURE;
  }

  argument = one_argument(argument, arg1);

  if (IS_PC(ch) && GET_LEVEL(ch) >= 106) {
    if (!str_cmp(arg1, "load")) {
      s.load();
      logf(108, LOG_WORLD, "Loaded %d scribe recipes.", s.size());
      return eSUCCESS;
    } else if (!str_cmp(arg1, "save")) {
      s.save();
      logf(108, LOG_WORLD, "Saved %d scribe recipes.", s.size());
      return eSUCCESS;
    } else if (!str_cmp(arg1, "list")) {
      s.list(ch);
      return eSUCCESS;
    } else if (!str_cmp(arg1, "add")) {
      return s.add(ch, argument);
    } else if (!str_cmp(arg1, "remove")) {
      return s.remove(ch, argument);
    }
  }

  if (affected_by_spell(ch, SKILL_SCRIBE_TIMER)) {
    send_to_char("You aren't ready to scribe anything again.\n\r", ch);
    return eFAILURE;
  }

  argument = one_argument(argument, dust);
  argument = one_argument(argument, pen);
  one_argument(argument, paper);

  if (!*dust) {
    send_to_char("You'll need to choose a dust, pen and paper.\n\r"
		 "$3Syntax:$R scribe <ink> <dust> <pen> <paper>\n\r", ch);
    return eFAILURE;
  }

  if (!*pen) {
    send_to_char("You'll need to choose a pen and paper.\n\r"
		 "$3Syntax:$R scribe <ink> <dust> <pen> <paper>\n\r", ch);
    return eFAILURE;
  }

  if (!*paper) {
    send_to_char("You'll need to select a paper.\n\r"
		 "$3Syntax:$R scribe <ink> <dust> <pen> <paper>\n\r", ch);
    return eFAILURE;
  }

  inkobj = get_obj_in_list_vis(ch, arg1, ch->carrying);
  dustobj = get_obj_in_list_vis(ch, dust, ch->carrying);
  penobj = get_obj_in_list_vis(ch, pen, ch->carrying);
  paperobj = get_obj_in_list_vis(ch, paper, ch->carrying);

  if (!inkobj) {
    send_to_char("You do not have that type of ink.\n\r", ch);
    return eFAILURE;
  }
  if (inkobj->obj_flags.type_flag != ITEM_DRINKCON || inkobj->obj_flags.value[2] != LIQ_INK) {
    send_to_char("That is not ink.\n\r", ch);
    return eFAILURE;
  }

  if (!dustobj) {
    send_to_char("You do not have that type of dust.\n\r", ch);
    return eFAILURE;
  }
  if (dustobj->obj_flags.type_flag != ITEM_OTHER) {
    send_to_char("That is not dust.\n\r", ch);
    return eFAILURE;
  }

  if (!penobj) {
    send_to_char("You do not have that type of pen.\n\r", ch);
    return eFAILURE;
  }
  if (penobj->obj_flags.type_flag != ITEM_PEN) {
    send_to_char("That is not a pen.\n\r", ch);
    return eFAILURE;
  }

  if (!paperobj) {
    send_to_char("You do not have that type of paper.\n\r", ch);
    return eFAILURE;
  }
  if (paperobj->obj_flags.type_flag != ITEM_SCROLL) {
    send_to_char("That is not paper.\n\r", ch);
    return eFAILURE;
  }
  
  if (inkobj->obj_flags.value[1] < 1) {
    send_to_char("There is no liquid left in that ink container.\n\r", ch);
    return eFAILURE;
  }

  if (!charge_moves(ch, SKILL_SCRIBE)) {
    return eFAILURE;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2.5);

  // Search for the current combination as a recipe
  Scribe::recipe r = { obj_index[inkobj->item_number].virt,
		       obj_index[dustobj->item_number].virt,
		       obj_index[penobj->item_number].virt,
		       obj_index[paperobj->item_number].virt };
  int spell = s.find(r);
  
  act("You sit down and carefully inscribe the words of the gods onto the parchment.", ch, 0, 0, TO_CHAR, 0);
  act("$n sits down and carefully inscribes the words of the gods onto some parchment.", ch, 0, 0, TO_ROOM, 0);

  if (inkobj->obj_flags.value[1] > 0) {
    inkobj->obj_flags.value[1]--;
  }

  if (spell == 0) {
    act("As you finish, nothing special seems to happen.", ch, 0, 0, TO_CHAR, 0);
    act("As $e finishes, nothing special seems to happen.", ch, 0, 0, TO_ROOM, 0);

    af.type = SKILL_SCRIBE_TIMER;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = 1;
    af.bitvector = -1;
    affect_to_char(ch, &af);

  } else if (!skill_success(ch, 0, SKILL_SCRIBE)) {
    act("As you finish, the letters on the newly minted scroll burst into $B$4flame$R leaving nothing but ash!", ch, 0, 0, TO_CHAR, 0);
    act("As $e finishes, the letters on the newly minted scroll burst into $B$4flame$R leaving nothing but ash!", ch, 0, 0, TO_ROOM, 0);
    extract_obj(paperobj);
    extract_obj(penobj);
    extract_obj(dustobj);
    // 50% of a failure causing 'wild magic' to be cast on self
    if (number(1,100) > 50) {
      cast_wild_magic(GET_LEVEL(ch), ch, "offense", 0, ch, 0, 0);
    }

  } else {
    act("As you finish, the letters on the newly minted scroll $Bglow$R briefly and return to normal.", ch, 0, 0, TO_CHAR, 0);
    act("As $e finishes, the letters on the newly minted scroll $Bglow$R briefly and return to normal.", ch, 0, 0, TO_ROOM, 0);

    af.type = SKILL_SCRIBE_TIMER;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = 1;
    af.bitvector = -1;
    affect_to_char(ch, &af);

    char ink_key[MAX_STRING_LENGTH], dust_key[MAX_STRING_LENGTH],
      pen_key[MAX_STRING_LENGTH], paper_key[MAX_STRING_LENGTH];
    char *args;

    args = one_argument(inkobj->name, ink_key);
    args = one_argument(args, ink_key);
    args = one_argument(args, ink_key);

    args = one_argument(dustobj->name, dust_key);
    args = one_argument(args, dust_key);
    args = one_argument(args, dust_key);

    args = one_argument(penobj->name, pen_key);
    args = one_argument(args, pen_key);
    args = one_argument(args, pen_key);

    args = one_argument(paperobj->name, paper_key);
    args = one_argument(args, paper_key);
    args = one_argument(args, paper_key);

    // Put it all together into the new name
    stringstream scrollname, scrollshort, scrolllong;
    scrollname << "scroll " << paper_key << " " << dust_key << " " << pen_key << " " << ink_key;
    scrollshort << "a " << paper_key << " $B" << dust_key << "$R scroll " << pen_key << " in " << ink_key << " ink";
    scrolllong << "a " << paper_key << " " << dust_key << " scroll " << pen_key << " in " << ink_key << "ink lies here";

    paperobj->obj_flags.type_flag=ITEM_SCROLL;
    paperobj->obj_flags.value[0] = learned/4 + GET_LEVEL(ch)/2;
    paperobj->obj_flags.value[1] = spell;
    paperobj->obj_flags.value[2] = 0;
    paperobj->obj_flags.value[3] = 0;
    paperobj->name = str_dup(scrollname.str().c_str());
    GET_OBJ_SHORT(paperobj) = str_dup(scrollshort.str().c_str());
    paperobj->description = str_dup(scrolllong.str().c_str());

    // We set the item to custom so that it will save uniquely every time
    SET_BIT(paperobj->obj_flags.more_flags, ITEM_CUSTOM);

    extract_obj(penobj);
    extract_obj(dustobj);
  }

  return eSUCCESS;
}

Scribe::Scribe(void) {
  if (initialized == false) {
    load();
    initialized = true;
  }
}

Scribe::~Scribe() {

}

void Scribe::load(void) {
  ifstream ifs(RECIPES_FILENAME, ios_base::in);
  if (!ifs.is_open()) {
    logf(IMMORTAL, LOG_BUG, "Unable to open %s.", RECIPES_FILENAME);
    return;
  }

  recipes.clear();
  
  try {
    while(!ifs.eof()) {
      int spell = 0;
      recipe r = {0, 0, 0, 0};
      
      ifs >> r.ink;
      ifs >> r.dust;
      ifs >> r.pen;
      ifs >> r.paper;
      ifs >> spell;
      
      // Don't insert empty entries
      if (r.ink && r.dust && r.pen && r.paper && spell) {
	recipes.insert(make_pair(r, spell));
      }
    }
  } catch(loadError &) {
    logf(IMMORTAL, LOG_BUG, "Error loading %s.", RECIPES_FILENAME);
  }
}

void Scribe::save(void) {
  ofstream ofs(RECIPES_FILENAME, ios_base::trunc);
  if (!ofs.is_open()) {
    logf(IMMORTAL, LOG_BUG, "Unable to open %s.", RECIPES_FILENAME);
    return;
  }  

  try {
    for (map<recipe, int>::iterator iter = recipes.begin(); iter != recipes.end(); ++iter) {
      pair<recipe, int> p = *iter;
      recipe r = p.first;
      int spell = p.second;
      
      ofs << r.ink << " " << r.dust << " " << r.pen << " " << r.paper << " " << spell << endl;
    }
  } catch(...) {
    logf(IMMORTAL, LOG_BUG, "Error saving %s.", RECIPES_FILENAME);
  }
}

void Scribe::list(char_data *ch) {
  char buffer[MAX_STRING_LENGTH];
  int i = 0;

  if (ch == 0) {
    return;
  }

  send_to_char( "[# ] [ink #] [dust #] [pen #] [paper #] Spell Name\n\r\n\r", ch);
  for (map<recipe, int>::reverse_iterator iter = recipes.rbegin(); iter != recipes.rend(); ++iter) {
    recipe r = iter->first;
    int spell = iter->second;

    sprinttype(spell-1, spells, buffer);
    csendf(ch, "[%2d] [%5d] [%6d] [%5d] [%7d] %s (%d)\n\r", ++i, r.ink, r.dust, r.pen, r.paper, buffer, spell);
  }
}

int Scribe::add(char_data *ch, char *argument) {
  int ink_vnum, dust_vnum, pen_vnum, paper_vnum, spell;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH], arg5[MAX_INPUT_LENGTH];
  
  if (!ch) {
    return eFAILURE;
  }

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  argument = one_argument(argument, arg4);
  argument = one_argument(argument, arg5);

  if (!*arg1 || !*arg2 || !*arg3 || !*arg4 || !*arg5) {
    send_to_char("Syntax: scribe add [ink_vnum] [dust_vnum] [pen_vnum] [paper_vnum] [spell_num]\n\r", ch);
    return eFAILURE;
  }

  ink_vnum = atoi(arg1);
  dust_vnum = atoi(arg2);
  pen_vnum = atoi(arg3);
  paper_vnum = atoi(arg4);

  if (ink_vnum < 6326 || ink_vnum > 6328) {
    send_to_char("Only vnums 6326-6328 are valid inks.\n\r", ch);
    return eFAILURE;
  }

  if (dust_vnum < 6335 || dust_vnum > 6337) {
    send_to_char("Only vnums 6335-6337 are valid dusts.\n\r", ch);
    return eFAILURE;
  }

  if (pen_vnum < 6329 || pen_vnum > 6334) {
    send_to_char("Only vnums 6329-6334 are valid pens.\n\r", ch);
    return eFAILURE;
  }

  if (paper_vnum < 6338 || paper_vnum > 6342) {
    send_to_char("Only vnums 6338-6342 are valid papers.\n\r", ch);
    return eFAILURE;
  }

  if((spell = find_skill_num(arg5)) < 0) {
    csendf(ch, "Cannot find spell '%s' in master spell list.\r\n", arg4);
    return eFAILURE;
  }

  recipe r = { ink_vnum, dust_vnum, pen_vnum, paper_vnum }; 
  recipes.insert(make_pair(r, spell));

  send_to_char("New scribe recipe added.\n\r", ch);

  return eSUCCESS;
}

int Scribe::remove(char_data *ch, char *argument) {
  if (!ch)  {
    return eFAILURE;
  }

  if (!*argument) {
    send_to_char("Syntax: scribe remove [recipe_num]\n\r", ch);
    return eFAILURE;
  }

  int i = 0;
  int target = atoi(argument);

  for (map<recipe, int>::reverse_iterator iter = recipes.rbegin(); iter != recipes.rend(); ++iter) {
    if (++i == target) {
      recipes.erase((*iter).first);
      csendf(ch, "Recipe # %d has been removed.\n\r", target);

      return eSUCCESS;
    }
  }

  csendf(ch, "Recipe # %d not found.\n\r", target);
  return eFAILURE;
}

int Scribe::size(void) {
  return recipes.size();
}

int Scribe::find(Scribe::recipe r) {
  int spell = 0;

  map<recipe, int>::iterator result = recipes.find(r);
  if (result != recipes.end()) {
    spell = result->second;
  }

  return spell;
}
