/*
  New golem code. Urizen.
   Separated all this from the rest(by putting it in save.cpp,
   magic.cpp, etc) to have all the golem code in one place.
*/
#include "DC/DC.h"
// Locals
void advance_golem_level(CharacterPtr golem);

// save.cpp
qint32 store_worn_eq(CharacterPtr ch, FILE *fpsave);
ObjectPtr obj_store_to_char(CharacterPtr ch, FILE *fpsave, ObjectPtr last_cont);

class golem_data
{ // This is how a golem looks.
public:
  const QString keyword;
  const QString name;
  const QString short_desc;
  const QString long_desc;
  const QString description;
  qint32 max_hp;
  qint32 roll1, roll2;  // Damage maxes
  qint32 dam, hit;      // bonus maxes
  qint32 components[5]; // Components(vnums)
  qint32 special_aff;   // Special affect(s). (iron)
  qint32 special_res;   // Resists(stone).
  qint32 ac;            // armor
  const QString creation_message;
  const QString shatter_message;
  const QString release_message;
};

const golem_data golem_list[] = {
    {"iron", "iron golem enchanted", "an enchanted iron golem", "A powerfully enchanted iron golem stands here, guarding its master.\r\n", "The iron golem is bound by its master's magics.  A mindless automaton,\r\nthe iron golem is one of the most powerful forces available in \r\na wizard's arsenal.  Nearly a full 8 feet tall and weighing several\r\ntons, this behemoth of pure iron is absolutely loyal to its master and\r\nsilently follows commands without fail.\r\n", 1400, 15, 5, 25, 50, {107, 108, 109, 0, 7004}, AFF_LIGHTNINGSHIELD, 0, -100, "There is a grinding and shrieking of metal as an iron golem is slowly formed.\r\n", "Unable to sustain further damage, the iron golem falls into unrecoverable scrap.", "As the magic binding it is released, the iron golem rusts to pieces."},
    {"stone", "stone enchanted golem", "an enchanted stone golem", "A powerfully enchanted stone golem stands here, guarding its master.\r\n", "The stone golem is bound by its caster's magics.  A mindless automaton,\r\nthe stone golem is one of the sturdiest and most resilliant creatures\r\nknown in the realms.  Nearly a full 8 feet tall and weighing several\r\ntons, this mountain of rock is absolutely loyal to its master and\r\nsilently follows orders without fail.\r\n", 2000, 5, 5, 25, 50, {104, 105, 106, 0, 7003}, -1, ISR_PIERCE, -100, "There is a deep rumbling as a stone golem slowly rises from the ground.\r\n", "Unable to sustain further damage, the stone golem shatters to pieces.", "As the magic binding it is released, the golem crumbles to dust."}};

void shatter_message(CharacterPtr ch)
{
  qint32 golemtype = !IS_AFFECTED(ch, AFF_GOLEM); // 0 or 1
  act_to_room(golem_list[golemtype].shatter_message, ch, 0, 0, 0);
}
void release_message(CharacterPtr ch)
{
  qint32 golemtype = !IS_AFFECTED(ch, AFF_GOLEM);
  act_to_room(golem_list[golemtype].release_message, ch, 0, 0, 0);
}

void golem_gain_exp(CharacterPtr ch)
{
  //  extern qint32 exp_table[~];
  qint32 level = 19 + ch->getLevel();
  if (ch->getLevel() >= 20)
    return;
  if (ch->exp > exp_table[level])
  {
    ch->exp = {};
    ch->incrementLevel();
    advance_golem_level(ch);
    ch->master->save(cmd_t::SAVE_SILENTLY);
    do_say(ch, "Errrrrhhgg...");
  }
}

