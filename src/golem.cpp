/*
  New golem code. Urizen.
   Separated all this from the rest(by putting it in save.cpp,
   magic.cpp, etc) to have all the golem code in one place.
*/
#include <cstring>



#include "character.h"
#include "structs.h"
#include "spells.h"
#include "utility.h"
#include "levels.h"
#include "player.h"
#include "db.h"
#include "interp.h"
#include "returnvals.h"
#include "ki.h"
#include "fileinfo.h"
#include "mobile.h"
#include "race.h"
#include "act.h"
#include "magic.h"
#include "affect.h"
#include "utility.h"
#include "isr.h"
#include "handler.h"
#include "const.h"

// Locals
void advance_golem_level(Character *golem);

// save.cpp
int store_worn_eq(Character *ch, FILE *fpsave);
class Object *obj_store_to_char(Character *ch, FILE *fpsave, class Object *last_cont);

struct golem_data
{ // This is how a golem looks.
  char *keyword;
  char *name;
  char *short_desc;
  char *long_desc;
  char *description;
  int max_hp;
  int roll1, roll2;  // Damage maxes
  int dam, hit;      // bonus maxes
  int components[5]; // Components(vnums)
  int special_aff;   // Special affect(s). (iron)
  int special_res;   // Resists(stone).
  int ac;            // armor
  char *creation_message;
  char *shatter_message;
  char *release_message;
};

const struct golem_data golem_list[] = {
    {"iron", "iron golem enchanted", "an enchanted iron golem", "A powerfully enchanted iron golem stands here, guarding its master.\r\n", "The iron golem is bound by its master's magics.  A mindless automaton,\r\nthe iron golem is one of the most powerful forces available in \r\na wizard's arsenal.  Nearly a full 8 feet tall and weighing several\r\ntons, this behemoth of pure iron is absolutely loyal to its master and\r\nsilently follows commands without fail.\r\n", 1400, 15, 5, 25, 50, {107, 108, 109, 0, 7004}, AFF_LIGHTNINGSHIELD, 0, -100, "There is a grinding and shrieking of metal as an iron golem is slowly formed.\r\n", "Unable to sustain further damage, the iron golem falls into unrecoverable scrap.", "As the magic binding it is released, the iron golem rusts to pieces."},
    {"stone", "stone enchanted golem", "an enchanted stone golem", "A powerfully enchanted stone golem stands here, guarding its master.\r\n", "The stone golem is bound by its caster's magics.  A mindless automaton,\r\nthe stone golem is one of the sturdiest and most resilliant creatures\r\nknown in the realms.  Nearly a full 8 feet tall and weighing several\r\ntons, this mountain of rock is absolutely loyal to its master and\r\nsilently follows orders without fail.\r\n", 2000, 5, 5, 25, 50, {104, 105, 106, 0, 7003}, -1, ISR_PIERCE, -100, "There is a deep rumbling as a stone golem slowly rises from the ground.\r\n", "Unable to sustain further damage, the stone golem shatters to pieces.", "As the magic binding it is released, the golem crumbles to dust."}};

void shatter_message(Character *ch)
{
  int golemtype = !IS_AFFECTED(ch, AFF_GOLEM); // 0 or 1
  act(golem_list[golemtype].shatter_message, ch, 0, 0, TO_ROOM, 0);
}
void release_message(Character *ch)
{
  int golemtype = !IS_AFFECTED(ch, AFF_GOLEM);
  act(golem_list[golemtype].release_message, ch, 0, 0, TO_ROOM, 0);
}

void golem_gain_exp(Character *ch)
{
  //  extern int exp_table[~];
  int level = 19 + ch->getLevel();
  if (ch->getLevel() >= 20)
    return;
  if (ch->exp > exp_table[level])
  {
    ch->exp = 0;
    ch->incrementLevel();
    advance_golem_level(ch);
    ch->master->save(666);
    do_say(ch, "Errrrrhhgg...", 0);
  }
}

