/************************************************************************
| $Id: guild.cpp,v 1.132 2014/07/04 22:00:04 jhhudso Exp $
| guild.C
| This contains all the guild commands - practice, gain, etc..
*/
#include "DC/DC.h"

extern QList<profession> professions;

command_return_t do_practice(CharacterPtr ch, QString arg, cmd_t cmd)
{
  /* Call "guild" with a null QString for an argument.
     This displays the character's skills. */

  if (arg[0] != '\0')
  {
    ch->sendln("You can only practice in a guild.");
  }
  else
  {
    guild(ch, 0, cmd, "", ch);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_profession(CharacterPtr ch, QString args, cmd_t cmd)
{
  // Command is enabled by giving someone the profession skill
  if (!ch->has_skill(SKILL_PROFESSION))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  // You can only use the command in the same room of a mob named guildmaster
  CharacterPtr victim = get_mob_room_vis(ch, "guildmaster");
  if (victim == nullptr)
  {
    ch->sendln("You can't pick a profession here. You need to find a Guild Master.");
    return ReturnValue::eFAILURE;
  }

  QString arg1;
  one_argument(args, arg1);

  // Show syntax if no argument is given
  if (arg1[0] == '\0')
  {
    // Show list of available professions based on player's class type
    ch->send(u"As a %s you can pick from the following professions:\r\n"_s.arg(pc_clss_types3[GET_CLASS(ch)]));

    for (QList<profession>::iterator i = professions.begin(); i != professions.end(); ++i)
    {
      if ((*i).c_class == GET_CLASS(ch))
      {
        ch->send(u"%s\r\n"_s.arg((*i).Name.c_str()));
      }
    }

    ch->sendln(u"\r\nSyntax: profession [profession name]\r\n        profession reset\r\n\r\nResetting your profession costs 10000 platinum."_s);

    return ReturnValue::eSUCCESS;
  }

  bool found = false;
  for (QList<profession>::iterator i = professions.begin(); i != professions.end(); ++i)
  {
    if ((*i).name == QString(arg1))
    {
      if ((*i).c_class == GET_CLASS(ch))
      {
        found = true;
        break;
      }
      else
      {
        ch->sendln("That profession is not available to your class.");
        return ReturnValue::eFAILURE;
      }
    }
  }

  if (found == false)
  {
    ch->send(u"%s not a valid profession.\r\n"_s.arg(arg1));
    return ReturnValue::eFAILURE;
  }

  return ReturnValue::eSUCCESS;
}

const QString per_col(qint32 percent)
{
  if (percent == 0)
    return ("$B$0");
  if (percent <= 5)
    return ("$4");
  if (percent <= 10)
    return ("$4");
  if (percent <= 15)
    return ("$B$4");
  if (percent <= 20)
    return ("$B$4");
  if (percent <= 30)
    return ("$5");
  if (percent <= 40)
    return ("$5");
  if (percent <= 50)
    return ("$B$5");
  if (percent <= 60)
    return ("$B$5");
  if (percent <= 70)
    return ("$2");
  if (percent <= 80)
    return ("$B$2");
  if (percent <= 85)
    return ("$B$3");
  if (percent <= 90)
    return ("$B$3");
  if (percent <= 100)
    return ("$B$7");
  return ("$B$1");
}

const QString how_good(qint32 percent)
{
  if (percent == 0)
    return (" not learned$R");
  if (percent <= 5)
    return (" horrible$R");
  if (percent <= 10)
    return (" crappy$R");
  if (percent <= 15)
    return (" meager$R");
  if (percent <= 20)
    return (" bad$R");
  if (percent <= 30)
    return (" poor$R");
  if (percent <= 40)
    return (" decent$R");
  if (percent <= 50)
    return (" average$R");
  if (percent <= 60)
    return (" fair$R");
  if (percent <= 70)
    return (" good$R");
  if (percent <= 80)
    return (" very good$R");
  if (percent <= 85)
    return (" excellent$R");
  if (percent <= 90)
    return (" superb$R");
  if (percent <= 100)
    return (" Masterful$R");

  return (" Transcendent$R");
}

// return a 1 if I just learned skill for first time
// else 0
qint32 Character::learn_skill(qint32 skill, qint32 amount, qint32 maximum)
{
  if (skills.contains(skill))
  {
    auto &learned = getSkill(skill).learned;
    auto old = learned;
    learned += amount;

    if (skill == SKILL_MAGIC_RESIST)
    {
      barb_magic_resist(this, old, learned);
    }

    if (learned > maximum)
    {
      learned = maximum;
    }
  }
  else
  {
    if (skill == SKILL_MAGIC_RESIST)
    {
      barb_magic_resist(this, 0, amount);
    }
    setSkill(skill, amount);
    prepare_character_for_sixty(this);
    return true;
  }

  return false;
}

qint32 search_skills2(qint32 arg, CharacterClassSkill *list_skills)
{
  for (qint32 i = {}; *list_skills[i].skillname != '\n'; i++)
    if (arg == list_skills[i].skillnum)
      return i;

  return -1;
}

qint32 search_skills(const QString arg, CharacterClassSkill *list_skills)
{
  for (qint32 i = {}; *list_skills[i].skillname != '\n'; i++)
    if (is_abbrev(arg, list_skills[i].skillname))
      return i;

  return -1;
}

CharacterClassSkill *Character::get_skill_list(void)
{
  CharacterClassSkill *skilllist = {};

  switch (GET_CLASS(this))
  {
  case CLASS_THIEF:
    skilllist = t_skills;
    break;
  case CLASS_WARRIOR:
    skilllist = w_skills;
    break;
  case CLASS_BARBARIAN:
    skilllist = b_skills;
    break;
  case CLASS_PALADIN:
    skilllist = p_skills;
    break;
  case CLASS_ANTI_PAL:
    skilllist = a_skills;
    break;
  case CLASS_RANGER:
    skilllist = r_skills;
    break;
  case CLASS_BARD:
    skilllist = d_skills;
    break;
  case CLASS_MONK:
    skilllist = k_skills;
    break;
  case CLASS_DRUID:
    skilllist = u_skills;
    break;
  case CLASS_CLERIC:
    skilllist = c_skills;
    break;
  case CLASS_MAGIC_USER:
    skilllist = m_skills;
    break;
  default:
    break;
  }
  return skilllist;
}

const QString attrstring(qint32 attr)
{
  switch (attr)
  {
  case STRDEX:
    return "Str/Dex";
  case STRCON:
    return "Str/Con";
  case STRINT:
    return "Str/Int";
  case STRWIS:
    return "Str/Wis";
  case DEXCON:
    return "Dex/Con";
  case DEXINT:
    return "Dex/Int";
  case DEXWIS:
    return "Dex/Wis";
  case CONINT:
    return "Con/Int";
  case CONWIS:
    return "Con/Wis";
  case INTWIS:
    return "Int/Wis";
  default:
    return "oh shit!";
  }
}

const QString attrname(qint32 clss, qint32 attr)
{
  switch (attr)
  {
  case STRDEX:
    if (clss == CLASS_WARRIOR)
      return "Offensive";
    else if (clss == CLASS_THIEF)
      return "Assassination";
    else if (clss == CLASS_ANTI_PAL)
      return "Combat";
    else if (clss == CLASS_MONK)
      return "Tiger";
    else if (clss == CLASS_RANGER)
      return "Physical Prowess";
    else if (clss == CLASS_BARD)
      return "Melee";
    else if (clss == CLASS_DRUID)
      return "Retribution";
    else
      return "ERR1";
  case STRCON:
    if (clss == CLASS_WARRIOR)
      return "Defensive";
    else if (clss == CLASS_PALADIN)
      return "Judgement";
    else if (clss == CLASS_BARBARIAN)
      return "Fury";
    else if (clss == CLASS_RANGER)
      return "Feral Summoning";
    else if (clss == CLASS_DRUID)
      return "Prevention";
    else if (clss == CLASS_CLERIC)
      return "Theurgy";
    else if (clss == CLASS_MAGIC_USER)
      return "Conjuration";
    else
      return "ERR2";
  case STRINT:
    if (clss == CLASS_ANTI_PAL)
      return "Thaumaturgy";
    else if (clss == CLASS_BARBARIAN)
      return "Aggression";
    else if (clss == CLASS_CLERIC)
      return "Castigation";
    else if (clss == CLASS_MAGIC_USER)
      return "Evocation";
    else
      return "ERR3";
  case STRWIS:
    if (clss == CLASS_PALADIN)
      return "Consecration";
    else
      return "ERR4";
  case DEXCON:
    if (clss == CLASS_BARBARIAN)
      return "Reflex";
    else if (clss == CLASS_BARD)
      return "Assault";
    else
      return "ERR5";
  case DEXINT:
    if (clss == CLASS_WARRIOR)
      return "Strategy";
    else if (clss == CLASS_THIEF)
      return "Subtlety";
    else if (clss == CLASS_ANTI_PAL)
      return "Treachery";
    else if (clss == CLASS_PALADIN)
      return "Battle";
    else if (clss == CLASS_RANGER)
      return "Hunter's Lore";
    else if (clss == CLASS_CLERIC)
      return "Augury";
    else if (clss == CLASS_MAGIC_USER)
      return "Enchantment";
    else
      return "ERR6";
  case DEXWIS:
    if (clss == CLASS_THIEF)
      return "Deception";
    else if (clss == CLASS_BARBARIAN)
      return "Arsenal";
    else if (clss == CLASS_MONK)
      return "Monkey";
    else if (clss == CLASS_DRUID)
      return "Natural";
    else if (clss == CLASS_CLERIC)
      return "Divinity";
    else if (clss == CLASS_MAGIC_USER)
      return "Invocation";
    else
      return "ERR7";
  case CONINT:
    if (clss == CLASS_THIEF)
      return "Martial";
    else if (clss == CLASS_ANTI_PAL)
      return "Desecration";
    else if (clss == CLASS_MONK)
      return "Dragon";
    else if (clss == CLASS_BARD)
      return "Charm";
    else if (clss == CLASS_DRUID)
      return "Elemental";
    else
      return "ERR8";
  case CONWIS:
    if (clss == CLASS_WARRIOR)
      return "Weapons";
    else if (clss == CLASS_ANTI_PAL)
      return "Necromancy";
    else if (clss == CLASS_PALADIN)
      return "Sanctification";
    else if (clss == CLASS_MONK)
      return "Crane";
    else if (clss == CLASS_RANGER)
      return "Protective Instincts";
    else if (clss == CLASS_BARD)
      return "Enhancement";
    else if (clss == CLASS_DRUID)
      return "Malediction";
    else if (clss == CLASS_CLERIC)
      return "Intercession";
    else if (clss == CLASS_MAGIC_USER)
      return "Abjuration";
    else
      return "ERR9";
  case INTWIS:
    if (clss == CLASS_PALADIN)
      return "Benediction";
    else if (clss == CLASS_RANGER)
      return "Natural Affinity";
    else if (clss == CLASS_BARD)
      return "Detection";
    else if (clss == CLASS_DRUID)
      return "Medicinal";
    else if (clss == CLASS_CLERIC)
      return "Restoration";
    else if (clss == CLASS_MAGIC_USER)
      return "Divination";
    else
      return "ERR10";
  default:
    return "Err";
  }
}

qint32 Character::skillmax(qint32 skill, qint32 eh)
{
  if (isNonPlayer())
  {
    return eh;
  }

  CharacterClassSkill *skilllist = get_skill_list();
  if (!skilllist)
  {
    skilllist = g_skills;
  }

  qint32 i = search_skills2(skill, skilllist);
  if (i == -1 && skilllist != g_skills)
  {
    skilllist = g_skills;
    i = search_skills2(skill, g_skills);
  }

  if (i == -1)
  {
    return eh;
  }

  if (skilllist[i].maximum < i)
  {
    return skilllist[i].maximum;
  }
  return eh;
}

QChar Character::charthing(qint32 known, qint32 skill, qint32 maximum)
{
  if (get_max(skill) <= known)
    return '*';
  if (known >= maximum / 2)
    return '=';
  return '+';
}

void Character::output_praclist(CharacterClassSkill *skilllist)
{
  qint32 known, last_profession = {};
  QString buf;
  for (qint32 i = {}; *skilllist[i].skillname != '\n'; i++)
  {
    known = has_skill(skilllist[i].skillnum);
    if (!known && getLevel() < skilllist[i].levelavailable)
      continue;

    if (isPlayer() && skilllist[i].group > 0 && skilllist[i].group != player->profession)
    {
      continue;
    }
    else
    {
      if (last_profession != skilllist[i].group)
      {
        last_profession = skilllist[i].group;
        send(u"\r\n$B%s Profession Skills:$R\r\n"_s.arg(qPrintable(find_profession(c_class).arg(skilllist[i].group))));
        sendln(" Ability:                Current/Practice/Autolearn  Cost:     Group:");
        sendln("--------------------------------------------------------------------------------");
      }
    }

    quint64 self_learn_max = skilllist[i].maximum * 0.5;
    if (getLevel() * 2 < self_learn_max)
    {
      self_learn_max = getLevel() * 2;
    }

    dc_sprintf(buf, " %c%-24s%s%15s $B$0$R%s%3d/%3llu/%3d$B$0$R  ", UPPER(*skilllist[i].skillname), (skilllist[i].skillname + 1), per_col(known), how_good(known), per_col(known), known, self_learn_max, get_max(skilllist[i].skillnum));
    send(buf);
    if (skilllist[i].skillnum >= 1 && skilllist[i].skillnum <= MAX_SPL_LIST)
    {
      if (skilllist[i].skillnum == SPELL_PORTAL && GET_CLASS(this) == CLASS_CLERIC)
        dc_sprintf(buf, "Mana: $B%3d$R ", 150);
      else
        dc_sprintf(buf, "Mana: $B%3d$R ", use_mana(this, skilllist[i].skillnum));
      send(buf);
    }
    else if (skilllist[i].skillnum >= SKILL_SONG_BASE && skilllist[i].skillnum <= SKILL_SONG_MAX)
    {
      send(u"Ki:   $B%1$R "_s.arg(song_info[skilllist[i].skillnum - SKILL_SONG_BASE].min_useski(), 3));
    }
    else if (skilllist[i].skillnum >= KI_OFFSET && skilllist[i].skillnum <= KI_OFFSET + MAX_KI_LIST)
    {
      send(u"Ki:   $B%1$R "_s.arg(ki_info[skilllist[i].skillnum - KI_OFFSET].min_useski(), 3));
    }
    else if (skilllist[i].skillnum == 318) // scan
    {
      send(u"Move: $B%1$R "_s.arg(2, 3));
    }
    else if (skilllist[i].skillnum == 320) // switch
    {
      send(u"Move: $B%1$R "_s.arg(4, 3));
    }
    else if (skilllist[i].skillnum == 319) // consider
    {
      send(u"Move: $B%1$R "_s.arg(5, 3));
    }
    else if (skilllist[i].skillnum == 368) // release
    {
      send(u"Move: $B%1$R "_s.arg(25, 3));
    }
    else if (skilllist[i].skillnum == 380) // fire arrows
    {
      send(u"Mana: $B%1$R "_s.arg(30, 3));
    }
    else if (skilllist[i].skillnum == 381) // ice arrows
    {
      send(u"Mana: $B%1$R "_s.arg(20, 3));
    }
    else if (skilllist[i].skillnum == 382) // tempest arrows
    {
      send(u"Mana: $B%1$R "_s.arg(10, 3));
    }
    else if (skilllist[i].skillnum == 383) // granite arrows
    {
      send(u"Mana: $B%1$R "_s.arg(40, 3));
    }
    else if (skill_cost.find(skilllist[i].skillnum) != skill_cost.end())
    {
      send(u"Move: $B%1$R "_s.arg(skill_cost.find(skilllist[i].skillnum)->second, 3));
    }
    else
      send("          ");
    if (skilllist[i].attrs)
    {
      if (skilllist != g_skills)
      {
        dc_sprintf(buf, " %s", attrname(GET_CLASS(this), skilllist[i].attrs));
        send(buf);
      }
      else
        send(" General");
    }
    if (skilllist[i].skillnum == SKILL_SONG_DISARMING_LIMERICK)
    {
      sendln("$B        #$R");
    }
    else if (skilllist[i].skillnum == SKILL_SONG_FANATICAL_FANFARE)
    {
      sendln("$B  #$R");
    }
    else if (skilllist[i].skillnum == SKILL_SONG_SEARCHING_SONG)
    {
      sendln("$B    #$R");
    }
    else if (skilllist[i].skillnum == SKILL_SONG_VIGILANT_SIREN)
    {
      sendln("$B  #$R");
    }
    else if (skilllist[i].skillnum == SKILL_SONG_MKING_CHARGE)
    {
      sendln("$B      #$R");
    }
    else if (skilllist[i].skillnum == SKILL_SONG_HYPNOTIC_HARMONY)
    {
      sendln("$B        #$R");
    }
    else if (skilllist[i].skillnum == SKILL_SONG_SHATTERING_RESO)
    {
      sendln("$B        #$R");
    }
    else
      sendln("");
  }
}

qint32 Character::skills_guild(const QString arg, CharacterPtr owner)
{
  QString buf;
  qint32 known, x;
  qint32 skillnumber;
  qint32 percent;

  if (isNonPlayer())
    return ReturnValue::eFAILURE;

  CharacterClassSkill *skilllist = get_skill_list();

  if (!skilllist)
    return ReturnValue::eFAILURE; // no skills to train

  if (arg.isEmpty()) // display skills that can be learned
  {
    dc_sprintf(buf, "You have %d practice sessions left.\r\n", player->practices);
    send(buf);
    sendln("You can practice any of these skills:\r\n");

    sendln("$BUniversal skills:$R");
    sendln(" Ability:                Current/Practice/Autolearn  Cost:     Group:");
    sendln("--------------------------------------------------------------------------------");
    output_praclist(g_skills);

    dc_sprintf(buf, "\r\n$B%c%s skills:$R\r\n", UPPER(*pc_clss_types[GET_CLASS(this)]), (1 + pc_clss_types[GET_CLASS(this)]));
    send(buf);

    if (GET_CLASS(this) != CLASS_MONK)
      sendln(" Ability:                Current/Practice/Autolearn  Cost:     Group:");
    else
      sendln(" Ability:                Current/Practice/Autolearn  Cost:     Style:");

    sendln("--------------------------------------------------------------------------------");

    output_praclist(skilllist);

    sendln("");

    if (GET_CLASS(this) == CLASS_BARD)
      sendln("$B#$R denotes a song which requires an instrument.");

    return ReturnValue::eSUCCESS;
  }

  if (GET_POS(this) == position_t::SLEEPING)
  {
    sendln("You cannot practice in your sleep.");
    return ReturnValue::eSUCCESS;
  }
  skillnumber = search_skills(arg, skilllist);

  if (skillnumber == -1)
  {
    return ReturnValue::eFAILURE;
  }

  if (getLevel() < skilllist[skillnumber].levelavailable)
  {
    sendln("You aren't advanced enough for that yet.");
    return ReturnValue::eSUCCESS;
  }

  x = skilllist[skillnumber].skillnum;
  known = has_skill(x);

  // we can only train them if they already know it, or if we're the trainer for that skill
  if (!known)
  {
    if (GET_CLASS(this) != GET_CLASS(owner))
    {
      do_say(owner, "I am sorry, I cannot teach you that.  You will have to find another trainer.");
      return ReturnValue::eSUCCESS;
    }
    else
    {
      skill_quest *sq;
      if ((sq = find_sq(skilllist[skillnumber].skillname)) != nullptr && sq->message && isSet(sq->clas, 1 << (GET_CLASS(this) - 1)))
      {
        mprog_driver(sq->message, owner, this, nullptr, nullptr, nullptr, nullptr);
        switch (skillnumber)
        {
        case SPELL_VAMPIRIC_AURA:
        case SPELL_DIVINE_INTER:
        case SKILL_BULLRUSH:
        case SPELL_HOLY_AURA:
        case SKILL_SPELLCRAFT:
        case SKILL_COMBAT_MASTERY:
        case KI_MEDITATION + KI_OFFSET:
        case SPELL_CONJURE_ELEMENTAL:
        case SPELL_RELEASE_ELEMENTAL:
        case SKILL_CRIPPLE:
        case SKILL_NAT_SELECT:
          do_say(owner, "Alternately, should you feel that you are not up to the task of an exciting quest, you can seek the Skills Master west of town.");
          do_say(owner, "Rumour has it he will teach certain skills for a hefty fee.  He will give you a LIST of what he has to offer.");
          return ReturnValue::eFAILURE;
        default:
          break;
        }
        return ReturnValue::eSUCCESS;
      }
    }

    // If this is a profession-specific skill and we are a mortal without that profession, disallow
    if (skilllist[skillnumber].group && isPlayer() && skilllist[skillnumber].group != player->profession)
    {
      send(u"You must join the %s profession in order to learn that.\r\n"_s.arg(qPrintable(find_profession(c_class).arg(skilllist[skillnumber].group))));
      return ReturnValue::eSUCCESS;
    }
  }
  if (player->practices <= 0)
  {
    sendln("You do not seem to be able to practice now.");
    return ReturnValue::eSUCCESS;
  }

  if (known >= get_max(x))
  {
    do_emote(owner, u"eyes you up and down."_s);
    do_say(owner, u"Taking into account your current attributes, your"_s);
    do_say(owner, u"maximum proficiency in this ability has been reached."_s);
    return ReturnValue::eSUCCESS;
  }

  if (known >= skilllist[skillnumber].maximum)
  {
    sendln("You are already learned in this area.");
    return ReturnValue::eSUCCESS;
  }
  qreal maxlearn = (qreal)skilllist[skillnumber].maximum;
  maxlearn *= 0.5;

  if (known >= (getLevel() * 2))
  {
    sendln("You are not experienced enough to practice that any further right now.");
    return ReturnValue::eSUCCESS;
  }
  if (known >= maxlearn)
  {
    sendln("You cannot learn more here.. you need to go out into the world and use it.");
    return ReturnValue::eSUCCESS;
  }
  switch (skillnumber)
  {
  case SPELL_VAMPIRIC_AURA:
  case SPELL_DIVINE_INTER:
  case SKILL_BULLRUSH:
  case SPELL_HOLY_AURA:
  case SKILL_SPELLCRAFT:
  case SKILL_COMBAT_MASTERY:
  case KI_MEDITATION + KI_OFFSET:
  case SKILL_CRIPPLE:
  case SPELL_CONJURE_ELEMENTAL:
  case SPELL_RELEASE_ELEMENTAL:
  case SKILL_NAT_SELECT:
    do_say(owner, "I cannot teach you that. You need to learn it by yourself.");

    return ReturnValue::eFAILURE;
  default:
    break;
  }

  if (!known)
    switch (GET_CLASS(this))
    {
    case CLASS_WARRIOR:
      do_say(owner, "Yar! I can be teachin' ye that skill myself! It should only take but a moment.");
      break;
    case CLASS_BARBARIAN:
      do_say(owner, "Hah! That easy to learn! I teach you meself.");
      break;
    case CLASS_THIEF:
      do_say(owner, "So young rogue, you wish to advance your skills.  I can teach you of this particular one myself.");
      break;
    case CLASS_MONK:
      do_say(owner, "Ahh, well met grasshopper!  I can teach you of this from my own knowledge.");
      break;
    case CLASS_RANGER:
      do_say(owner, "My woodland lore is more than sufficient to teach you this myself young apprentice!");
      break;
    case CLASS_ANTI_PAL:
      do_say(owner, "Ahh, young dark lord, it is but a simple matter to learn this ability.  Follow my inion.");
      break;
    case CLASS_PALADIN:
      do_say(owner, "This ability is one that I am capable of teaching you myself young novice.  Observe closely.");
      break;
    case CLASS_BARD:
      do_say(owner, "Ahh young prodigy, that is a tune with which I myself am familiar! Allow me to show you...");
      break;
    case CLASS_MAGIC_USER:
      do_say(owner, "Ahh young apprentice, this is a simple matter for me to teach you if you are capable of comprehending.");
      break;
    case CLASS_CLERIC:
      do_say(owner, "Well met Acolyte!  I shall pray for you to receive knowledge of the blessing you request.");
      break;
    case CLASS_DRUID:
      do_say(owner, "Nature be with you, young druid.  I can teach you the ability you seek myself if you are willing.");
      break;
    }

  sendln("You practice for a while...");
  player->practices--;

  percent = 1 + (qint32)int_app[GET_INT(this)].learn_bonus;

  learn_skill(x, percent, skilllist[skillnumber].maximum);

  if (known >= skilllist[skillnumber].maximum)
  {
    sendln("You are now learned in this area.");
    return ReturnValue::eSUCCESS;
  }

  return ReturnValue::eSUCCESS;
}

qint32 guild(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr owner)
{
  qint64 exp_needed;
  qint32 x = {};

  if (cmd == cmd_t::GAIN && !ch->isNonPlayer())
  { /*   gain crap...  */

    if (ch->isImmortalPlayer() || ch->getLevel() >= DC::MAX_MORTAL_LEVEL)
    {
      ch->sendln("You have already reached the highest level!");
      return ReturnValue::eSUCCESS;
    }

    // TODO - make it so you have to be at YOUR guildmaster to gain

    if (ch->getLevel() == 50)
    { // To get past 50 you need to know your q skill.
      qint32 skl = -1;
      switch (GET_CLASS(ch))
      {
      case CLASS_MAGE:
        skl = SKILL_SPELLCRAFT;
        break;
      case CLASS_BARBARIAN:
        skl = SKILL_BULLRUSH;
        break;
      case CLASS_PALADIN:
        skl = SPELL_HOLY_AURA;
        break;
      case CLASS_MONK:
        skl = KI_OFFSET + KI_MEDITATION;
        break;
      case CLASS_WARRIOR:
        skl = SKILL_COMBAT_MASTERY;
        break;
      case CLASS_THIEF:
        skl = SKILL_CRIPPLE;
        break;
      case CLASS_RANGER:
        skl = SKILL_NAT_SELECT;
        break;
      case CLASS_CLERIC:
        skl = SPELL_DIVINE_INTER;
        break;
      case CLASS_ANTI_PAL:
        skl = SPELL_VAMPIRIC_AURA;
        break;
      case CLASS_DRUID:
        skl = SPELL_CONJURE_ELEMENTAL;
        break;
      case CLASS_BARD:
        skl = SKILL_SONG_HYPNOTIC_HARMONY;
        break;
      }
      if (!ch->has_skill(skl))
      {
        ch->sendln("You need to learn your Quest Skill before you can progress further.");
        return ReturnValue::eSUCCESS;
      }
    }
    exp_needed = exp_table[(qint32)ch->getLevel() + 1];

    if (exp_needed > ch->exp)
    {
      x = (qint32)(exp_needed - ch->exp);

      ch->send(u"You need %1 experience to level.\r\n"_s.arg(x));
      return ReturnValue::eSUCCESS;
    }

    ch->sendln("You raise a level!");
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_MORTAL, "%s (%s) just gained level %d.", qPrintable(ch->name()), pc_clss_types3[(qint32)GET_CLASS(ch)], ch->getLevel() + 1);

    ch->incrementLevel();
    advance_level(ch, 0);
    ch->exp -= (qint32)exp_needed;
    qint32 bonus = (ch->getLevel() - 50) * 250;
    if (bonus > 0 && DC::MAX_MORTAL_LEVEL == 60)
    {
      QString buf;
      if (ch->getLevel() == 60)
      {
        dc_sprintf(buf, "You have truly reached the highest level of %s mastery.", pc_clss_types3[GET_CLASS(ch)]);
        do_say(owner, buf);
        do_say(owner, "As such, the guild will imbue into you some of our most powerful magic and grant you freedom from hunger and thirst!");
        //	     ch->send(buf);
      }
      else
      {
        dc_sprintf(buf, "Well done master %s, the guild has collected a tithe to reward your continued support of our profession.", pc_clss_types3[GET_CLASS(ch)]);

        do_say(owner, buf);
        dc_sprintf(buf, "Your guildmaster gives you %d platinum coins.\r\n", bonus);
        ch->send(buf);
        GET_PLATINUM(ch) += bonus;
      }
    }
    return ReturnValue::eSUCCESS;
  }

  if (cmd == cmd_t::REMORT)
  { // remort crap
    qint32 groupnumber;

    if (isSet(ch->player->toggles, Player::PLR_CLS_TREE_A))
    {
      REMOVE_BIT(ch->player->toggles, Player::PLR_CLS_TREE_A);
      groupnumber = 1;
    }
    else if (isSet(ch->player->toggles, Player::PLR_CLS_TREE_B))
    {
      REMOVE_BIT(ch->player->toggles, Player::PLR_CLS_TREE_B);
      groupnumber = 2;
    }
    else if (isSet(ch->player->toggles, Player::PLR_CLS_TREE_C))
    {
      REMOVE_BIT(ch->player->toggles, Player::PLR_CLS_TREE_C);
      groupnumber = 3;
    }
    else
    {
      ch->sendln("You have not even chosen a profession out of which to remort!");
      return ReturnValue::eSUCCESS;
    }

    ch->sendln("Your profession skills have been reset.");
    CharacterClassSkill *class_skills = ch->get_skill_list();
    char_skill_data *skill;

    for (qint32 i = {}; *class_skills[i].skillname != '\n'; i++)
      if (class_skills[i].group == groupnumber)
      {
        if (ch->skills.contains(class_skills[i].skillnum))
        {
          ch->skills[class_skills[i].skillnum].learned = {};
        }
      }

    ch->sendln("You have remorted back to level 50.");
    if (ch->getLevel() <= DC::MAX_MORTAL_LEVEL)
      ch->setLevel(50);

    SET_BIT(ch->player->toggles, Player::PLR_REMORTED);

    ch->save(cmd_t::SAVE_SILENTLY);
    return ReturnValue::eSUCCESS;
  }

  if ((cmd != cmd_t::PRACTICE))
    return ReturnValue::eFAILURE;

  if (ch->isNonPlayer())
  {
    ch->sendln("Why practice?  You're just going to die anyway...");
    return ReturnValue::eFAILURE;
  }

  for (; *arg == ' '; arg++)
    ; // skip whitespace

  if (arg.isEmpty())
  {
    ch->skills_guild(arg, owner);
  }
  else
  {
    if (isSet(ch->skills_guild(arg, owner), ReturnValue::eSUCCESS))
      return ReturnValue::eSUCCESS;
    else if (search_skills(arg, g_skills) != -1)
      do_say(owner, "Seek out the SKILLS MASTER in the forests west of Sorpigal to learn ch ability.");
    else
      ch->sendln("You do not know of ch ability...");
  }

  return ReturnValue::eSUCCESS;
}

qint32 skill_master(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr invoker)
{
  QString buf;
  qint32 number, i, percent;
  qint32 learned = {};
  CharacterClassSkill *skilllist = ch->get_skill_list();

  if (ch->isNonPlayer())
  {
    ch->sendln("Why practice?  You're just going to die anyway...");
    return ReturnValue::eFAILURE;
  }

  if (cmd != cmd_t::PRACTICE && cmd != cmd_t::BUY && cmd != cmd_t::LIST)
    return ReturnValue::eFAILURE;

  for (; *arg == ' '; arg++)
    ;

  qint32 skl = -1;
  switch (GET_CLASS(ch))
  {
  case CLASS_MAGE:
    skl = SKILL_SPELLCRAFT;
    break;
  case CLASS_BARBARIAN:
    skl = SKILL_BULLRUSH;
    break;
  case CLASS_PALADIN:
    skl = SPELL_HOLY_AURA;
    break;
  case CLASS_MONK:
    skl = KI_OFFSET + KI_MEDITATION;
    break;
  case CLASS_WARRIOR:
    skl = SKILL_COMBAT_MASTERY;
    break;
  case CLASS_THIEF:
    skl = SKILL_CRIPPLE;
    break;
  case CLASS_RANGER:
    skl = SKILL_NAT_SELECT;
    break;
  case CLASS_CLERIC:
    skl = SPELL_DIVINE_INTER;
    break;
  case CLASS_ANTI_PAL:
    skl = SPELL_VAMPIRIC_AURA;
    break;
  case CLASS_DRUID:
    skl = SPELL_CONJURE_ELEMENTAL;
    break;
  case CLASS_BARD:
    skl = SKILL_SONG_HYPNOTIC_HARMONY;
    break;
  }
  if (cmd == cmd_t::LIST)
  {
    QString buf;

    dc_snprintf(buf, sizeof(buf), "This is what is available:\r\n[2000 platinum] %s (type $B$2buy questskill$R to purchase it)\r\n", qPrintable(get_skill_name(skl)));
    ch->send(buf);
    return ReturnValue::eSUCCESS;
  }
  if (cmd == cmd_t::BUY)
  {
    if (ch->getLevel() < 50)
    {
      do_say(invoker, "You have not obtained a high enough level to buy anything from me.");
      return ReturnValue::eSUCCESS;
    }
    if (str_cmp(arg, "questskill"))
    {
      do_say(invoker, "I cannot teach you that. Type LIST to see what is available.");
      //      do_say(invoker,"I could teach you your Quest Skill, for a price of 2000 platinum coins.",9);
      //     do_say(invoker,"Just \"buy questskill\" to obtain it.",9);
      return ReturnValue::eSUCCESS;
    }
    if (ch->has_skill(skl))
    {
      do_say(invoker, "I cannot teach you anything further.");
      return ReturnValue::eSUCCESS;
    }
    if (GET_PLATINUM(ch) < 2000)
    {
      do_say(invoker, "You can't afford it, you need 2000 platinum!");
      return ReturnValue::eSUCCESS;
    }
    GET_PLATINUM(ch) -= 2000;
    do_say(invoker, "Okay, you've got a deal!");
    ch->learn_skill(skl, 1, 1);

    extern void prepare_character_for_sixty(CharacterPtr ch);
    prepare_character_for_sixty(ch);
    dc_snprintf(buf, sizeof(buf), "$BYou have learned the basics of %s.$R\r\n", qPrintable(get_skill_name(skl)));
    ch->send(buf);

    switch (GET_CLASS(ch))
    {
    case CLASS_DRUID:
      ch->learn_skill(SPELL_RELEASE_ELEMENTAL, 1, 1);
      dc_snprintf(buf, sizeof(buf), "$BYou have learned the basics of %s.$R\r\n", qPrintable(get_skill_name(SPELL_RELEASE_ELEMENTAL)));
      ch->send(buf);
      break;
    }
    return ReturnValue::eSUCCESS;
  }

  if (arg.isEmpty())
  {
    dc_sprintf(buf, "You have %d practice sessions left.\r\n", ch->player->practices);
    ch->send(buf);
    ch->sendln("You can practice any of these skills:");
    for (i = {}; *g_skills[i].skillname != '\n'; i++)
    {
      qint32 known = ch->has_skill(g_skills[i].skillnum);
      if (ch->getLevel() < g_skills[i].levelavailable)
        continue;
      dc_sprintf(buf, " %-20s%14s   (Level %2llu)\r\n", g_skills[i].skillname, how_good(known), g_skills[i].levelavailable);
      ch->send(buf);
    }
    return ReturnValue::eSUCCESS;
  }
  number = search_skills(arg, g_skills);

  if (number == -1)
  {
    if (!skilllist)
      return ReturnValue::eFAILURE;
    if (search_skills(arg, skilllist) != -1)
      do_say(invoker, "You must speak with your guildmaster to learn such a complicated ability.");
    else
      ch->sendln("You do not know of ch skill...");
    return ReturnValue::eSUCCESS;
  }

  if (ch->player->practices <= 0)
  {
    ch->sendln("You do not seem to be able to practice now.");
    return ReturnValue::eSUCCESS;
  }
  learned = ch->has_skill(g_skills[number].skillnum);

  if (learned >= g_skills[number].maximum)
  {
    ch->sendln("You are already learned in ch area.");
    return ReturnValue::eSUCCESS;
  }
  if (learned >= (ch->getLevel() * 3))
  {
    ch->sendln("You aren't experienced enough to practice that any further right now.");
    return ReturnValue::eSUCCESS;
  }

  ch->sendln("You practice for a while...");
  ch->player->practices--;

  percent = 1 + (qint32)int_app[GET_INT(ch)].learn_bonus;

  ch->learn_skill(g_skills[number].skillnum, percent, g_skills[number].maximum);
  learned = ch->has_skill(g_skills[number].skillnum);

  if (learned >= g_skills[number].maximum)
  {
    ch->sendln("You are now learned in ch area.");
    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eSUCCESS;
}

qint32 Character::get_stat(attribute_t stat)
{
  switch (stat)
  {
  case attribute_t::STRENGTH:
    return GET_RAW_STR(this);
    break;
  case attribute_t::INTELLIGENCE:
    return GET_RAW_INT(this);
    break;
  case attribute_t::WISDOM:
    return GET_RAW_WIS(this);
    break;
  case attribute_t::DEXTERITY:
    return GET_RAW_DEX(this);
    break;
  case attribute_t::CONSTITUTION:
    return GET_RAW_CON(this);
    break;
  case attribute_t::UNDEFINED:
    break;
  };
  return 0;
}

qint32 Character::get_stat_bonus(qint32 stat)
{
  qint32 bonus = {};
  switch (stat)
  {
  case STRDEX:
    bonus = MAX(0, get_stat(attribute_t::STRENGTH) - 15) + MAX(0, get_stat(attribute_t::DEXTERITY) - 15);
    break;
  case STRCON:
    bonus = MAX(0, get_stat(attribute_t::STRENGTH) - 15) + MAX(0, get_stat(attribute_t::CONSTITUTION) - 15);
    break;
  case STRINT:
    bonus = MAX(0, get_stat(attribute_t::STRENGTH) - 15) + MAX(0, get_stat(attribute_t::INTELLIGENCE) - 15);
    break;
  case STRWIS:
    bonus = MAX(0, get_stat(attribute_t::STRENGTH) - 15) + MAX(0, get_stat(attribute_t::WISDOM) - 15);
    break;
  case DEXCON:
    bonus = MAX(0, get_stat(attribute_t::DEXTERITY) - 15) + MAX(0, get_stat(attribute_t::CONSTITUTION) - 15);
    break;
  case DEXINT:
    bonus = MAX(0, get_stat(attribute_t::DEXTERITY) - 15) + MAX(0, get_stat(attribute_t::INTELLIGENCE) - 15);
    break;
  case DEXWIS:
    bonus = MAX(0, get_stat(attribute_t::DEXTERITY) - 15) + MAX(0, get_stat(attribute_t::WISDOM) - 15);
    break;
  case CONINT:
    bonus = MAX(0, get_stat(attribute_t::CONSTITUTION) - 15) + MAX(0, get_stat(attribute_t::INTELLIGENCE) - 15);
    break;
  case CONWIS:
    bonus = MAX(0, get_stat(attribute_t::CONSTITUTION) - 15) + MAX(0, get_stat(attribute_t::WISDOM) - 15);
    break;
  case INTWIS:
    bonus = MAX(0, get_stat(attribute_t::INTELLIGENCE) - 15) + MAX(0, get_stat(attribute_t::WISDOM) - 15);
    break;
  default:
    bonus = {};
    break;
  }

  return bonus;
}

// TODO - go ahead and remove 'learned' from everywhere to use it.
// We can't always pass it in, since in 'group' type spells or
// object affects someone that doesn't have the skill is getting a
// valid amount passed in.

void Character::skill_increase_check(qint32 skill, qint32 learned, qint32 difficulty)
{
  qint32 chance, maximum;

  if (isNonPlayer())
  {
    return;
  }

  if (in_room < 1)
  {
    return;
  }

  if (isSet(dc_->world[in_room].room_flags, NOLEARN))
  {
    return;
  }

  if (isSet(dc_->world[in_room].room_flags, SAFE))
  {
    return;
  }

  if (difficulty < 1)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "%s had an invalid skill level of %d in skill %d.", qPrintable(name()), difficulty, skill);
    return; // Skill w/out difficulty.
  }

  if (skill == SKILL_DODGE && affected_by_spell(SKILL_DEFENDERS_STANCE))
  {
    return;
  }

  if (skill == SKILL_COMBAT_MASTERY && dc_->number(0, 8) > 0)
  {
    return;
  }

  learned = has_skill(skill);

  // If we don't know the skill yet then we can't learn it any more
  if (learned < 1)
  {
    return; // get out if i don't have the skill
  }

  // If we've already learned it 2*level or more then we can't learn it anymore until we gain more levels
  if (learned >= (getLevel() * 2))
  {
    return;
  }

  CharacterClassSkill *skilllist = get_skill_list();
  if (!skilllist)
  {
    return; // class has no skills by default
  }

  maximum = {};
  qint32 i;
  for (i = {}; *skilllist[i].skillname != '\n'; i++)
  {
    if (skilllist[i].skillnum == skill)
    {
      maximum = skilllist[i].maximum;
      break;
    }
  }

  if (maximum < 1)
  {
    skilllist = g_skills;
    for (i = {}; *skilllist[i].skillname != '\n'; i++)
    {
      if (skilllist[i].skillnum == skill)
      {
        maximum = skilllist[i].maximum;
        break;
      }
    }
  }

  if (maximum < 1)
  {
    return;
  }

  qreal percent = maximum * 0.75;

  if (skilllist[i].attrs)
  {
    percent += maximum / 100.0 * get_stat_bonus(skilllist[i].attrs);
  }

  percent = MIN<float>(maximum, percent);
  percent = MAX<float>(maximum * 0.75, percent);

  if (learned >= (qint32)percent)
  {
    return;
  }

  chance = dc_->number(1, 101);
  if (learned < 15)
  {
    chance += 5;
  }

  qint32 oi = 101;
  if (difficulty > 500)
  {
    oi = 100;
    difficulty -= 500;
  }

  switch (difficulty)
  {
  case SKILL_INCREASE_EASY:
    if (oi == 101)
    {
      oi = 94;
    }

    oi -= int_app[GET_INT(this)].easy_bonus;
    break;
  case SKILL_INCREASE_MEDIUM:
    if (oi == 101)
    {
      oi = 96;
    }

    oi -= int_app[GET_INT(this)].medium_bonus;
    break;
  case SKILL_INCREASE_HARD:
    if (oi == 101)
    {
      oi = 98;
    }

    oi -= int_app[GET_INT(this)].hard_bonus;
    break;
  default:
    dc_->logentry(u"Illegal difficulty value sent to skill_increase_check"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    break;
  }

  if (oi > chance)
  {
    return;
  }
  // figure out the name of the affect (if any)
  QString skillname = get_skill_name(skill);

  if (skillname.isEmpty())
  {
    send(u"Attempt to increase an unknown skill %1.  Tell a god. (bug)\r\n"_s.arg(skill));
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "skill_increase_check(%s, skill=%d, learned=%d, difficulty=%d): Attempt to increase an unknown skill.", qPrintable(name()), skill, learned, difficulty);
    return;
  }

  // increase the skill by one
  learn_skill(skill, 1, get_max(skill));
  learned = has_skill(skill);
  sendln(u"$R$B$5You feel more competent in your %1 ability. It increased to %2 out of %3.$R"_s.arg(skillname).arg(learned).arg(get_max(skill)));
}