qint32 verify_existing_components(CharacterPtr ch, qint32 golemtype)
{
  qint32 retval = {};

  // OVERSEERS or higher don't need components
  if (ch->getLevel() >= OVERSEER)
  {
    SET_BIT(retval, ReturnValue::eSUCCESS);
    SET_BIT(retval, ReturnValue::eEXTRA_VALUE); // Special effect.
    return retval;
  }

  // check_components didn't suit me.
  qint32 i;
  ObjectPtr curr, next_content;
  QString buf;
  SET_BIT(retval, ReturnValue::eSUCCESS);
  for (i = {}; i < 5; i++)
  {
    if (golem_list[golemtype].components[i] == 0)
      continue;
    bool found = false;
    for (curr = ch->carrying; curr; curr = next_content)
    {
      next_content = curr->next_content;
      qint32 vnum = dc_->obj_index[curr->item_number].vnum();
      if (vnum == golem_list[golemtype].components[i])
      {
        found = true;
        if (i == 4)
          SET_BIT(retval, ReturnValue::eEXTRA_VALUE); // Special effect.
      }
    }
    if (!found && i != 4)
    {
      return ReturnValue::eFAILURE;
    }
  }
  for (i = {}; i < 5; i++)
  {
    for (curr = ch->carrying; curr; curr = next_content)
    {
      next_content = curr->next_content;
      if (golem_list[golemtype].components[i] == dc_->obj_index[curr->item_number].vnum())
      {
        if (ch->dc_->number(0, 2) || !spellcraft(ch, SPELL_CREATE_GOLEM))
        {
          dc_sprintf(buf, "%s explodes, releasing a stream of magical energies!\r\n", qPrintable(curr->short_description()));
          ch->send(buf);
          obj_from_char(curr);
          extract_obj(curr);
          break; // Only remove ONE of the components of that type.
        }
        else
        {
          dc_sprintf(buf, "%s glows bright red, but you manage to retain it by only extracting part of its magic.\r\n", qPrintable(curr->short_description()));
          ch->send(buf);
          break;
        }
      }
    }
  }
  return retval;
}