int verify_existing_components(Character *ch, int golemtype)
{
  int retval = 0;

  // OVERSEERS or higher don't need components
  if (ch->getLevel() >= OVERSEER)
  {
    SET_BIT(retval, eSUCCESS);
    SET_BIT(retval, eEXTRA_VALUE); // Special effect.
    return retval;
  }

  // check_components didn't suit me.
  int i;
  class Object *curr, *next_content;
  char buf[MAX_STRING_LENGTH];
  SET_BIT(retval, eSUCCESS);
  for (i = 0; i < 5; i++)
  {
    if (golem_list[golemtype].components[i] == 0)
      continue;
    bool found = false;
    for (curr = ch->carrying; curr; curr = next_content)
    {
      next_content = curr->next_content;
      int vnum = obj_index[curr->item_number].virt;
      if (vnum == golem_list[golemtype].components[i])
      {
        found = true;
        if (i == 4)
          SET_BIT(retval, eEXTRA_VALUE); // Special effect.
      }
    }
    if (!found && i != 4)
    {
      return eFAILURE;
    }
  }
  for (i = 0; i < 5; i++)
  {
    for (curr = ch->carrying; curr; curr = next_content)
    {
      next_content = curr->next_content;
      if (golem_list[golemtype].components[i] == obj_index[curr->item_number].virt)
      {
        if (number(0, 2) || !spellcraft(ch, SPELL_CREATE_GOLEM))
        {
          sprintf(buf, "%s explodes, releasing a stream of magical energies!\r\n", curr->short_description);
          ch->send(buf);
          obj_from_char(curr);
          extract_obj(curr);
          break; // Only remove ONE of the components of that type.
        }
        else
        {
          sprintf(buf, "%s glows bright red, but you manage to retain it by only extracting part of its magic.\r\n", curr->short_description);
          ch->send(buf);
          break;
        }
      }
    }
  }
  return retval;
}

void save_golem_data(Character *ch)
{
  char file[200];
  FILE *fpfile = nullptr;
  int golemtype = 0;
  if (IS_NPC(ch) || GET_CLASS(ch) != CLASS_MAGIC_USER || !ch->player->golem)
    return;
  golemtype = !IS_AFFECTED(ch->player->golem, AFF_GOLEM); // 0 or 1
  sprintf(file, "%s/%c/%s.%d", FAMILIAR_DIR, ch->getNameC()[0], ch->getNameC(), golemtype);
  if (!(fpfile = fopen(file, "w")))
  {
    logentry("Error while opening file in save_golem_data[golem.cpp].", ANGEL, LogChannels::LOG_BUG);
    return;
  }
  Character *golem = ch->player->golem; // Just to make the code below cleaner.
  uint8_t legacy_level = (uint8_t)golem->getLevel();
  fwrite(&legacy_level, 1, 1, fpfile);
  fwrite(&(golem->exp), sizeof(golem->exp), 1, fpfile);
  // Use previously defined functions after this.
  obj_to_store(golem->carrying, golem, fpfile, -1);
  store_worn_eq(golem, fpfile);
  fclose(fpfile);
}

void save_charmie_data(Character *ch)
{
  char file[200];
  FILE *fpfile = nullptr;

  if (IS_NPC(ch) || ch->followers == nullptr)
  {
    return;
  }

  for (follow_type *followers = ch->followers; followers != nullptr; followers = followers->next)
  {
    Character *follower = followers->follower;

    if (follower == nullptr || IS_PC(follower) || follower->master == nullptr || !IS_AFFECTED(follower, AFF_CHARM))
    {
      continue;
    }

    // logf(IMMORTAL, LogChannels::LOG_MISC, "Saving charmie %s for %s", follower->name, ch->getNameC());
    sprintf(file, "%s/%c/%s.%d", FOLLOWER_DIR, ch->getNameC()[0], ch->getNameC(), 0);
    if (!(fpfile = fopen(file, "w")))
    {
      logf(ANGEL, LogChannels::LOG_BUG, "Error while opening file in save_charmie_data[golem.cpp].");
      return;
    }
    obj_to_store(follower->carrying, follower, fpfile, -1);
    store_worn_eq(follower, fpfile);
    fclose(fpfile);
  }
}