void Character::verify_max_stats(void)
{
  for (auto &curr : skills)
  {
    if (get_max(curr.first) && get_max(curr.first) < curr.second.learned)
    {
      curr.second.learned = get_max(curr.first);
    }
  }
}

qint32 Character::get_max(qint32 skill)
{
  CharacterClassSkill *skilllist = get_skill_list();
  if (!skilllist)
    skilllist = g_skills;

  qint32 maximum = {};
  qint32 i = {};
  for (; *skilllist[i].skillname != '\n'; i++)
  {
    if (skilllist[i].skillnum == skill)
    {
      maximum = skilllist[i].maximum;
      break;
    }
  }

  if (!maximum && skilllist != g_skills)
  {
    skilllist = g_skills;
    for (i = {}; *skilllist[i].skillname != '\n'; i++)
      if (skilllist[i].skillnum == skill)
      {
        maximum = skilllist[i].maximum;
        break;
      }
  }

  qreal percent = maximum * 0.75;

  if (maximum && skilllist[i].attrs)
    percent += maximum / 100.0 * get_stat_bonus(skilllist[i].attrs);

  percent = MIN<float>(maximum, percent);
  percent = MAX<float>(maximum * 0.75, percent);

  return (qint32)percent;
}

void Character::check_maxes(void)
{
  qint32 maximum;
  CharacterClassSkill *skilllist = get_skill_list();
  if (!skilllist)
    return; // class has no skills by default
  maximum = {};
  qint32 i;
  for (i = {}; *skilllist[i].skillname != '\n'; i++)
  {
    maximum = skilllist[i].maximum;
    qreal percent = maximum * 0.75;
    if (skilllist[i].attrs)
      percent += maximum / 100.0 * get_stat_bonus(skilllist[i].attrs);

    percent = MIN<float>(maximum, percent);
    percent = MAX<float>(maximum * 0.75, percent);

    percent = (qint32)percent;

    if (has_skill(skilllist[i].skillnum) > percent)
    {
      if (skills.contains(skilllist[i].skillnum))
      {
        skills[skilllist[i].skillnum].learned = (qint32)percent;
      }
    }
  }

  skilllist = g_skills;
  for (i = {}; *skilllist[i].skillname != '\n'; i++)
  {
    maximum = skilllist[i].maximum;
    qreal percent = maximum * 0.75;
    if (skilllist[i].attrs)
      percent += maximum / 100.0 * get_stat_bonus(skilllist[i].attrs);

    percent = MIN<float>(maximum, percent);
    percent = MAX<float>(maximum * 0.75, percent);

    percent = (qint32)percent;

    if (has_skill(skilllist[i].skillnum) > percent)
    {
      if (skills.contains(skilllist[i].skillnum))
      {
        skills[skilllist[i].skillnum].learned = (qint32)percent;
      }
    }
  }
}