void save_golem_data(CharacterPtr ch)
{
  QString file;
  FILE *fpfile = {};
  qint32 golemtype = {};
  if (ch->isNonPlayer() || GET_CLASS(ch) != CLASS_MAGIC_USER || !ch->player->golem)
    return;
  golemtype = !IS_AFFECTED(ch->player->golem, AFF_GOLEM); // 0 or 1
  dc_sprintf(file, "%s/%c/%s.%d", FAMILIAR_DIR, qPrintable(ch->name())[0], qPrintable(ch->name()), golemtype);
  if (!(fpfile = fopen(file, "w")))
  {
    dc_->logentry(u"Error while opening file in save_golem_data[golem.cpp]."_s, ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }
  CharacterPtr golem = ch->player->golem; // Just to make the code below cleaner.
  quint8 legacy_level = (quint8)golem->getLevel();
  fwrite(&legacy_level, 1, 1, fpfile);
  fwrite(&(golem->exp), sizeof(golem->exp), 1, fpfile);
  // Use previously defined functions after this.
  obj_to_store(golem->carrying, golem, fpfile, -1);
  store_worn_eq(golem, fpfile);
}

void save_charmie_data(CharacterPtr ch)
{
  QString file;
  FILE *fpfile = {};

  if (ch->isNonPlayer() || ch->followers == nullptr)
  {
    return;
  }

  for (follow_type *followers = ch->followers; followers != nullptr; followers = followers->next)
  {
    CharacterPtr follower = followers->follower;

    if (follower == nullptr || follower->isPlayer() || follower->master == nullptr || !IS_AFFECTED(follower, AFF_CHARM))
    {
      continue;
    }

    // dc_->logf(IMMORTAL, DC::LogChannel::LOG_MISC, "Saving charmie %s for %s", follower->name, qPrintable(ch->name()));
    dc_sprintf(file, "%s/%c/%s.%d", FOLLOWER_DIR, qPrintable(ch->name())[0], qPrintable(ch->name()), 0);
    if (!(fpfile = fopen(file, "w")))
    {
      dc_->logf(ANGEL, DC::LogChannel::LOG_BUG, "Error while opening file in save_charmie_data[golem.cpp].");
      return;
    }
    obj_to_store(follower->carrying, follower, fpfile, -1);
    store_worn_eq(follower, fpfile);
  }
}

void advance_golem_level(CharacterPtr golem)
{
  qint32 golemtype = !IS_AFFECTED(golem, AFF_GOLEM); // 0 or 1
  golem->max_hit = golem->raw_hit = (golem->raw_hit + (golem_list[golemtype].max_hp / 20));
  golem->addHP(golem_list[golemtype].max_hp / 20);
  golem->hitroll += golem_list[golemtype].hit / 20;
  golem->damroll += golem_list[golemtype].dam / 20;
  golem->armor += golem_list[golemtype].ac / 20;
  golem->exp = {};
  redo_hitpoints(golem);
}

void set_golem(CharacterPtr golem, qint32 golemtype)
{ // Set the basics.
  golem->race = RACE_GOLEM;
  golem->short_description(golem_list[golemtype].short_desc);
  golem->name(golem_list[golemtype].name);
  golem->long_description(golem_list[golemtype].long_desc);
  golem->description(golem_list[golemtype].description);
  golem->title = {};
  golem->affected_by[0] = {};
  golem->affected_by[1] = {};

  if (!golemtype)
    SETBIT(golem->affected_by, AFF_GOLEM);
  SETBIT(golem->affected_by, AFF_INFRARED);
  SETBIT(golem->affected_by, AFF_STABILITY);
  SETBIT(golem->affected_by, AFF_DETECT_INVISIBLE);
  SETBIT(golem->mobdata->actflags, ACT_2ND_ATTACK);
  SETBIT(golem->mobdata->actflags, ACT_3RD_ATTACK);
  golem->setType(Character::Type::NPC);
  golem->misc = {};
  golem->armor = {};
  golem->setLevel(1);
  golem->hitroll = golem_list[golemtype].hit / 20;
  golem->damroll = golem_list[golemtype].dam / 20;
  golem->armor = golem_list[golemtype].ac / 20;
  golem->raw_hit = golem->max_hit = golem->hit = golem_list[golemtype].max_hp / 20;
  golem->mobdata->damnodice = golem_list[golemtype].roll1;
  golem->mobdata->damsizedice = golem_list[golemtype].roll2;
  golem->setGold(0);
  golem->plat = {};
  golem->max_move = golem->mana = golem->max_mana = 100;
  golem->setMove(golem->max_move);
  golem->mobdata->last_room = {};
  golem->setStanding();
  golem->immune = golem->suscept = golem->resist = {};
  golem->c_class = {};
  golem->height = 255; // Was 350, but it can't fit in a byte
  golem->weight = 255; // Was 530, ditto
}

void Character::load_golem_data(qint32 golemtype)
{
  QString file;
  FILE *fpfile = {};
  CharacterPtr golem;
  if (isNonPlayer() || (GET_CLASS(this) != CLASS_MAGIC_USER && getLevel() < OVERSEER) || player->golem)
    return;
  if (golemtype < 0 || golemtype > 1)
    return; // Say what?
  dc_sprintf(file, "%s/%c/%s.%d", FAMILIAR_DIR, qPrintable(name())[0], qPrintable(name()), golemtype);
  if (!(fpfile = fopen(file, "r")))
  { // No golem. Create a new one.
    golem = dc_->clone_mobile(real_mobile(8));
    set_golem(golem, golemtype);
    golem->alignment = alignment;
    player->golem = golem;
    return;
  }
  golem = dc_->clone_mobile(real_mobile(8));
  set_golem(golem, golemtype); // Basics
  player->golem = golem;
  quint8 golem_level = {};
  fread(&(golem_level), sizeof(golem_level), 1, fpfile);
  golem->setLevel(golem_level);

  for (; golem_level > 1; golem_level--)
  {
    advance_golem_level(golem); // Level it up again.
  }

  fread(&(golem->exp), sizeof(golem->exp), 1, fpfile);
  ObjectPtr last_cont = {}; // Last container.
  while (!feof(fpfile))
  {
    last_cont = obj_store_to_char(golem, fpfile, last_cont);
  }
}

qint32 cast_create_golem(quint8 level, CharacterPtr ch, QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill)
{
  CharacterPtr golem;
  qint32 i;
  QString buf;
  arg = one_argument(arg, buf);
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;
  if (ch->player->golem)
  {
    ch->sendln("You already have a golem.");
    return ReturnValue::eFAILURE;
  }
  for (i = {}; i < MAX_GOLEMS; i++)
  {
    if (!str_cmp(golem_list[i].keyword, buf))
      break;
  }
  if (i >= MAX_GOLEMS)
  {
    ch->sendln("You cannot create any such golem.");
    return ReturnValue::eFAILURE;
  }
  qint32 retval = verify_existing_components(ch, i);
  if (isSet(retval, ReturnValue::eFAILURE))
  {
    ch->sendln("Since you do not have the required spell components, the magic fades into nothingness.");
    return ReturnValue::eFAILURE;
  }
  ch->load_golem_data(i); // Load the golem up;
  ch->skill_increase_check(SPELL_CREATE_GOLEM, skill, SKILL_INCREASE_EASY);
  golem = ch->player->golem;
  if (!golem)
  { // Returns false if something goes wrong. (Not a mage, etc).
    ch->send("Something goes wrong, and you fail!");
    return ReturnValue::eFAILURE;
  }
  if (isSet(retval, ReturnValue::eEXTRA_VALUE))
  {
    ch->sendln("Adding in the final ingredient, your golem increases in strength!");
    SETBIT(golem->affected_by, golem_list[i].special_aff);
    SET_BIT(golem->resist, golem_list[i].special_res);
  }
  char_to_room(golem, ch->in_room);
  add_follower(golem, ch);
  SETBIT(golem->affected_by, AFF_CHARM);
  //  affected_type af;
  send_to_char(golem_list[i].creation_message, ch);
  return ReturnValue::eSUCCESS;
}

extern QString frills;

command_return_t do_golem_score(CharacterPtr ch, QString argument, cmd_t cmd)
{ /* Pretty much a rip of score */
  QString race;
  QString buf, scratch;
  qint32 level = {};
  qint32 to_dam, to_hit;
  CharacterPtr master = ch;
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (cmd == cmd_t::GOLEMSCORE)
  {
    if (!ch->player->golem)
    {
      ch->sendln("But you don't have a golem!");
      return ReturnValue::eFAILURE;
    }
    else
    {
      ch = ch->player->golem;
    }
  }
  else if (cmd == cmd_t::FSCORE)
  {
    follow_type *folnext;
    quint8 charmies_found = {};
    for (auto fol = ch->followers; fol; fol = folnext)
    {
      folnext = fol->next;
      if (IS_AFFECTED(fol->follower, AFF_CHARM))
        charmies_found++;
    }

    if (charmies_found > 1)
    {
      if (QString(argument).isEmpty())
      {
        ch->sendln(u"Specify which non-player follower you want to fscore."_s);
        return ReturnValue::eFAILURE;
      }
      else
      {
        auto vict = get_mob_room_vis(ch, argument);
        if (!vict)
        {
          ch->sendln("No mob by that name here.");
          return ReturnValue::eFAILURE;
        }
        if (!IS_AFFECTED(vict, AFF_CHARM))
        {
          ch->sendln(u"%1 is not a charmie."_s.arg(qPrintable(vict->shortdesc_or_name())));
          return ReturnValue::eFAILURE;
        }
        if (vict->master != ch)
        {
          ch->sendln(u"%1 is not your charmie."_s.arg(qPrintable(vict->shortdesc_or_name())));
          return ReturnValue::eFAILURE;
        }
        ch = vict;
      }
    }
    else
    {
      for (auto fol = ch->followers; fol; fol = folnext)
      {
        folnext = fol->next;
        if (IS_AFFECTED(fol->follower, AFF_CHARM))
        {
          ch = fol->follower;
          break;
        }
      }
    }

    if (ch == master)
    {
      ch->sendln("But you don't have any non-player followers!");
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    dc_->logentry(u"unexpected cmd set to %1 sent to do_golem_score"_s.arg(QString::number(static_cast<quint64>(cmd))));
    return ReturnValue::eFAILURE;
  }

  affected_typePtr aff;

  qint64 exp_needed;

  quint32 immune = 0, suscept = 0, resist = {};
  QString isrString;

  dc_sprintf(race, "%s", races[(qint32)ch->race].singular_name);
  if (cmd == cmd_t::GOLEMSCORE && ch->getLevel() + 19 > 60)
  {
    dc_->logentry(u"do_golem_score: bug with %1's golem. It has level %2 which + 19 is %3 > 60."_s.arg(qPrintable(master->name())).arg(ch->getLevel()).arg(ch->getLevel() + 19));
    master->send("There is an error with your golem. Contact an immortal.\r\n");
    produce_coredump(ch);
    return ReturnValue::eSUCCESS;
  }
  exp_needed = (qint32)(exp_table[(qint32)ch->getLevel() + 19] - (qint64)ch->exp);

  to_hit = GET_REAL_HITROLL(ch);
  to_dam = GET_REAL_DAMROLL(ch);

  dc_sprintf(buf, "$7($5:$7)================================================="
                  "========================($5:$7)\r\n"
                  "|=| %-30s  -- Character Attributes (DarkCastleMUD) |=|\r\n"
                  "($5:$7)=============================($5:$7)================="
                  "========================($5:$7)\r\n",
             qPrintable(ch->shortdesc_or_name()));

  master->send(buf);

  dc_sprintf(buf, "|\\| $4Strength$7:        %4d  (%2d) |/| $1Race$7:  %-10s  $1HitPts$7:%5d$1/$7(%5d) |~|\r\n"
                  "|~| $4Dexterity$7:       %4d  (%2d) |o| $1Class$7: %-11s $1Mana$7:   %4d$1/$7(%5d) |\\|\r\n"
                  "|/| $4Constitution$7:    %4d  (%2d) |\\| $1Level$7:  %-6llu     $1Fatigue$7:%4d$1/$7(%5d) |o|\r\n"
                  "|o| $4Intelligence$7:    %4d  (%2d) |~| $1Height$7: %3d        $1Ki$7:     %4d$1/$7(%5d) |/|\r\n"
                  "|\\| $4Wisdom$7:          %4d  (%2d) |/| $1Weight$7: %3d                             |~|\r\n"
                  "|~| $3Rgn$7: $4H$7:%3d $4M$7:%3d $4V$7:%3d $4K$7:%2d |o| $1Age$7:    %3d yrs    $1Align$7: %+5d         |\\|\r\n",
             GET_STR(ch), GET_RAW_STR(ch), race, ch->getHP(), GET_MAX_HIT(ch),
             GET_DEX(ch), GET_RAW_DEX(ch), pc_clss_types[(qint32)GET_CLASS(ch)], GET_MANA(ch), GET_MAX_MANA(ch),
             GET_CON(ch), GET_RAW_CON(ch), ch->getLevel(), GET_MOVE(ch), GET_MAX_MOVE(ch),
             GET_INT(ch), GET_RAW_INT(ch), GET_HEIGHT(ch), GET_KI(ch), GET_MAX_KI(ch),
             GET_WIS(ch), GET_RAW_WIS(ch), GET_WEIGHT(ch), ch->hit_gain_lookup(),
             ch->mana_gain_lookup(), ch->move_gain_lookup(), ch->ki_gain_lookup(), GET_AGE(ch),
             GET_ALIGNMENT(ch));

  master->send(buf);

  dc_sprintf(buf, "($5:$7)=============================($5:$7)===($5:$7)===================================($5:$7)\r\n"
                  "|/| $2Combat Statistics:$7                |\\| $2Equipment and Valuables:$7          |o|\r\n"
                  "|o|  $3Armor$7:   %5d   $3Pkills$7:  %5d  |~|  $3Items Carried$7:  %-3d/(%-3d)        |/|\r\n"
                  "|\\|  $3BonusHit$7: %+4d   $3PDeaths$7: %5d  |/|  $3Weight Carried$7: %-3d/(%-4d)       |~|\r\n"
                  "|~|  $3BonusDam$7: %+4d   $3RDeaths$7: %5d  |o|  $3Experience$7:     %-10ld       |\\|\r\n"
                  "|/|  $B$4FIRE$R[%+3d]  $B$3COLD$R[%+3d]  $B$5NRGY$R[%+3d]  |\\|  $3ExpTillLevel$7:   %-10ld       |o|\r\n"
                  "|o|  $B$2ACID$R[%+3d]  $B$7MAGK$R[%+3d]  $2POIS$7[%+3d]  |~|  $3Gold$7: %-10lu $3Platinum$7: %-5d |/|\r\n"
                  "|\\|  $3MELE$R[%+3d]  $3SPEL$R[%+3d]   $3KI$R [%+3d]  |/|  $3Bank$7: %-10d                 |-|\r\n"
                  "($5:$7)===================================($5:$7)===================================($5:$7)\r\n",
             GET_ARMOR(ch), 0, IS_CARRYING_N(ch), CAN_CARRY_N(ch),
             to_hit, 0, IS_CARRYING_W(ch), CAN_CARRY_W(ch),
             to_dam, 0, ch->exp,
             get_saves(ch, SAVE_TYPE_FIRE), get_saves(ch, SAVE_TYPE_COLD), get_saves(ch, SAVE_TYPE_ENERGY), ch->getLevel() == 50 ? 0 : exp_needed,
             get_saves(ch, SAVE_TYPE_ACID), get_saves(ch, SAVE_TYPE_MAGIC), get_saves(ch, SAVE_TYPE_POISON), ch->getGold(), (qint32)GET_PLATINUM(ch),
             ch->melee_mitigation, ch->spell_mitigation, ch->song_mitigation, 0);
  master->send(buf);

  if ((immune = ch->immune))
  {
    for (qint32 i = {}; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(immune, i);
      if (!isrString.isEmpty())
      {
        scratch = frills[level];
        dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
                   scratch, "Immunity", isrString.c_str(), scratch);
        master->send(buf);
        isrString = QString();
        if (++level == 4)
          level = {};
      }
    }
  }
  if ((suscept = ch->suscept))
  {
    for (qint32 i = {}; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(suscept, i);
      if (!isrString.isEmpty())
      {
        scratch = frills[level];
        dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
                   scratch, "Susceptibility", isrString.c_str(), scratch);
        master->send(buf);
        isrString = QString();
        if (++level == 4)
          level = {};
      }
    }
  }
  if ((resist = ch->resist))
  {
    for (qint32 i = {}; i <= ISR_MAX; i++)
    {
      isrString = get_isr_string(resist, i);
      if (!isrString.isEmpty())
      {
        scratch = frills[level];
        dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
                   scratch, "Resistibility", isrString.c_str(), scratch);
        master->send(buf);
        isrString = QString();
        if (++level == 4)
          level = {};
      }
    }
  }

  dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
             frills[level], "STABILITY", "NONE", frills[level]);
  master->send(buf);
  ++level == 4 ? level = 0 : level;
  dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
             frills[level], "INFRARED", "NONE", frills[level]);
  master->send(buf);
  ++level == 4 ? level = 0 : level;
  if (ISSET(ch->affected_by, AFF_LIGHTNINGSHIELD))
  {
    dc_sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\r\n",
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
      QString aff_name = get_skill_name(aff->type);
      switch (aff->type)
      {
      case Character::PLAYER_CANTQUIT:
        aff_name = u"Can't Quit"_s;
        break;
      case Character::PLAYER_OBJECT_THIEF:
        aff_name = u"DIRTY_DIRTY_THIEF"_s;
        break;
      case SKILL_HARM_TOUCH:
        aff_name = u"harmtouch reuse timer"_s;
        break;
      case SKILL_LAY_HANDS:
        aff_name = u"layhands reuse timer"_s;
        break;
      case SKILL_QUIVERING_PALM:
        aff_name = u"quiver reuse timer"_s;
        break;
      case SKILL_BLOOD_FURY:
        aff_name = u"blood fury reuse timer"_s;
        break;
      case SKILL_CRAZED_ASSAULT:
        if (dc_strcmp(apply_types[(qint32)aff->location], "HITROLL"))
          aff_name = u"crazed assault reuse timer"_s;
        break;
      case SPELL_HOLY_AURA_TIMER:
        aff_name = u"holy aura timer"_s;
        break;
      default:
        break;
      }
      if (aff_name.isEmpty()) // not one we want displayed
        continue;

      if (aff->type == Character::PLAYER_CANTQUIT)
      {
        dc_sprintf(buf, "|%c| Affected by %-25s (%s) |%s%s|\r\n",
                   scratch, qPrintable(aff_name),
                   ((IS_AFFECTED(ch, AFF_DETECT_MAGIC) && aff->duration < 3) ? "$2(fading)$7" : "        "),
                   apply_types[(qint32)aff->location], aff->caster.c_str());
      }
      else
      {
        dc_sprintf(buf, "|%c| Affected by %-25s %s Modifier %-13s   |%c|\r\n",
                   scratch, qPrintable(aff_name),
                   ((IS_AFFECTED(ch, AFF_DETECT_MAGIC) && aff->duration < 3) ? "$2(fading)$7" : "        "),
                   apply_types[(qint32)aff->location], scratch);
      }
      master->send(buf);
      if (++level == 4)
        level = {};
    }
  }

  master->sendln("($5:$7)=========================================================================($5:$7)");

  return ReturnValue::eSUCCESS;
}

qint32 spell_release_golem(quint8 level, CharacterPtr ch, QString arg, qint32 type, CharacterPtr tar_ch, ObjectPtr tar_obj, qint32 skill)
{
  follow_type *fol;
  for (fol = ch->followers; fol; fol = fol->next)
    if (fol->follower->isNonPlayer() && dc_->mob_index[fol->follower->mobdata->nr].vnum() == 8)
    {
      release_message(fol->follower);
      extract_char(fol->follower, false);
      return ReturnValue::eSUCCESS;
    }
  ch->sendln("You don't have a golem.");
  return ReturnValue::eSUCCESS;
}