void advance_golem_level(Character *golem)
{
  int golemtype = !IS_AFFECTED(golem, AFF_GOLEM); // 0 or 1
  golem->max_hit = golem->raw_hit = (golem->raw_hit + (golem_list[golemtype].max_hp / 20));
  golem->addHP(golem_list[golemtype].max_hp / 20);
  golem->hitroll += golem_list[golemtype].hit / 20;
  golem->damroll += golem_list[golemtype].dam / 20;
  golem->armor += golem_list[golemtype].ac / 20;
  golem->exp = 0;
  redo_hitpoints(golem);
}

void set_golem(Character *golem, int golemtype)
{ // Set the basics.
  GET_RACE(golem) = RACE_GOLEM;
  golem->short_desc = str_hsh(golem_list[golemtype].short_desc);
  golem->setName(golem_list[golemtype].name);
  golem->long_desc = str_hsh(golem_list[golemtype].long_desc);
  golem->description = str_hsh(golem_list[golemtype].description);
  golem->title = 0;
  golem->affected_by[0] = 0;
  golem->affected_by[1] = 0;

  if (!golemtype)
    SETBIT(golem->affected_by, AFF_GOLEM);
  SETBIT(golem->affected_by, AFF_INFRARED);
  SETBIT(golem->affected_by, AFF_STABILITY);
  SETBIT(golem->affected_by, AFF_DETECT_INVISIBLE);
  SETBIT(golem->mobdata->actflags, ACT_2ND_ATTACK);
  SETBIT(golem->mobdata->actflags, ACT_3RD_ATTACK);
  golem->misc = MISC_IS_MOB;
  golem->armor = 0;
  golem->setLevel(1);
  golem->hitroll = golem_list[golemtype].hit / 20;
  golem->damroll = golem_list[golemtype].dam / 20;
  golem->armor = golem_list[golemtype].ac / 20;
  golem->raw_hit = golem->max_hit = golem->hit = golem_list[golemtype].max_hp / 20;
  golem->mobdata->damnodice = golem_list[golemtype].roll1;
  golem->mobdata->damsizedice = golem_list[golemtype].roll2;
  golem->setGold(0);
  golem->plat = 0;
  golem->max_move = golem->mana = golem->max_mana = 100;
  golem->setMove(golem->max_move);
  golem->mobdata->last_room = 0;
  golem->setStanding();
  golem->immune = golem->suscept = golem->resist = 0;
  golem->c_class = 0;
  golem->height = 255; // Was 350, but it can't fit in a byte
  golem->weight = 255; // Was 530, ditto
}

void load_golem_data(Character *ch, int golemtype)
{
  char file[200];
  FILE *fpfile = nullptr;
  Character *golem;
  if (IS_NPC(ch) || (GET_CLASS(ch) != CLASS_MAGIC_USER && ch->getLevel() < OVERSEER) || ch->player->golem)
    return;
  if (golemtype < 0 || golemtype > 1)
    return; // Say what?
  sprintf(file, "%s/%c/%s.%d", FAMILIAR_DIR, ch->getNameC()[0], ch->getNameC(), golemtype);
  if (!(fpfile = fopen(file, "r")))
  { // No golem. Create a new one.
    golem = clone_mobile(real_mobile(8));
    set_golem(golem, golemtype);
    golem->alignment = ch->alignment;
    ch->player->golem = golem;
    return;
  }
  golem = clone_mobile(real_mobile(8));
  set_golem(golem, golemtype); // Basics
  ch->player->golem = golem;
  uint8_t golem_level{};
  fread(&(golem_level), sizeof(golem_level), 1, fpfile);
  golem->setLevel(golem_level);

  for (; golem_level > 1; golem_level--)
  {
    advance_golem_level(golem); // Level it up again.
  }

  fread(&(golem->exp), sizeof(golem->exp), 1, fpfile);
  class Object *last_cont = nullptr; // Last container.
  while (!feof(fpfile))
  {
    last_cont = obj_store_to_char(golem, fpfile, last_cont);
  }
  fclose(fpfile);
}

