// This file takes care of all the skills that make stuff by combining it
// in it's container type.  For example, poison making.


#include <db.h>
#include <fight.h>
#include <room.h>
#include <obj.h>
#include <connect.h>
#include <utility.h>
#include <character.h>
#include <handler.h>
#include <db.h>
#include <player.h>
#include <levels.h>
#include <interp.h>
#include <magic.h>
#include <act.h>
#include <mobile.h>
#include <spells.h>
#include <string.h> // strstr()
#include <returnvals.h>

#include <vector>
#include <algorithm>
using namespace std;

extern CWorld world;
extern struct index_data *obj_index; 

int saves_spell(CHAR_DATA *ch, CHAR_DATA *vict, int spell_base, int16 save_type);

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

int do_scribe(struct char_data *ch, char *argument, int cmd)
{
  char ink[MAX_STRING_LENGTH], pen[MAX_STRING_LENGTH], paper[MAX_STRING_LENGTH];
  OBJ_DATA *inkobj, *penobj, *paperobj;
  int spellnum = 0;
  bool scrollok = FALSE;

  if(!*argument) {
    send_to_char("Scribe what?\n\rsyntax: scribe <ink> <pen> <paper>\n\r", ch);
    return eFAILURE;
  }

  argument = one_argument(argument, ink);
  argument = one_argument(argument, pen);
  argument = one_argument(argument, paper);

  if(!*pen) {
    send_to_char("You must choose a pen and paper.\n\rsyntax: scribe <ink> <pen> <paper>\n\r", ch);
    return eFAILURE;
  }

  if(!*paper) {
    send_to_char("You must choose a piece of paper on which to scribe.\n\rsyntax: scribe <ink> <pen> <paper>\n\r", ch);
    return eFAILURE;
  }

  inkobj = get_obj_in_list_vis(ch, ink, ch->carrying);
  penobj = get_obj_in_list_vis(ch, pen, ch->carrying);
  paperobj = get_obj_in_list_vis(ch, paper, ch->carrying);

  if(!inkobj) {
    send_to_char("You do not have that type of ink.\n\r", ch);
    return eFAILURE;
  }
  if(!penobj) {
    send_to_char("You do not have that type of pen.\n\r", ch);
    return eFAILURE;
  }
  if(!paperobj) {
    send_to_char("You do not have that type of paper.\n\r", ch);
    return eFAILURE;
  }

  spellnum = inkobj->obj_flags.value[0] + penobj->obj_flags.value[0] + paperobj->obj_flags.value[0];

  switch(spellnum) {
    case SPELL_ARMOR:
    case SPELL_TELEPORT:
    case SPELL_BLESS:
    case SPELL_BLINDNESS:
    case SPELL_BURNING_HANDS:
    case SPELL_IRIDESCENT_AURA:
    case SPELL_CHARM_PERSON:
    case SPELL_CHILL_TOUCH:
    case SPELL_CLONE:
    case SPELL_COLOUR_SPRAY:
    case SPELL_CONTROL_WEATHER:
    case SPELL_CREATE_FOOD:
    case SPELL_CREATE_WATER:
    case SPELL_REMOVE_BLIND:
    case SPELL_CURE_CRITIC:
    case SPELL_CURE_LIGHT:
    case SPELL_CURSE:
    case SPELL_DETECT_EVIL:
    case SPELL_DETECT_INVISIBLE:
    case SPELL_DETECT_MAGIC:
    case SPELL_DETECT_POISON:
    case SPELL_DISPEL_EVIL:
    case SPELL_EARTHQUAKE:
    case SPELL_ENCHANT_WEAPON:
    case SPELL_ENERGY_DRAIN:
    case SPELL_FIREBALL:
    case SPELL_HARM:
    case SPELL_HEAL:
    case SPELL_INVISIBLE:
    case SPELL_LIGHTNING_BOLT:
    case SPELL_LOCATE_OBJECT:
    case SPELL_MAGIC_MISSILE:
    case SPELL_POISON:
    case SPELL_PROTECT_FROM_EVIL:
    case SPELL_REMOVE_CURSE:
    case SPELL_SANCTUARY:
    case SPELL_SHOCKING_GRASP:
    case SPELL_SLEEP:
    case SPELL_STRENGTH:
    case SPELL_SUMMON:
    case SPELL_VENTRILOQUATE:
    case SPELL_WORD_OF_RECALL:
    case SPELL_REMOVE_POISON:
    case SPELL_SENSE_LIFE:
    case SPELL_SUMMON_FAMILIAR:
    case SPELL_LIGHTED_PATH:
    case SPELL_RESIST_ACID:
    case SPELL_SUN_RAY:
    case SPELL_RAPID_MEND:
    case SPELL_ACID_SHIELD:
    case SPELL_WATER_BREATHING:
    case SPELL_GLOBE_OF_DARKNESS:
    case SPELL_IDENTIFY:
    case SPELL_ANIMATE_DEAD:
    case SPELL_FEAR:
    case SPELL_FLY:
    case SPELL_CONT_LIGHT:
    case SPELL_KNOW_ALIGNMENT:
    case SPELL_DISPEL_MAGIC:
    case SPELL_CONJURE_ELEMENTAL:
    case SPELL_CURE_SERIOUS:
    case SPELL_CAUSE_LIGHT:
    case SPELL_CAUSE_CRITICAL:
    case SPELL_CAUSE_SERIOUS:
    case SPELL_FLAMESTRIKE:
    case SPELL_STONE_SKIN:
    case SPELL_SHIELD:
    case SPELL_WEAKEN:
    case SPELL_MASS_INVISIBILITY:
    case SPELL_ACID_BLAST:
    case SPELL_PORTAL:
    case SPELL_INFRAVISION:
    case SPELL_REFRESH:
    case SPELL_HASTE:
    case SPELL_DISPEL_GOOD:
    case SPELL_HELLSTREAM:
    case SPELL_POWER_HEAL:
    case SPELL_FULL_HEAL:
    case SPELL_FIRESTORM:
    case SPELL_POWER_HARM:
    case SPELL_DETECT_GOOD:
    case SPELL_VAMPIRIC_TOUCH:
    case SPELL_LIFE_LEECH:
    case SPELL_PARALYZE:
    case SPELL_REMOVE_PARALYSIS:
    case SPELL_FIRESHIELD:
    case SPELL_METEOR_SWARM:
    case SPELL_WIZARD_EYE:
    case SPELL_TRUE_SIGHT:
    case SPELL_MANA:
    case SPELL_SOLAR_GATE:
    case SPELL_HEROES_FEAST:
    case SPELL_HEAL_SPRAY:
    case SPELL_GROUP_SANC:
    case SPELL_GROUP_RECALL:
    case SPELL_GROUP_FLY:
    case SPELL_ENCHANT_ARMOR:
    case SPELL_RESIST_FIRE:
    case SPELL_RESIST_COLD:
    case SPELL_BEE_STING:
    case SPELL_BEE_SWARM:
    case SPELL_CREEPING_DEATH:
    case SPELL_BARKSKIN:
    case SPELL_HERB_LORE:
    case SPELL_CALL_FOLLOWER:
    case SPELL_ENTANGLE:
    case SPELL_EYES_OF_THE_OWL:
    case SPELL_FELINE_AGILITY:
    case SPELL_FOREST_MELD:
    case SPELL_COMPANION:
    case SPELL_DROWN:
    case SPELL_HOWL:
    case SPELL_SOULDRAIN:
    case SPELL_SPARKS:
    case SPELL_CAMOUFLAGE:
    case SPELL_FARSIGHT:
    case SPELL_FREEFLOAT:
    case SPELL_INSOMNIA:
    case SPELL_SHADOWSLIP:
    case SPELL_RESIST_ENERGY:
    case SPELL_STAUNCHBLOOD:
    case SPELL_CREATE_GOLEM:
    case SPELL_REFLECT:
    case SPELL_DISPEL_MINOR:
    case SPELL_RELEASE_GOLEM:
    case SPELL_BEACON:
    case SPELL_STONE_SHIELD:
    case SPELL_GREATER_STONE_SHIELD:
    case SPELL_IRON_ROOTS:
    case SPELL_EYES_OF_THE_EAGLE:
    case SPELL_MISANRA_QUIVER:
    case SPELL_ICESTORM:
    case SPELL_LIGHTNING_SHIELD:
    case SPELL_BLUE_BIRD:
    case SPELL_DEBILITY:
    case SPELL_ATTRITION:
    case SPELL_VAMPIRIC_AURA:
    case SPELL_HOLY_AURA:
    case SPELL_DISMISS_FAMILIAR:
    case SPELL_DISMISS_CORPSE:
    case SPELL_BLESSED_HALO:
    case SPELL_VISAGE_OF_HATE:
    case SPELL_PROTECT_FROM_GOOD:
    case SPELL_OAKEN_FORTITUDE:
    case SPELL_FROSTSHIELD:
    case SPELL_STABILITY:
    case SPELL_KILLER:
    case SPELL_CANTQUIT:
    case SPELL_SOLIDITY:
    case SPELL_EAS:
    case SPELL_ALIGN_GOOD:
    case SPELL_ALIGN_EVIL:
    case SPELL_AEGIS:
    case SPELL_U_AEGIS:
    case SPELL_RESIST_MAGIC:
    case SPELL_EAGLE_EYE:
    case SPELL_CALL_LIGHTNING:
    case SPELL_DIVINE_FURY:
    case SPELL_GHOSTWALK:
    case SPELL_MEND_GOLEM:
    case SPELL_CLARITY:
    case SPELL_RELEASE_ELEMENTAL:
    case SPELL_DIVINE_INTER:
      scrollok = TRUE;
      break;
    default:
      scrollok = FALSE;
  }

  send_to_char("You sit down and carefully inscribe the words of the gods onto the parchment.\n\r", ch);
  act("$n sits down and carefully inscribes the words of the gods onto some parchment.", ch, 0, 0, TO_ROOM, 0);

  //do something to the paper object to make it the scroll
  

  if(!scrollok) {
    send_to_char("As you finish, nothing special seems to happen.\n\r", ch);
    act("As $e finishes, nothing special seems to happen.", ch, 0, 0, TO_ROOM, 0);
  } else if(!skill_success(ch, 0, SKILL_SCRIBE_SCROLL)) {
    send_to_char("As you finish, the letters on the newly minted scroll burst into $B$4flame$R leaving nothing but ash!", ch);
    act("As $e finishes, the letters on the newly minted scroll burst into $B$4flame$R leaving nothing but ash!", ch, 0, 0, TO_ROOM, 0);
    extract_obj(paperobj);
  } else {
    send_to_char("As you finish, the letters on the newly minted scroll $Bglow$R briefly and return to normal.\n\r", ch);
    act("As $e finishes, the letters on the newly minted scroll $Bglow$R briefly and return to normal.", ch, 0, 0, TO_ROOM, 0);
  }

  

  return eSUCCESS;
}