int cast_create_golem(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  Character *golem;
  int i;
  char buf[MAX_INPUT_LENGTH];
  arg = one_argument(arg, buf);
  if (IS_NPC(ch))
    return eFAILURE;
  if (ch->player->golem)
  {
    ch->sendln("You already have a golem.");
    return eFAILURE;
  }
  for (i = 0; i < MAX_GOLEMS; i++)
  {
    if (!str_cmp(golem_list[i].keyword, buf))
      break;
  }
  if (i >= MAX_GOLEMS)
  {
    ch->sendln("You cannot create any such golem.");
    return eFAILURE;
  }
  int retval = verify_existing_components(ch, i);
  if (isSet(retval, eFAILURE))
  {
    ch->sendln("Since you do not have the required spell components, the magic fades into nothingness.");
    return eFAILURE;
  }
  load_golem_data(ch, i); // Load the golem up;
  ch->skill_increase_check(SPELL_CREATE_GOLEM, skill, SKILL_INCREASE_EASY);
  golem = ch->player->golem;
  if (!golem)
  { // Returns false if something goes wrong. (Not a mage, etc).
    ch->send("Something goes wrong, and you fail!");
    return eFAILURE;
  }
  if (isSet(retval, eEXTRA_VALUE))
  {
    ch->sendln("Adding in the final ingredient, your golem increases in strength!");
    SETBIT(golem->affected_by, golem_list[i].special_aff);
    SET_BIT(golem->resist, golem_list[i].special_res);
  }
  char_to_room(golem, ch->in_room);
  add_follower(golem, ch, 0);
  SETBIT(golem->affected_by, AFF_CHARM);
  //  struct affected_type af;
  send_to_char(golem_list[i].creation_message, ch);
  return eSUCCESS;
}

extern char frills[];

int do_golem_score(Character *ch, char *argument, int cmd)
{ /* Pretty much a rip of score */
  char race[100];
  char buf[MAX_STRING_LENGTH], scratch;
  int level = 0;
  int to_dam, to_hit;
  Character *master = ch;
  if (IS_NPC(ch))
    return eFAILURE;
  if (!ch->player->golem)
  {
    ch->sendln("But you don't have a golem!");
    return eFAILURE;
  }
  ch = ch->player->golem;
  struct affected_type *aff;

  int64_t exp_needed;

  uint32_t immune = 0, suscept = 0, resist = 0;
  std::string isrString;

  sprintf(race, "%s", races[(int)GET_RACE(ch)].singular_name);
  if (ch->getLevel() + 19 > 60)
  {
    logentry(QString("do_golem_score: bug with %1's golem. It has level %2 which + 19 is %3 > 60.").arg(GET_NAME(master)).arg(ch->getLevel()).arg(ch->getLevel() + 19));
    master->send("There is an error with your golem. Contact an immortal.\r\n");
    produce_coredump(ch);
    return eSUCCESS;
  }
  exp_needed = (int)(exp_table[(int)ch->getLevel() + 19] - (int64_t)GET_EXP(ch));

  to_hit = GET_REAL_HITROLL(ch);
  to_dam = GET_REAL_DAMROLL(ch);

  sprintf(buf,
          "$7($5:$7)================================================="
          "========================($5:$7)\n\r"
          "|=| %-30s  -- Character Attributes (DarkCastleMUD) |=|\n\r"
          "($5:$7)=============================($5:$7)================="
          "========================($5:$7)\n\r",
          GET_SHORT(ch));

  master->send(buf);

  sprintf(buf,
          "|\\| $4Strength$7:        %4d  (%2d) |/| $1Race$7:  %-10s  $1HitPts$7:%5d$1/$7(%5d) |~|\n\r"
          "|~| $4Dexterity$7:       %4d  (%2d) |o| $1Class$7: %-11s $1Mana$7:   %4d$1/$7(%5d) |\\|\n\r"
          "|/| $4Constitution$7:    %4d  (%2d) |\\| $1Level$7:  %-6d     $1Fatigue$7:%4d$1/$7(%5d) |o|\n\r"
          "|o| $4Intelligence$7:    %4d  (%2d) |~| $1Height$7: %3d        $1Ki$7:     %4d$1/$7(%5d) |/|\n\r"
          "|\\| $4Wisdom$7:          %4d  (%2d) |/| $1Weight$7: %3d                             |~|\n\r"
          "|~| $3Rgn$7: $4H$7:%3d $4M$7:%3d $4V$7:%3d $4K$7:%2d |o| $1Age$7:    %3d yrs    $1Align$7: %+5d         |\\|\n\r",
          GET_STR(ch), GET_RAW_STR(ch), race, ch->getHP(), GET_MAX_HIT(ch),
          GET_DEX(ch), GET_RAW_DEX(ch), pc_clss_types[(int)GET_CLASS(ch)], GET_MANA(ch), GET_MAX_MANA(ch),
          GET_CON(ch), GET_RAW_CON(ch), ch->getLevel(), GET_MOVE(ch), GET_MAX_MOVE(ch),
          GET_INT(ch), GET_RAW_INT(ch), GET_HEIGHT(ch), GET_KI(ch), GET_MAX_KI(ch),
          GET_WIS(ch), GET_RAW_WIS(ch), GET_WEIGHT(ch), ch->hit_gain_lookup(),
          ch->mana_gain_lookup(), ch->move_gain_lookup(), ch->ki_gain_lookup(), GET_AGE(ch),
          GET_ALIGNMENT(ch));

  master->send(buf);

  sprintf(buf,
          "($5:$7)=============================($5:$7)===($5:$7)===================================($5:$7)\n\r"
          "|/| $2Combat Statistics:$7                |\\| $2Equipment and Valuables:$7          |o|\n\r"
          "|o|  $3Armor$7:   %5d   $3Pkills$7:  %5d  |~|  $3Items Carried$7:  %-3d/(%-3d)        |/|\n\r"
          "|\\|  $3BonusHit$7: %+4d   $3PDeaths$7: %5d  |/|  $3Weight Carried$7: %-3d/(%-4d)       |~|\n\r"
          "|~|  $3BonusDam$7: %+4d   $3RDeaths$7: %5d  |o|  $3Experience$7:     %-10ld       |\\|\n\r"
          "|/|  $B$4FIRE$R[%+3d]  $B$3COLD$R[%+3d]  $B$5NRGY$R[%+3d]  |\\|  $3ExpTillLevel$7:   %-10ld       |o|\n\r"
          "|o|  $B$2ACID$R[%+3d]  $B$7MAGK$R[%+3d]  $2POIS$7[%+3d]  |~|  $3Gold$7: %-10lu $3Platinum$7: %-5d |/|\n\r"
          "|\\|  $3MELE$R[%+3d]  $3SPEL$R[%+3d]   $3KI$R [%+3d]  |/|  $3Bank$7: %-10d                 |-|\n\r"
          "($5:$7)===================================($5:$7)===================================($5:$7)\n\r",
          GET_ARMOR(ch), 0, IS_CARRYING_N(ch), CAN_CARRY_N(ch),
          to_hit, 0, IS_CARRYING_W(ch), CAN_CARRY_W(ch),
          to_dam, 0, GET_EXP(ch),
          get_saves(ch, SAVE_TYPE_FIRE), get_saves(ch, SAVE_TYPE_COLD), get_saves(ch, SAVE_TYPE_ENERGY), ch->getLevel() == 50 ? 0 : exp_needed,
          get_saves(ch, SAVE_TYPE_ACID), get_saves(ch, SAVE_TYPE_MAGIC), get_saves(ch, SAVE_TYPE_POISON), (int)ch->getGold(), (int)GET_PLATINUM(ch),
          ch->melee_mitigation, ch->spell_mitigation, ch->song_mitigation, 0);
  master->send(buf);

  if ((immune = ch->immune))
  {
    for (int i = 0; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(immune, i);
      if (!isrString.empty())
      {
        scratch = frills[level];
        sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
                scratch, "Immunity", isrString.c_str(), scratch);
        master->send(buf);
        isrString = std::string();
        if (++level == 4)
          level = 0;
      }
    }
  }
  if ((suscept = ch->suscept))
  {
    for (int i = 0; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(suscept, i);
      if (!isrString.empty())
      {
        scratch = frills[level];
        sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
                scratch, "Susceptibility", isrString.c_str(), scratch);
        master->send(buf);
        isrString = std::string();
        if (++level == 4)
          level = 0;
      }
    }
  }
  if ((resist = ch->resist))
  {
    for (int i = 0; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(resist, i);
      if (!isrString.empty())
      {
        scratch = frills[level];
        sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
                scratch, "Resistibility", isrString.c_str(), scratch);
        master->send(buf);
        isrString = std::string();
        if (++level == 4)
          level = 0;
      }
    }
  }

  sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
          frills[level], "STABILITY", "NONE", frills[level]);
  master->send(buf);
  ++level == 4 ? level = 0 : level;
  sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
          frills[level], "INFRARED", "NONE", frills[level]);
  master->send(buf);
  ++level == 4 ? level = 0 : level;
  if (ISSET(ch->affected_by, AFF_LIGHTNINGSHIELD))
  {
    sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
            frills[level], "LIGHTNING SHIELD", "NONE", frills[level]);
    master->send(buf);
    ++level == 4 ? level = 0 : level;
  }

  if ((aff = ch->affected))
  {
    for (; aff; aff = aff->next)
    {
      scratch = frills[level];
      // figure out the name of the affect (if any)
      const char *aff_name = get_skill_name(aff->type);
      switch (aff->type)
      {
      case Character::PLAYER_CANTQUIT:
        aff_name = "Can't Quit";
        break;
      case Character::PLAYER_OBJECT_THIEF:
        aff_name = "DIRTY_DIRTY_THIEF";
        break;
      case SKILL_HARM_TOUCH:
        aff_name = "harmtouch reuse timer";
        break;
      case SKILL_LAY_HANDS:
        aff_name = "layhands reuse timer";
        break;
      case SKILL_QUIVERING_PALM:
        aff_name = "quiver reuse timer";
        break;
      case SKILL_BLOOD_FURY:
        aff_name = "blood fury reuse timer";
        break;
      case SKILL_CRAZED_ASSAULT:
        if (strcmp(apply_types[(int)aff->location], "HITROLL"))
          aff_name = "crazed assault reuse timer";
        break;
      case SPELL_HOLY_AURA_TIMER:
        aff_name = "holy aura timer";
        break;
      default:
        break;
      }
      if (!aff_name) // not one we want displayed
        continue;

      if (aff->type == Character::PLAYER_CANTQUIT)
      {
        sprintf(buf, "|%c| Affected by %-25s (%s) |%c|\n\r",
                scratch, aff_name,
                ((IS_AFFECTED(ch, AFF_DETECT_MAGIC) && aff->duration < 3) ? "$2(fading)$7" : "        "),
                apply_types[(int)aff->location], aff->caster.c_str());
      }
      else
      {
        sprintf(buf, "|%c| Affected by %-25s %s Modifier %-13s   |%c|\n\r",
                scratch, aff_name,
                ((IS_AFFECTED(ch, AFF_DETECT_MAGIC) && aff->duration < 3) ? "$2(fading)$7" : "        "),
                apply_types[(int)aff->location], scratch);
      }
      master->send(buf);
      if (++level == 4)
        level = 0;
    }
  }

  master->sendln("($5:$7)=========================================================================($5:$7)");

  return eSUCCESS;
}

int spell_release_golem(uint8_t level, Character *ch, char *arg, int type, Character *tar_ch, class Object *tar_obj, int skill)
{
  struct follow_type *fol;
  for (fol = ch->followers; fol; fol = fol->next)
    if (IS_NPC(fol->follower) && mob_index[fol->follower->mobdata->nr].virt == 8)
    {
      release_message(fol->follower);
      extract_char(fol->follower, false);
      return eSUCCESS;
    }
  ch->sendln("You don't have a golem.");
  return eSUCCESS;
}
