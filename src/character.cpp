#include "DC/DC.h"

void set_golem(CharacterPtr golem, qint32 golemtype);
ObjectPtr obj_store_to_char(CharacterPtr ch, QTextStream fpsave, ObjectPtr last_cont);

char_file_u4::char_file_u4()
{
}
QString color_to_code(QString color)
{
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

  return colors.value(color);
}

QString Character::getSettingAsColor(QString key, QString defaultValue)
{
  return color_to_code(getSetting(key, defaultValue));
}

QString Character::getSetting(QString key, QString defaultValue)
{
  if (player != nullptr)
  {
    if (player->config == nullptr)
    {
      player->config = new PlayerConfig;
    }
    return player->config->value(key, defaultValue);
  }
  return defaultValue;
}

void Mobile::setObject(ObjectPtr o)
{
  object = o;
}

ObjectPtr Mobile::getObject(void)
{
  return object;
}

bool Mobile::isObject(void)
{
  return object != nullptr;
}

QString Player::getJoining(void)
{
  QString buffer;
  for (const auto &[key, value] : joining.asKeyValueRange())
    if (value)
      buffer += key + u" "_s;
  return buffer.trimmed();
}

void Player::setJoining(QString list)
{
  joining.clear();
  auto parts = list.split(' ');
  for (auto &i : parts)
  {
    joining.insert(i, true);
  }
}

void Player::toggleJoining(QString key)
{
  if (joining.contains(key))
    joining[key] = !joining[key];
  else
    joining[key] = true;
}

PlayerConfig::PlayerConfig(QObject *parent) : QObject(parent), dc_(qobject_cast<DC *>(parent))
{
  config["color.good"] = "green";
  config["color.bad"] = "red";
  config["tell.history.timestamp"] = "0";
  config["gossip.history.timestamp"] = "0";
  config["locale"] = "en_US";
  config["timezone"] = "America/Chicago";
  config["mode"] = "line";
  config["fighting.showdps"] = "0";
  config["dateformat"] = "ISODate";
}

player_config_value_t PlayerConfig::value(const player_config_key_t &key, const player_config_value_t &defaultValue) const
{
  if (config.contains(key) && config.value(key).isEmpty())
    return defaultValue;
  else
    return config.value(key, defaultValue);
}

player_config_key_t PlayerConfig::key(const player_config_value_t &value, const player_config_key_t &defaultKey) const
{
  return config.key(value, defaultKey);
}

QMap<QString, QString> &PlayerConfig::getQMap(void)
{
  return config;
}

player_config_t::iterator PlayerConfig::insert(const player_config_key_t &key, const player_config_value_t &value)
{
  return config.insert(key, value);
}

player_config_t::iterator PlayerConfig::find(const player_config_key_t &key)
{
  return config.find(key);
}

player_config_t::iterator PlayerConfig::begin()
{
  return config.begin();
}

player_config_t::iterator PlayerConfig::end()
{
  return config.end();
}

player_config_t::const_iterator PlayerConfig::constBegin() const
{
  return config.constBegin();
}

player_config_t::const_iterator PlayerConfig::constEnd() const
{
  return config.constEnd();
}

bool Character::isMortalPlayer(void) const
{
  return isPlayer() && level_ < IMMORTAL;
}

bool Character::isImmortalPlayer(void) const
{
  return isPlayer() && level_ >= IMMORTAL;
}

bool Character::isImplementerPlayer(void) const
{
  return isPlayer() && level_ == IMPLEMENTER;
}

quint64 Character::getGold(void)
{
  return gold_;
}

void Character::setGold(quint64 gold)
{
  gold_ = gold;
}

bool Character::addGold(quint64 gold)
{
  if (gold_ + gold < gold)
  {
    return false;
  }

  gold_ += gold;
  return true;
}

bool Character::removeGold(quint64 gold)
{
  if (gold > gold_)
  {
    return false;
  }

  gold_ -= gold;
  return true;
}

bool Character::multiplyGold(double mult)
{
  if (gold_ * mult < gold_)
  {
    return false;
  }

  gold_ *= mult;
  return true;
}

quint64 &Character::getGoldReference(void)
{
  return gold_;
}

bool Character::load_charmie_equipment(QString player_name, bool previous)
{
  qint32 golemtype = {};

  if (player_name.isEmpty())
  {
    return false;
  }

  QTextStream fpfile = {};

  if (isNonPlayer() || level_ < IMMORTAL)
  {
    return false;
  }

  player_name = player_name.toLower();
  player_name[0] = player_name[0].toUpper();

  QString restored = "";
  if (previous)
  {
    restored = ".restored";
  }
  QString filename = u"%1.%2%3"_s.arg(player_name).arg(QString::number(0)).arg(restored);

  QString path = u"%1/%2/"_s.arg(FOLLOWER_DIR).arg(player_name[0]);
  QString fullpath = path + filename;
  if (!(fpfile = fopen(qPrintable(fullpath), "r")))
  {
    send(u"No charmie save file found at '%1'."_s.arg(fullpath));
    return false;
  }

  CharacterPtr charmie = dc_->clone_mobile(real_mobile(8));
  if (charmie == nullptr)
  {
    dc_->logentry(u"Error. clone_mobile(real_mobile(8)) returned nullptr."_s);
    return false;
  }
  charmie->setLevel(1);
  ObjectPtr last_cont = {}; // Last container.
  while (!feof(fpfile))
  {
    last_cont = obj_store_to_char(charmie, fpfile, last_cont);
  }

  char_to_room(charmie, in_room);

  QString message = u"Restored charmie for player %1 with file '%2'."_s.arg(player_name).arg(fullpath);
  send(message);
  dc_->logentry(message);

  if (!previous)
  {
    QFile file(fullpath);
    if (file.rename(fullpath + ".restored"))
    {
      dc_->logentry(u"Renamed '%1' to '%2'."_s.arg(fullpath).arg(fullpath + ".restored"));
    }
  }

  return true;
}

bool DC::validateName(QString name)
{
  if (name.isEmpty() || name.size() < Character::MIN_NAME_SIZE || name.size() > Character::MAX_NAME_SIZE)
  {
    return false;
  }

  for (auto &c : name)
  {
    if (!isalpha(c.toLatin1()))
    {
      return false;
    }
  }

  if (dc_->on_forbidden_name_list(name))
  {
    return false;
  }

  return true;
}

void Connection::send(QString txt)
{
  /* if there's no descriptor, don't worry about output */
  if (descriptor == 0)
  {
    return;
  }

  if (allowColor && !isEditing())
  {
    txt = handle_ansi(txt, character);
  }

  if (character && IS_AFFECTED(character, AFF_INSANE) && isPlaying())
  {
    txt = scramble_text(txt);
  }

  output += txt.toStdString();
}

QString Connection::getName(void)
{
  if (character)
  {
    return character->name();
  }
  return {};
}

const QStringList Object::apply_types =
    {
        "NONE", // 0
        "STR",
        "DEX",
        "INT",
        "WIS",
        "CON",
        "SEX",
        "CLASS",
        "LEVEL",
        "AGE",
        "CHAR_WEIGHT", // 10
        "CHAR_HEIGHT",
        "MANA",
        "HIT_POINTS",
        "MOVE",
        "GOLD",
        "EXP",
        "ARMOR",
        "HITROLL",
        "DAMROLL",
        "SAVE_VS_FIRE", // 20
        "SAVE_VS_COLD",
        "SAVE_VS_ENERGY",
        "SAVE_VS_ACID",
        "SAVE_VS_MAGIC",
        "SAVE_VS_POISON",
        "HIT -N- DAM",
        "SANCTUARY",
        "SENSE LIFE",
        "DETECT INVISIBLE",
        "INVISIBILITY", // 30
        "SNEAK",
        "INFRARED",
        "HASTE",
        "PROTECTION FROM EVIL",
        "FLY",
        "MAGIC MISSILE",
        "WEP BLIND",
        "EARTHQUAKE",
        "CURSE",
        "COLOUR SPRAY", // 40
        "DISPEL EVIL",
        "ENERGY DRAIN",
        "FIREBALL",
        "LIGHTNING BOLT",
        "HARM",
        "POISON",
        "SLEEP",
        "FEAR",
        "DISPEL MAGIC",
        "WEAKEN", // 50
        "CAUSE LIGHT",
        "CAUSE CRITICAL",
        "PARALYZE",
        "ACID BLAST",
        "BEE STING",
        "CURE LIGHT",
        "FLAMESTRIKE",
        "HEAL SPRAY",
        "DROWN",
        "HOWL", // 60
        "SOULDRAIN",
        "SPARKS",
        "BARKSKIN",
        "RESIST FIRE",
        "RESIST COLD",
        "KI",
        "CAMOUFLAGE",
        "FARSIGHT",
        "FREEFLOAT",
        "FROSTSHIELD", // 70
        "INSOMNIA",
        "LIGHTNING SHIELD",
        "REFLECT",
        "RESIST ELECTRICITY",
        "SHADOWSLIP",
        "SOLIDITY",
        "STABILITY",
        "STAUNCHBLOOD",
        "DISPEL GOOD",
        "TELEPORT", // 80
        "CHILL TOUCH",
        "POWER HARM",
        "VAMPIRIC TOUCH",
        "LIFE LEECH",
        "METEOR SWARM",
        "ENTANGLE",
        "INSANE",
        "GLITTER DUST",
        "RESIST ACID",
        "HP REGEN",
        "MANA REGEN",
        "MOVE REGEN",
        "KI REGEN",
        "CREATE FOOD",
        "DAMAGED",
        "THIEF_POISON",
        "PROTECTION FROM GOOD",
        "MELEE MITIGATION",
        "SPELL MITIGATION",
        "SONG MITIGATION",
        "RESIST MAGIC",
        "ALL SAVES",
        "SPELLDAMAGE",
        "FREEDOM FROM HUNGER",
        "FREEDOM FROM THIRST",
        "AFF BLIND",
        "WATERBREATHING",
        "DETECT MAGIC",
        "WILD MAGIC"};

Sockets::Sockets(CharacterPtr ch, QString searchkey)
{
  for (auto &conn : dc_->connections_)
  {
    if (!conn)
      continue;

    if (ch->getLevel() < OVERSEER)
    {
      if (conn->character == nullptr)
        continue;
      if (conn->character->name().isEmpty())
        continue;
    }
    if (conn->character)
    {
      if (!CAN_SEE(ch, conn->character))
        continue;
      if (ch->getLevel() < conn->character->getLevel())
        continue;
      if ((conn->connected != Connection::states::PLAYING) && (ch->getLevel() < conn->character->getLevel()))
        continue;
    }

    if (!searchkey.isEmpty())
    {
      if (!conn->getPeerOriginalAddress().toString().contains(searchkey) && conn->character != nullptr && !conn->character->name().isEmpty() && QString(qPrintable(conn->character->name())).contains(searchkey, Qt::CaseInsensitive) == false)
      {
        continue;
      }
    }

    const QString name = conn->name();
    if (name.size() > longest_name_size_)
    {
      longest_name_size_ = name.size();
    }

    const QString IPstr = conn->getPeerFullAddressString();
    if (IPstr.size() > longest_IP_size_)
    {
      longest_IP_size_ = IPstr.size();
    }

    const QString state = constindex(conn->connected, DC::connected_states);
    if (state.size() > longest_connection_state_size_)
    {
      longest_connection_state_size_ = state.size();
    }

    const QString idle = QString::number(conn->idle_time / DC::PASSES_PER_SEC);
    if (idle.size() > longest_idle_size_)
    {
      longest_idle_size_ = idle.size();
    }

    IPs_[conn->getPeerAddress().toString()]++;
    connections_.push_back(conn);
  }
}

void Character::display_string_list(QStringList list)
{
  quint64 count{};
  for (const auto &entry : list)
  {
    send(u"%1"_s.arg(entry, 18));
    if (++count % 4 == 0)
      sendln();
  }
  sendln();
}

const QStringList Player::toggle_txt = {
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
    "nodupekeys"};

const QStringList Character::class_names = {
    "undefined",
    "mage",
    "cleric",
    "thief",
    "warrior",
    "antipaladin",
    "paladin",
    "barbarian",
    "monk",
    "ranger",
    "bard",
    "druid",
    "psionicist"};

const QStringList Character::race_names = {
    "undefined",
    "human",
    "elf",
    "dwarf",
    "hobbit",
    "pixie",
    "ogre",
    "gnome",
    "orc",
    "troll"};

const QStringList Character::song_names = {u"listsongs"_s, u"whistle sharp"_s, u"stop"_s, /* If you move stop, update do_sing */
                                           u"travelling march"_s, u"bountiful sonnet"_s, u"insane chant"_s, u"glitter dust"_s, u"synchronous chord"_s, u"healing melody"_s, u"sticky lullaby"_s, u"revealing staccato"_s,
                                           u"flight of the bumblebee"_s, u"jig of alacrity"_s, u"note of knowledge"_s, u"terrible clef"_s, u"soothing rememberance"_s, u"forgetful rhythm"_s, u"searching song"_s,
                                           u"vigilant siren"_s, u"astral chanty"_s, u"disarming limerick"_s, u"shattering resonance"_s, u"irresistable ditty"_s, u"fanatical fanfare"_s, u"dischordant dirge"_s,
                                           u"crushing crescendo"_s, u"hypnotic harmony"_s, u"mountain king's charge"_s, u"submariner's anthem"_s, u"summoning song"_s, u"\n"_s};

const QList<Toggle> Player::togglables = {
    {"brief", PLR_BRIEF_BIT, &Character::do_brief},
    {"compact", PLR_COMPACT_BIT, &Character::do_compact},
    {"beep", PLR_BEEP_BIT, &Character::do_beep_set},
    {"anonymous", PLR_ANONYMOUS_BIT, &Character::do_anonymous},
    {"ansi", PLR_ANSI_BIT, &Character::do_ansi},
    {"vt100", PLR_VT100_BIT, &Character::do_vt100},
    {"wimpy", PLR_WIMPY_BIT, &Character::do_wimpy},
    {"pager", PLR_PAGER_BIT, &Character::do_pager, UINT64_MAX, "$B$4off$R", "$B$2on$R"},
    {"bard-songs", PLR_BARD_SONG_BIT, &Character::do_bard_song_toggle, UINT64_MAX, "$B$2on$R (brief)", "$B$4off$R (verbose)"},
    {"auto-eat", PLR_AUTOEAT_BIT, &Character::do_autoeat},
    {"summonable", PLR_SUMMONABLE_BIT, &Character::do_summon_toggle},
    {"lfg", PLR_LFG_BIT, &Character::do_lfg_toggle},
    {"charmiejoin", PLR_CHARMIEJOIN_BIT, &Character::do_charmiejoin_toggle},
    {"notax", PLR_NOTAX_BIT, &Character::do_notax_toggle},
    {"guide", PLR_GUIDE_TOG_BIT, &Character::do_guide_toggle, PLR_GUIDE_BIT},
    {"news-up", PLR_NEWS_BIT, &Character::do_news_toggle},
    {"ascii", PLR_ASCII_BIT, &Character::do_ascii_toggle, UINT64_MAX, "$B$4off$R", "$B$2on$R"},
    {"damage", PLR_DAMAGE_BIT, &Character::do_damage_toggle},
    {"nodupekeys", PLR_NODUPEKEYS_BIT, &Character::do_nodupekeys_toggle}};

Toggle::Toggle(QString name, quint64 shift, ReturnValues (Character::*function)(QStringList arguments, cmd_t cmd), quint64 dependency_shift, QString on_message, QString off_message)
    : name_(name), valid_(true), shift_(shift), dependency_shift_(dependency_shift), value_(1U << shift), on_message_(on_message), off_message_(off_message), function_(function)
{
}

level_ Character::getLevel(void) const
{
  if (level_ > 110)
  {
    produce_coredump();
    dc_->logentry(u"Warning: getLevel returned %1."_s.arg(QString::number(level_)));
  }

  return level_;
}

void Character::setLevel(level_t level)
{
  level_ = level;

  if (level_ > 110)
  {
    produce_coredump();
    dc_->logentry(u"Warning: setLevel(%1)."_s.arg(QString::number(level_)));
  }
}

bool Character::isPlayer(void) const
{
  return getType() == Type::Player && player;
}

bool Character::isNonPlayer(void) const
{
  return (getType() == Type::NPC || getType() == Type::Object) && mobdata;
}

bool Character::isObjectProgram(void) const
{
  return getType() == Type::Object && mobdata && objdata;
}

auto Character::getType(void) const -> Type
{
  return type_;
}

void Character::setType(const Type type)
{
  type_ = type;
}

Room &Entity::room(void)
{
  return dc_->world[in_room];
}

move_t Character::move_limit(void)
{
  move_t max;

  if (isPlayer())
    /* HERE SHOULD BE CON CALCULATIONS INSTEAD */
    max = (max_move) + graf(age().year, 50, 70, 160, 120, 100, 40, 20);
  else
    max = max_move;

  /* Class/Level calculations */

  /* Skill/Spell calculations */

  return max;
}

QString Character::parse_prompt_variable(QString variable, PromptVariableType type)
{
  bool supports_color = isPlayer() && (isSet(GET_TOGGLES(this), Player::PLR_ANSI) || isSet(GET_TOGGLES(this), Player::PLR_VT100));
  bool use_color = false;
  auto target = this;
  targets_t target_is = targets::Self;

  QString color{}, value = {};

  const static QMap<QString, QString> legacy_to_modern{
      {"h", "hp"},
      {"H", "maxhp"},
      {"i", "colorhp"},
      {"m", "mana"},
      {"M", "maxmana"},
      {"n", "colormana"},
      {"v", "move"},
      {"V", "maxmove"},
      {"w", "colormove"},
      {"k", "ki"},
      {"K", "maxki"},
      {"l", "colorki"},
      {"a", "align"},
      {"A", "coloralign"},
      {"I", "hp%"},
      {"N", "mana%"},
      {"L", "ki%"},
      {"W", "move%"},
      {"x", "xp"},
      {"X", "xptnl"},
      {"g", "gold"},
      {"G", "platsofgold"},
      {"s", "sector"},
      {"d", "timeofday"},
      {"D", "weather"},
      {"R", "room"},
      {"O", "lastobj"},
      {"$", "platinum"},
      {"%", "%"},
      {"Z", "zone"},
      {"S", "lastmob"},
      {"z", "wizinvis"},
      {"c", "condition"},
      {"f", "target.condition"},
      {"t", "tank.condition"},
      {"y", "charmie.condition"},
      {"p", "tank.name"},
      {"q", "target.name"},
      {"C", "colorcondition"},
      {"F", "target.colorcondition"},
      {"T", "tank.colorcondition"},
      {"Y", "charmie.colorhp%"},
      {"P", "tank.colorname"},
      {"Q", "target.colorname"},
      {"b", "gmember1.name"},
      {"e", "gmember2.name"},
      {"j", "gmember3.name"},
      {"u", "gmember4.name"},
      {"B", "gmember1.colorhp"},
      {"E", "gmember2.colorhp"},
      {"J", "gmember3.colorhp"},
      {"U", "gmember4.colorhp"},
      {"0", "normal"},
      {"1", "red"},
      {"2", "green"},
      {"3", "yellow"},
      {"4", "blue"},
      {"5", "purple"},
      {"6", "cyan"},
      {"7", "grey"},
      {"8", "bold"},
      {"r", "cr"}};

  if (type == PromptVariableType::Legacy)
  {
    if (legacy_to_modern.contains(variable))
    {
      variable = legacy_to_modern.value(variable);
    }
  }

  QStringList arguments = variable.split('.');
  variable = arguments.value(0);
  if (variable == "charmie")
  {
    target = get_charmie(this);
    if (!target)
      return {};
    target_is = targets::Charmie;
    arguments.pop_front();
    variable = arguments.value(0);
  }
  else if (variable == "tank")
  {
    if (fighting && fighting->fighting)
    {
      target = fighting->fighting;
      target_is = targets::Tank;
    }
    else
      return {};
    arguments.pop_front();
    variable = arguments.value(0);
  }
  else if (variable == "fighting" || variable == "target" || variable == "opponent")
  {
    if (!fighting)
      return {};
    target = fighting;
    target_is = targets::Fighting;
    arguments.pop_front();
    variable = arguments.value(0);
  }
  else
  {
    QStringList gmembers = variable.split(QRegularExpression("(gmember|groupmember)"));
    if (gmembers.size() > 1)
    {
      bool ok = false;
      auto gmember_no = gmembers.last().toUInt(&ok);
      if (!ok || !gmember_no || gmember_no > getFollowers().size())
        return {};

      target = getFollowers().at(gmember_no - 1);
      if (!target)
        return {};
      target_is = targets::GrouptMember;

      arguments.pop_front();
      variable = arguments.value(0);
    }
  }

  if (variable.startsWith("color"))
  {
    arguments[0] = arguments[0].remove(QRegularExpression{"^color"});
    variable = arguments.value(0);
    use_color = supports_color;
  }

  // %h or %{hp} - Current hitpoints
  // %i          - Current hitpoints with color
  if (variable == "hp")
  {
    value = QString::number(target->getHP());
    if (use_color)
      color = calc_color(target->getHP(), GET_MAX_HIT(target));
  }
  // %H or %{maxhp} - Maximum hitpoints
  else if (variable == "maxhp")
  {
    value = QString::number(GET_MAX_HIT(target));
  }
  // %I or %{hp%} - Current hitpoints as a percent
  else if (variable == "hp%")
  {
    value = QString::number((target->getHP() * 100) / GET_MAX_HIT(target));
    if (use_color)
      color = calc_color(target->getHP(), GET_MAX_HIT(target));
  }
  // %m or %{mana} - Current mana
  // %n            - Current mana with color
  else if (variable == "mana")
  {
    value = QString::number(GET_MANA(target));
    if (use_color)
      color = calc_color(GET_MANA(target), GET_MAX_MANA(target));
  }
  else if (variable == "maxmana")
  {
    value = QString::number(GET_MAX_MANA(target));
  }
  else if (variable == "mana%")
  {
    value = QString::number((GET_MANA(target) * 100) / GET_MAX_MANA(target));
    if (use_color)
      color = calc_color(GET_MANA(target), GET_MAX_MANA(target));
  }
  else if (variable == "move")
  {
    value = QString::number(GET_MOVE(target));
    if (use_color)
      color = calc_color(GET_MOVE(target), GET_MAX_MOVE(target));
  }
  else if (variable == "maxmove")
  {
    value = QString::number(GET_MAX_MOVE(target));
  }
  else if (variable == "move%")
  {
    value = QString::number((GET_MOVE(target) * 100) / GET_MAX_MOVE(target));
    if (use_color)
      color = calc_color(GET_MOVE(target), GET_MAX_MOVE(target));
  }
  else if (variable == "ki")
  {
    value = QString::number(GET_KI(target));
    if (use_color)
      color = calc_color(GET_KI(target), GET_MAX_KI(target));
  }
  else if (variable == "maxki")
  {
    value = QString::number(GET_MAX_KI(target));
  }
  else if (variable == "ki%")
  {
    value = QString::number((GET_KI(target) * 100) / GET_MAX_KI(target));
    if (use_color)
      color = calc_color(GET_KI(target), GET_MAX_KI(target));
  }
  else if (variable == "xp")
    value = QString::number(target->exp);
  else if (variable == "xptnl")
    value = QString::number(exp_table[getLevel() + 1] - exp);
  else if (variable == "align" || variable == "alignment")
  {
    value = QString::number(GET_ALIGNMENT(this));
    if (use_color)
      color = calc_color_align(GET_ALIGNMENT(this));
  }
  else if (variable == "gold")
    value = QString::number(getGold());
  else if (variable == "platinum" || variable == "plats")
    value = QString::number(GET_PLATINUM(this));
  else if (variable == "platsofgold")
    value = QString::number(getGold() / 20000);
  else if (variable == "cond" || variable == "condition")
  {
    if (target)
    {
      if (target_is == targets::Charmie || target_is == targets::GrouptMember)
        return u"%1"_s.arg(calc_condition(target, use_color));

      if (fighting)
      {
        if (target_is == targets::Self)
          return u"<%1>"_s.arg(calc_condition(target, use_color));
        else if (target_is == targets::Fighting)
          return u"(%1)"_s.arg(calc_condition(target, use_color));
        else if (target_is == targets::Tank)
          return u"[%1]"_s.arg(calc_condition(target, use_color));
      }
    }
  }
  else if (variable == "normal" && supports_color)
    value = NTEXT;
  else if (variable == "red" && supports_color)
    value = RED;
  else if (variable == "green" && supports_color)
    value = GREEN;
  else if (variable == "yellow" && supports_color)
    value = YELLOW;
  else if (variable == "blue" && supports_color)
    value = BLUE;
  else if (variable == "purple" && supports_color)
    value = PURPLE;
  else if (variable == "cyan" && supports_color)
    value = CYAN;
  else if (variable == "grey" && supports_color)
    value = GREY;
  else if (variable == "bold" && supports_color)
    value = BOLD;
  else if (variable == "%")
    value = "%";
  else if (variable == "lastobj")
  {
    if (isPlayer())
      value = QString::number(player->last_obj_vnum);
  }
  else if (variable == "lastmob")
  {
    if (isPlayer())
      value = QString::number(player->last_mob_edit);
  }
  else if (variable == "weather")
  {
    extern QStringList sky_look;
    if (OUTSIDE(this))
      value = sky_look[weather_info.sky];
    else
      value = "indoors";
  }
  else if (variable == "name")
  {
    return target->calc_name(use_color);
  }
  else if (variable == "room")
  {
    value = QString::number(in_room);
    if (supports_color)
      color = GREEN;
  }
  else if (variable == "zone")
  {
    const auto rm = dc_->world[in_room];
    value = QString::number(rm.zone);
    if (supports_color)
      color = RED;
  }
  else if (variable == "sector")
  {
    if (dc_->rooms.contains(in_room))
      value = sector_types[dc_->world[in_room].sector_type];
  }
  else if (variable == "timeofday")
  {
    value = time_look[weather_info.sunlight];
  }
  else if (variable == "cr")
    value = u"\r\n"_s;

  if (value.isEmpty())
    return {};

  if (color.isEmpty())
    return value;

  return u"%1%2%3"_s.arg(color).arg(value).arg(NTEXT);
}

QString Character::createPrompt(void)
{
  QString source = {};
  if (isNonPlayer())
  {
    source = u"HP: %i/%H %f >"_s;
  }
  else
  {
    source = getPrompt();
  }

  // Searching for %{variable} like %{hp}
  // or %{variable1.variable2} like %{gmember1.hp}
  QRegularExpression re{"%{([a-zA-Z0-9.%]+)}"};
  auto match = re.match(source);
  while (match.hasMatch())
  {
    source = source.replace(match.capturedStart(), match.capturedLength(), parse_prompt_variable(match.captured(1)));
    match = re.match(source);
  }

  re = QRegularExpression{"%([a-np-zA-Z0-8%$])"};
  match = re.match(source);
  while (match.hasMatch())
  {
    source = source.replace(match.capturedStart(), match.capturedLength(), parse_prompt_variable(match.captured(1), PromptVariableType::Legacy));
    match = re.match(source);
  }

  if (!source.endsWith(' ') && !source.endsWith('\n') && !source.endsWith('\r'))
    source.append(" ");

  return source;
}

QString Character::calc_name(bool use_color)
{
  quint8 percent = {};
  QString namebuffer;

  if (getHP() == 0 || GET_MAX_HIT(this) == 0)
    percent = {};
  else
    percent = getHP() * 100 / GET_MAX_HIT(this);

  if (use_color)
  {
    if (percent >= 100)
      namebuffer = cond_colorcodes.value(0);
    else if (percent >= 90)
      namebuffer = cond_colorcodes.value(1);
    else if (percent >= 75)
      namebuffer = cond_colorcodes.value(2);
    else if (percent >= 50)
      namebuffer = cond_colorcodes.value(3);
    else if (percent >= 30)
      namebuffer = cond_colorcodes.value(4);
    else if (percent >= 15)
      namebuffer = cond_colorcodes.value(5);
    else if (percent >= 0)
      namebuffer = cond_colorcodes.value(6);
  }

  if (isPlayer())
    namebuffer += name();
  else if (!short_description().isEmpty())
    namebuffer += short_description();
  else
    namebuffer += u"unknown"_s;

  if (use_color)
    namebuffer += NTEXT;

  return namebuffer;
}

void ChannelMessage::set_wizinvis(const CharacterPtr sender)
{
  if (sender && sender->isPlayer())
  {
    wizinvis_ = sender->player->wizinvis;
  }
  else
  {
    wizinvis_ = {};
  }
}

void ChannelMessage::set_name(const CharacterPtr sender)
{
  if (sender)
  {
    sender_name_ = qPrintable(sender->shortdesc_or_name());
  }
  else
  {
    sender_name_ = u"Unknown"_s;
    logbug(u"channel_msg::set_name: sender is nullptr. type: %1 msg: %2"_s.arg(type_).arg(msg_));
  }
}

QString ChannelMessage::getMessage(CharacterPtr ch) const
{
  if (!ch)
    return {};

  QString prefix;
  switch (type_)
  {
  case DC::LogChannel::CHANNEL_TELL:
    prefix = "tell";
    break;
  case DC::LogChannel::CHANNEL_GOSSIP:
    prefix = "gossip";
    break;
  default:
    prefix = "unknown";
    break;
  }

  QTimeZone timezone = QTimeZone(ch->getSetting("timezone", "America/Chicago").toLatin1());
  QString timestamp = ch->getSetting(u"%1.history.timestamp"_s.arg(prefix));
  QString dateformat_str = ch->getSetting("dateformat", "ISODate");
  Qt::DateFormat dateformat = Qt::DateFormat(QMetaEnum::fromType<Qt::DateFormat>().keyToValue(qPrintable(dateformat_str)));

  if (timestamp == "1" || timestamp.startsWith('t', Qt::CaseInsensitive))
    return getMessage(ch->getLevel(), true, timezone, dateformat);
  else
    return getMessage(ch->getLevel(), false, timezone, dateformat);
}

ReturnValues Character::do_alias(QStringList arguments, cmd_t cmd)
{
  if (!player)
  {
    return ReturnValue::eFAILURE;
  }

  if (arguments.isEmpty())
  {
    if (player->aliases_.isEmpty())
    {
      sendln(u"No aliases defined."_s);
      return ReturnValue::eSUCCESS;
    }

    auto removed_count = player->aliases_.remove("");
    if (removed_count)
    {
      sendln(u"Removed an alias with an empty alias."_s);
    }

    quint64 x = {};
    sendln(u"Aliases:"_s);
    for (const auto [alias, command] : player->aliases_.asKeyValueRange())
    {
      sendln(u"%2=%3"_s.arg(alias).arg(command));
    }
    return ReturnValue::eSUCCESS;
  }

  QString arg1 = arguments.value(0).trimmed();
  QString arg2 = arguments.value(1).trimmed();

  // Alias assignment
  if (arg1.contains("=") || arg2.contains("="))
  {
    auto new_alias_arguments = arguments.join(' ').trimmed().split('=');
    auto alias = new_alias_arguments.value(0).trimmed().toLower();
    auto command = new_alias_arguments.value(1).trimmed();

    if (alias == "alias" || alias == "deleteall")
    {
      sendln(u"You cannot create a command alias named 'alias' or 'deleteall'."_s);
      return ReturnValue::eFAILURE;
    }

    if (alias.isEmpty())
    {
      sendln(u"You need to specify an alias."_s);
      return ReturnValue::eFAILURE;
    }

    if (command.isEmpty())
    {
      sendln(u"You need to specify a command for your alias."_s);
      return ReturnValue::eFAILURE;
    }

    if (player->aliases_.contains(alias) && player->aliases_[alias] == command)
    {
      sendln(u"Alias '%1' with command '%2' already set."_s.arg(alias).arg(command));
      return ReturnValue::eFAILURE;
    }
    else if (player->aliases_.contains(alias))
    {
      sendln(u"Alias '%1' with command '%2' replaced with '%3'."_s.arg(alias).arg(player->aliases_[alias]).arg(command));
    }
    else
    {
      sendln(u"Alias '%1' defined with command '%2'."_s.arg(alias).arg(command));
    }
    player->aliases_[alias] = command;
    save();
    return ReturnValue::eSUCCESS;
  }

  if (arg1 == "deleteall")
  {
    if (player->aliases_.isEmpty())
    {
      sendln(u"No aliases defined."_s);
      return ReturnValue::eFAILURE;
    }

    for (const auto [alias, command] : player->aliases_.asKeyValueRange())
    {
      sendln(u"Removed alias %2=%3"_s.arg(alias).arg(command));
    }

    player->aliases_.clear();
    save();
    return ReturnValue::eSUCCESS;
  }

  if (!player->aliases_.contains(arg1))
  {
    sendln(u"Alias '%1' not found to delete."_s.arg(arg1));
    return ReturnValue::eFAILURE;
  }

  player->aliases_.remove(arg1);
  sendln(u"Alias '%1' deleted."_s.arg(arg1));
  save();
  return ReturnValue::eFAILURE;
}

ReturnValues Character::do_pets(QStringList arguments, cmd_t cmd)
{
  QString arg1 = arguments.value(0);
  bool arg1_level_ok = false;
  auto arg1_level = arg1.toUInt(&arg1_level_ok);

  QString arg2 = arguments.value(1);
  bool arg2_level_ok = false;
  auto arg2_level = arg2.toUInt(&arg2_level_ok);

  extern qint32 top_of_mobt;
  QMultiMap<level_t, QString> results;

  for (vnum_t vnum = {}; (vnum <= dc_->mob_index_[top_of_mobt]->vnum()); ++vnum)
  {
    auto nr = real_mobile(vnum);
    if (nr < 0)
      continue;

    auto victim = (CharacterPtr)(dc_->mob_index_[nr]->item);
    if ((arg1_level_ok && victim->getLevel() < arg1_level) ||
        (arg2_level_ok && victim->getLevel() < arg2_level))
      continue;

    auto victim_qty = dc_->mob_index_[nr]->qty;
    bool include_bard = false;
    if (ISSET(victim->mobdata->actflags, ACT_BARDCHARM))
    {
      if (arg1.isEmpty() && GET_CLASS(this) == CLASS_BARD)
      {
        include_bard = true;
      }
      else if (!arg1.isEmpty() && u"bard"_s.startsWith(arg1, Qt::CaseInsensitive) || arg1 == u"all"_s)
      {
        include_bard = true;
      }
      if (include_bard)
        results.insert(victim->getLevel(), pet_info(victim, u"Bard"_s, victim_qty));
    }

    bool include_ranger = false;
    if (ISSET(victim->mobdata->actflags, ACT_CHARM) && !isSet(victim->immune, ISR_CHARM))
    {
      if (arg1.isEmpty() && GET_CLASS(this) == CLASS_RANGER)
      {
        include_ranger = true;
      }
      else if (!arg1.isEmpty() && u"ranger"_s.startsWith(arg1, Qt::CaseInsensitive) || arg1 == u"all"_s)
      {
        include_ranger = true;
      }
      if (include_ranger)
        results.insert(victim->getLevel(), pet_info(victim, u"Ranger"_s, victim_qty));
    }

    bool include_antipaladin = false;
    if (vnum >= 22389 && vnum <= 22398)
    {
      if (arg1.isEmpty() && GET_CLASS(this) == CLASS_ANTI_PAL)
      {
        include_antipaladin = true;
      }
      else if (!arg1.isEmpty() && (u"antipaladin"_s.startsWith(arg1, Qt::CaseInsensitive) || u"anti-paladin"_s.startsWith(arg1, Qt::CaseInsensitive) || u"anti_paladin"_s.startsWith(arg1, Qt::CaseInsensitive)) || arg1 == u"all"_s)
      {
        include_antipaladin = true;
      }
      if (include_antipaladin)
        results.insert(victim->getLevel(), pet_info(victim, u"AntiPal"_s, victim_qty));
    }

    bool include_cleric = false;
    if (vnum >= 22389 && vnum <= 22398)
    {
      if (arg1.isEmpty() && GET_CLASS(this) == CLASS_CLERIC)
      {
        include_cleric = true;
      }
      else if (!arg1.isEmpty() && u"cleric"_s.startsWith(arg1, Qt::CaseInsensitive) || arg1 == u"all"_s)
      {
        include_cleric = true;
      }
      if (include_cleric)
        results.insert(victim->getLevel(), pet_info(victim, u"Cleric"_s, victim_qty));
    }

    bool include_druid = false;
    if ((vnum >= 6 && vnum <= 7) || (vnum >= 88 && vnum <= 91))
    {
      if (arg1.isEmpty() && GET_CLASS(this) == CLASS_DRUID)
      {
        include_druid = true;
      }
      else if (!arg1.isEmpty() && u"druid"_s.startsWith(arg1, Qt::CaseInsensitive) || arg1 == u"all"_s)
      {
        include_druid = true;
      }
      if (include_druid)
        results.insert(victim->getLevel(), pet_info(victim, u"Druid"_s, victim_qty));
    }

    bool include_mage = false;
    if (vnum >= 4 && vnum <= 5)
    {
      if (arg1.isEmpty() && GET_CLASS(this) == CLASS_MAGE)
      {
        include_mage = true;
      }
      else if (!arg1.isEmpty() && u"mage"_s.startsWith(arg1, Qt::CaseInsensitive) || arg1 == u"all"_s)
      {
        include_mage = true;
      }
      if (include_mage)
        results.insert(victim->getLevel(), pet_info(victim, u"Mage"_s, victim_qty));
    }
  }

  if (results.isEmpty())
  {
    if (arg1.isEmpty())
      sendln(u"No charmable pets found for a %1."_s.arg(Character::classes_[c_class].name));
    else
      sendln(u"No charmable pets found for a %1."_s.arg(arg1));
    sendln(u"Type 'pets all' to see charmable pets for all classes."_s);
    sendln(u"Type 'pets bard' to see charmable pets for bards."_s);
    sendln(u"Type 'pets bard 50' or 'pets 50' to only see pets level 50 or above."_s);
    return ReturnValue::eSUCCESS;
  }
  sendln(u"$B$7LVL,ATK,HIT,DAM,   HP, -AC, dice, class, description$R"_s);
  for (const auto &line : results)
    sendln(line);
  sendln(u"$B$5*$R = in world now"_s);
  // loop though all npcs

  // search npcs affect of charmable
  // sort by class

  return ReturnValue();
}
auto Character::do_arena(QStringList arguments, cmd_t cmd) -> ReturnValue
{
  auto rufus = get_mob_room_vis(this, "rufus arena-keeper");
  if (!isImmortalPlayer() && !rufus)
  {
    sendln(u"You must be in the same room as Rufus the Arena-keeper to use this command."_s);
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0);
  if (arg1.isEmpty())
  {
    return do_arena_usage(arguments);
  }
  arguments.pop_front();

  if (arg1 == "info")
  {
    return do_arena_info(arguments);
  }
  else if (arg1 == "start")
  {
    return do_arena_start(arguments);
  }
  else if (arg1 == "join")
  {
    return do_arena_join(arguments);
  }
  else if (arg1 == "cancel")
  {
    return do_arena_cancel(arguments);
  }
  else
  {
    return do_arena_usage(arguments);
  }

  return ReturnValue::eSUCCESS;
}
ReturnValues Character::do_ban(QStringList arguments, cmd_t cmd)
{
  if (arguments.isEmpty())
  {
    if (dc_->bans_)
    {
      sendln(u"No sites are banned."_s);
      return ReturnValue::eSUCCESS;
    }

    sendln(u"%1  %2  %3  %4"_s.arg("Banned Site Name ", -15).arg("Ban Type", -8).arg("Banned On", -19).arg("Banned By", -16));
    sendln(u"%1  %2  %3  %4"_s.arg("-----------------------", -15).arg("---------------------------------", -8).arg("-------------------", -19).arg("---------------------------------", -16));

    QString buffer;
    for (const auto &ban : dc_->bans_.list())
    {
      sendln(u"%1  %2  %3  %4"_s.arg(ban.site(), -15).arg(Ban::ban_types.value(qsizetype(ban.type())), -8).arg(ban.date().toString(), -19).arg(ban.name(), -16));
    }
    return ReturnValue::eSUCCESS;
  }

  auto flag = arguments.value(0).toUpper();
  auto site = arguments.value(1);
  if (flag.isEmpty() || site.isEmpty())
  {
    sendln(u"Usage: ban {all | select | new} site_name"_s);
    return ReturnValue::eSUCCESS;
  }

  struct sockaddr_in sa{};
  if (inet_pton(AF_INET, qPrintable(site), &(sa.sin_addr)) == 0)
  {
    sendln(u"Invalid IP address."_s);
    return ReturnValue::eFAILURE;
  }

  if (flag != "SELECT" && flag != "ALL" && flag != "NEW")
  {
    sendln(u"Flag must be ALL, SELECT, or NEW."_s);
    return ReturnValue::eSUCCESS;
  }
  switch (dc_->bans_.is_banned(site))
  {
  case Ban::type_t::NOT:
    break;
  case Ban::type_t::NEW:
  case Ban::type_t::SELECT:
  case Ban::type_t::ALL:
    sendln(u"That site has already been banned -- unban it to change the ban type."_s);
    return ReturnValue::eSUCCESS;
    break;
  }

  Ban ban;
  ban.site(site);
  ban.name(name());
  ban.date(QDateTime::fromSecsSinceEpoch(time(0)));
  ban.type(flag);
  dc_->bans_.add(ban);

  loggod(u"1s has banned %2 for %3 players."_s.arg(name()).arg(site).arg(Ban::ban_types.value(qsizetype(ban.type()))));
  sendln(u"Site banned."_s);
  dc_->bans_.save();
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_unban(QStringList arguments, cmd_t cmd)
{
  auto site = arguments.value(0);
  if (site.isEmpty())
  {
    sendln(u"A site to unban might help."_s);
    return ReturnValue::eSUCCESS;
  }

  switch (dc_->bans_.is_banned(site))
  {
  case Ban::type_t::NOT:
    sendln(u"That site is not currently banned."_s);
    return ReturnValue::eSUCCESS;
    break;
  case Ban::type_t::NEW:
  case Ban::type_t::SELECT:
  case Ban::type_t::ALL:
    break;
  }

  dc_->bans_.remove(site);
  sendln(u"Site unbanned."_s);
  loggod(u"%1 removed the %2-player ban."_s.arg(name()).arg(site));
  dc_->bans_.save();

  return ReturnValue::eSUCCESS;
}

QString Character::createBlackjackPrompt(void)
{
  if (!isPlayer())
    return {};

  bool ascii = isSet(player->toggles, Player::PLR_ASCII);
  QString prompt;
  bool showColor = false;
  if (isSet(GET_TOGGLES(this), Player::PLR_ANSI) || isSet(GET_TOGGLES(this), Player::PLR_VT100))
  {
    showColor = true;
  }

  if (in_room < 21902 || in_room > 21905)
    if (in_room != 44)
      return {};
  auto obj = dc_->world[in_room]->contents_;
  for (; obj; obj = obj->next_content)
  {
    if (obj->table)
      break;
  }

  if (!obj || !obj->table->plr)
    return {};
  // Prompt-time
  qint32 plrsdone = {};
  QString buf;
  buf[0] = '\0';
  lineTwo[0] = '\0';
  lineTop[0] = '\0';
  CasinoPlayerPtr plr, pnext;
  for (plr = obj->table->plr; plr; plr = pnext)
  {
    pnext = plr->next;
    if (!dc_->verify(plr))
      continue;
    if (!plr->hand_data[0])
      continue;
    if (plr->ch == this)
    {
      QString buf2;
      buf2[0] = '\0';
      if (plr->table->cr == plr)
      {
        dc_strcat(buf2, "HIT STAY ");
        if (plr->hand_data[2] == 0)
          dc_strcat(buf2, "DOUBLE ");
      }
      if (canInsurance(plr))
        dc_strcat(buf2, "INSURANCE ");
      if (canSplit(plr))
        dc_strcat(buf2, "SPLIT ");
      if (buf2[0] != '\0')
      {
        prompt += "You can: ";
        prompt += showColor ? BOLD CYAN : "";
        prompt += buf2;
        prompt += showColor ? NTEXT : "";
        prompt += "\r\n";
      }
      if (hands(plr) > 1)
      {
        dc_sprintf(tempBuf, "%s, hand %d: ", qPrintable(plr->ch->name()), hand_number(plr));
        dc_sprintf(buf, "%s%s%s%s, hand %d%s: %s = %d   ", buf, showColor ? BOLD : "", plr == plr->table->cr && showColor ? GREEN : "", qPrintable(plr->ch->name()), hand_number(plr), showColor ? NTEXT : "", show_hand(plr->hand_data, 0, ascii, showColor), hand_strength(plr));
        padnext = hand_strength(plr) > 9 ? 8 : 7;
      }
      else
      {
        dc_sprintf(tempBuf, "%s: ", qPrintable(plr->ch->name()));
        dc_sprintf(buf, "%s%s%s%s%s: %s = %d   ", buf, showColor ? BOLD : "", plr == plr->table->cr && showColor ? GREEN : "", qPrintable(plr->ch->name()), showColor ? NTEXT : "", show_hand(plr->hand_data, 0, ascii, showColor), hand_strength(plr));
        padnext = hand_strength(plr) > 9 ? 8 : 7;
      }
    }
    //    }
    else
    {
      if (hands(plr) > 1)
      {
        dc_sprintf(tempBuf, "%s, hand %d: ", qPrintable(plr->ch->name()), hand_number(plr));
        dc_sprintf(buf, "%s%s%s, hand %d%s: %s ", buf, plr == plr->table->cr && showColor ? BOLD GREEN : "", qPrintable(plr->ch->name()), hand_number(plr), showColor ? NTEXT : "", show_hand(plr->hand_data, 0, ascii, showColor));
        padnext = 1;
      }
      else
      {
        dc_sprintf(tempBuf, "%s: ", qPrintable(plr->ch->name()));
        dc_sprintf(buf, "%s%s%s%s: %s ", buf, plr == plr->table->cr && showColor ? BOLD GREEN : "", qPrintable(plr->ch->name()), showColor ? NTEXT : "", show_hand(plr->hand_data, 0, ascii, showColor));
        padnext = 1;
      }
    }
    if (++plrsdone % 3 == 0)
    {
      if (buf[0] != '\0')
      {
        prompt += "\r\n";
        if (ascii)
        {
          prompt += lineTop;
          prompt += "\r\n";
        }

        prompt += buf;
        prompt += "\r\n";
        if (ascii)
        {
          prompt += lineTwo;
          prompt += "\r\n";
        }

        for (qint32 z = {}; lineTop[z]; z++)
          if (lineTop[z] == ',')
            lineTop[z] = '\'';
        if (ascii)
        {
          prompt += lineTop;
          prompt += "\r\n";
        }
        buf[0] = '\0';
        lineTwo[0] = '\0';
        lineTop[0] = '\0';
        padnext = {};
      }
    }
  }
  if (obj->table->hand_data[0])
  {
    dc_sprintf(tempBuf, "Dealer: ");
    dc_sprintf(buf, "%s%sDealer%s: %s", buf, showColor ? BOLD YELLOW : "", showColor ? NTEXT : "", obj->table->state < 2 ? show_hand(obj->table->hand_data, 1, ascii, showColor) : show_hand(obj->table->hand_data, 0, ascii, showColor));
    dc_sprintf(buf, "%s\r\n", buf);
  }
  // fixPadding(&buf[0]);
  if (buf[0] != '\0')
  {
    prompt += "\r\n";
    if (ascii)
    {
      prompt += lineTop;
      prompt += "\r\n";
    }
    prompt += buf;
    if (ascii)
    {
      prompt += lineTwo;
      prompt += "\r\n";
    }
    for (qint32 z = {}; lineTop[z]; z++)
      if (lineTop[z] == ',')
        lineTop[z] = '\'';
      else if (lineTop[z] == '_')
        lineTop[z] = '-';
    if (ascii)
    {
      prompt += lineTop;
      prompt += "\r\n";
    }
  }
  return prompt;
}

void Character::sendBlackjackPrompt(void)
{
  send(createBlackjackPrompt());
}
ReturnValues Character::do_auction(QStringList arguments, cmd_t cmd)
{
  ConnectionPtr i = {};
  bool silence = false;

  if (isSet(dc_->world[in_room]->room_flags_, QUIET))
  {
    send(u"SHHHHHH!! Can't you see people are trying to read?\r\n"_s);
    return ReturnValue::eSUCCESS;
  }

  for (const auto &tmp_obj : dc_->world[in_room]->contents_)
    if (tmp_obj && dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      send(u"The magical silence prevents you from speaking!\r\n"_s);
      return ReturnValue::eFAILURE;
    }

  if (isNonPlayer() && master)
  {
    do_say(this, "That's okay, I'll let you do all the auctioning, master.");
    return ReturnValue::eSUCCESS;
  }

  if (isPlayer())
  {
    if (!isNonPlayer() && isSet(player->punish, PUNISH_SILENCED))
    {
      send_to_char("You must have somehow offended the gods, for "
                   "you find yourself unable to!\r\n",
                   this);
      return ReturnValue::eSUCCESS;
    }
    if (!(isSet(misc, DC::LogChannel::CHANNEL_AUCTION)))
    {
      sendln(u"You told yourself not to AUCTION!!"_s);
      return ReturnValue::eSUCCESS;
    }
    if (level_ < 3)
    {
      sendln(u"You must be at least 3rd level to auction."_s);
      return ReturnValue::eSUCCESS;
    }
  }

  if (arguments.isEmpty())
  {
    QQueue<QString> tmp = auction_history;
    sendln(u"Here are the last 10 auctions:"_s);
    while (!tmp.isEmpty())
    {
      act_to_victim(tmp.front(), this, 0, this, 0);
      tmp.pop_front();
    }
  }
  else
  {
    decrementMove(5);

    QString buf1;
    if (isNonPlayer())
    {
      buf1 = u"$6$B%1 auctions '%2'$R"_s.arg(qPrintable(shortdesc_or_name())).arg(arguments.join(' '));
    }
    else
    {
      buf1 = u"$6$B%1 auctions '%2'$R"_s.arg(qPrintable(name())).arg(arguments.join(' '));
    }

    QString buf2 = u"$6$BYou auction '%1'$R"_s.arg(arguments.join(' '));
    act_to_character(buf2, this, 0, 0, 0);

    auction_history.push_back(buf1);
    if (auction_history.size() > 10)
      auction_history.pop_front();

    for (auto &conn : dc_->connections_)
      if (conn->character != this && !conn->connected &&
          (isSet(conn->character->misc, DC::LogChannel::CHANNEL_AUCTION)) &&
          !is_ignoring(conn->character, this))
      {
        for (const auto &tmp_obj : dc_->world[conn->character->in_room]->contents_)
          if (dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
          {
            silence = true;
            break;
          }
        if (!silence)
          act_to_victim(buf1, this, 0, conn->character, 0);
      }
  }
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_tell(QStringList arguments, cmd_t cmd)
{
  CharacterPtr vict = {};
  QString name = {}, message = {}, buf = {}, log_buf = {};
  ObjectPtr tmp_obj = {};

  if (!isNonPlayer() && isSet(player->punish, PUNISH_NOTELL))
  {
    sendln(u"Your message didn't get through!!"_s);
    return ReturnValue::eSUCCESS;
  }

  for (tmp_obj = dc_->world[in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
    if (dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
    {
      sendln(u"The magical silence prevents you from speaking!"_s);
      return ReturnValue::eFAILURE;
    }

  if (!isNonPlayer() && !isSet(misc, DC::LogChannel::CHANNEL_TELL))
  {
    sendln(u"You have tell channeled off!!"_s);
    return ReturnValue::eSUCCESS;
  }

  name = arguments.value(0);
  if (!arguments.isEmpty())
  {
    arguments.pop_front();
  }
  message = arguments.join(' ');

  if (name.isEmpty() || message.isEmpty())
  {
    if (player->tell_history.isEmpty())
    {
      sendln(u"You have not sent or recieved any tell messages."_s);
      return ReturnValue::eSUCCESS;
    }

    if (player->tell_history.isEmpty())
    {
      sendln(u"There have been no tell messages."_s);
      return ReturnValue::eSUCCESS;
    }
    else if (player->tell_history.count() == 1)
    {
      sendln(u"Here is the only tell message:"_s);
    }
    else
    {
      sendln(u"Here are the last %1 tell messages:"_s.arg(player->tell_history.count()));
    }
    for (const auto &c : player->tell_history)
    {
      sendln(c.getMessage(this));
    }

    return ReturnValue::eSUCCESS;
  }

  if (cmd == cmd_t::TELL_REPLY)
  {
    if (!(vict = get_active_pc(name)))
    {
      sendln(u"They seem to have left!"_s);
      return ReturnValue::eSUCCESS;
    }
    cmd = cmd_t::DEFAULT;
  }
  else if (!(vict = get_active_pc_vis(name)))
  {
    vict = get_pc_vis(this, name);
    if ((vict != nullptr) && vict->getLevel() >= IMMORTAL)
    {
      sendln(u"That person is busy right now."_s);
      sendln(u"Your message has been saved."_s);

      buf = fmt::format("$2$B{} told you, '{}'$R\r\n", PERS(this, vict), message.toStdString()).c_str();
      record_msg(buf, vict);

      buf = fmt::format("$2$B{} told you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou told {}, '{}'$R", qPrintable(vict->shortdesc_or_name()), message.toStdString()).c_str();
      tell_history(this, buf);
    }
    else
    {
      sendln(u"No-one by that name here."_s);
    }

    return ReturnValue::eSUCCESS;
  }

  // vict guarantted to be a PC
  // Re: Last comment. Switched immortals crash this.

  if (vict->isPlayer() && !isSet(vict->misc, DC::LogChannel::CHANNEL_TELL) && level_ <= DC::MAX_MORTAL_LEVEL)
  {
    sendln(u"The person is ignoring all tells right now."_s);
    return ReturnValue::eSUCCESS;
  }
  else if (vict->isPlayer() && !isSet(vict->misc, DC::LogChannel::CHANNEL_TELL))
  {
    // Immortal sent a tell to a player with NOTELL.  Allow the tell butnotify the imm.
    sendln(u"That player has tell channeled off btw..."_s);
  }
  if (this == vict)
    sendln(u"You try to tell yourself something."_s);
  else if ((GET_POS(vict) == position_t::SLEEPING || isSet(dc_->world[vict->in_room]->room_flags_, QUIET)) && level_ < IMMORTAL)
    act_to_character("Sorry, $E cannot hear you.", this, 0, vict, STAYHIDE);
  else
  {
    for (tmp_obj = dc_->world[vict->in_room]->contents_; tmp_obj; tmp_obj = tmp_obj->next_content)
      if (dc_->obj_index_[tmp_obj->item_number]->vnum() == SILENCE_OBJ_NUMBER)
      {
        act_to_character("$E cannot hear you right now.", this, 0, vict, STAYHIDE);
        return ReturnValue::eSUCCESS;
      }
    if (is_ignoring(vict, this))
    {
      send(u"%s is ignoring you right now.\r\n"_s.arg(qPrintable(vict->shortdesc_or_name())));
      return ReturnValue::eSUCCESS;
    }
    if (is_busy(vict) && level_ >= OVERSEER)
    {
      if (vict->isNonPlayer())
      {
        buf = fmt::format("{} tells you, '{}'", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("{} tells you, '{}'{}", PERS(this, vict), message.toStdString(), isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();

        if (isPlayer() && vict->isPlayer())
        {
          vict->player->last_tell = qPrintable(name());
        }
      }

      ansi_color(GREEN, vict);
      ansi_color(BOLD, vict);
      send_to_char_regardless(buf, vict);
      ansi_color(NTEXT, vict);

      buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();

      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou tell {}, '{}'$R", PERS(vict, this), message.toStdString()).c_str();
      send(buf);
    }
    else if (!is_busy(vict) && GET_POS(vict) > position_t::SLEEPING)
    {
      if (vict->isNonPlayer())
      {
        buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("$2$B{} tells you, '{}'$R{}", PERS(this, vict), message.toStdString(), isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();
        if (isPlayer() && vict->isPlayer())
          vict->player->last_tell = qPrintable(name());
      }
      act_to_character(buf, vict, 0, 0, STAYHIDE);

      buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou tell {}, '{}'$R", PERS(vict, this), message.toStdString()).c_str();
      act_return ar = act_to_character(buf, this, 0, 0, STAYHIDE);
      tell_history(this, ar.str);

      // Log what I told a logged player under their name
      if (!vict->isNonPlayer() && isSet(vict->player->punish, PUNISH_LOG) && isPlayer())
      {
        dc_->logentry(u"Log %1: %2 told them: %3"_s.arg(qPrintable(vict->name())).arg(qPrintable(name())).arg(message), IMPLEMENTER, DC::LogChannel::LOG_PLAYER, vict);
      }
    }
    else if (!is_busy(vict) && GET_POS(vict) == position_t::SLEEPING &&
             level_ >= SERAPH)
    {
      vict->sendln(u"A heavenly power intrudes on your subconcious dreaming..."_s);
      if (vict->isNonPlayer())
      {
        buf = fmt::format("{} tells you, '{}'", PERS(this, vict), message.toStdString()).c_str();
      }
      else
      {
        buf = fmt::format("{} tells you, '{}'{}", PERS(this, vict), message.toStdString(), isSet(vict->player->toggles, Player::PLR_BEEP) ? '\a' : '\0').c_str();

        if (isPlayer() && vict->isPlayer())
          vict->player->last_tell = qPrintable(name());
      }
      ansi_color(GREEN, vict);
      ansi_color(BOLD, vict);
      send_to_char_regardless(buf, vict);
      ansi_color(NTEXT, vict);

      buf = fmt::format("$2$B{} tells you, '{}'$R", PERS(this, vict), message.toStdString()).c_str();
      vict->tell_history(this, buf);

      buf = fmt::format("$2$BYou tell {}, '{}'$R", PERS(vict, this), message.toStdString()).c_str();
      act_return ar = act_to_character(buf, this, 0, 0, STAYHIDE);
      tell_history(this, ar.str);

      sendln(u"They were sleeping btw..."_s);
      // Log what I told a logged player under their name
      if (!vict->isNonPlayer() && isSet(vict->player->punish, PUNISH_LOG) && isPlayer())
      {
        dc_->logentry(u"Log %1: %2 told them: %3"_s.arg(qPrintable(vict->name())).arg(qPrintable(name())).arg(message), IMPLEMENTER, DC::LogChannel::LOG_PLAYER, vict);
      }
    }
    else
    {
      buf = fmt::format("$2$B%s can't hear anything right now.$R", qPrintable(vict->shortdesc_or_name())).c_str();
      act_to_character(buf, this, 0, 0, STAYHIDE);
    }
  }
  return ReturnValue::eSUCCESS;
}
void Character::tell_history(CharacterPtr ch, QString message)
{
  if (!isPlayer())
  {
    return;
  }

  ChannelMessage cm(ch, DC::LogChannel::CHANNEL_TELL, message);

  player->tell_history.enqueue(cm);
  if (player->tell_history.size() > 1000)
  {
    player->tell_history.dequeue();
  }
}

void Character::gtell_history(CharacterPtr ch, QString message)
{
  if (player == nullptr)
  {
    return;
  }

  communication c(ch, message);

  player->gtell_history.push_back(c);
  if (player->gtell_history.size() > 10)
  {
    player->gtell_history.pop_front();
  }
}

ReturnValues Character::do_rage(QStringList arguments, cmd_t cmd)
{
  if (getHP() == 1)
  {
    sendln(u"You are feeling too weak right now to work yourself up into a rage."_s);
    return ReturnValue::eFAILURE;
  }

  if (!canPerform(SKILL_RAGE, "You should learn the skill before you try doing any raging in this machine...\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  QString name = arguments.value(0);

  CharacterPtr victim = get_char_room_vis(name);
  if (!victim)
  {
    if (fighting)
    {
      victim = fighting;
    }
    else
    {
      sendln(u"Who do you want to rage on?"_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (in_room != victim->in_room)
  {
    sendln(u"That person seems to have left."_s);
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln(u"Aren't we funny today..."_s);
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(this) || !can_be_attacked(this, victim))
    return ReturnValue::eFAILURE;

  if (!charge_moves(SKILL_RAGE))
    return ReturnValue::eSUCCESS;

  ReturnValues retval = {};
  if (!skill_success(victim, SKILL_RAGE))
  {
    act("You start advancing towards $N, but trip over your own feet!",
        this, 0, victim, TO_CHAR, 0);
    act("$n starts advancing towards $N, but trips over $s own feet!",
        this, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act_to_victim("$n starts advancing toward you, but trips over $s own feet!", this, 0, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      return retval;
    }

    setSitting();
    SET_BIT(combat, COMBAT_BASH1);
  }
  else
  {
    act("You advance confidently towards $N, and fly into a rage!",
        this, 0, victim, TO_CHAR, 0);
    act("$n advances confidently towards $N, and flies into a rage!",
        this, 0, victim, TO_ROOM, NOTVICT);
    act_return ar = act_to_victim("$n advances confidently towards you, and flies into a rage!", this, 0, victim, 0);
    retval = ar.retval;
    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      return retval;
    }

    SET_BIT(combat, COMBAT_RAGE1);
  }

  WAIT_STATE(this, DC::PULSE_VIOLENCE * 3);

  if (!fighting)
    return attack(this, victim, TYPE_UNDEFINED);

  // chance of bonus round at high level of skill
  if (has_skill(SKILL_RAGE) > 75 && !number(0, 9))
    return attack(this, victim, TYPE_UNDEFINED);

  return ReturnValue::eSUCCESS;
}
ReturnValues Character::do_track(QStringList arguments, cmd_t cmd)
{
  qint32 x, y;
  ReturnValues retval, how_deep, learned;
  QString buf;
  QString race;
  QString sex;
  QString condition;
  QString weight;
  CharacterPtr quarry;
  CharacterPtr tmp_ch;
  QString victim = arguments.value(0);

  learned = how_deep = ((has_skill(SKILL_TRACK) / 10) + 1);

  if (getLevel() >= IMMORTAL)
    how_deep = 50;

  quarry = get_char_room_vis(victim);

  if (!hunting.isEmpty())
  {
    if (get_char_room_vis(hunting))
    {
      ansi_color(RED, this);
      ansi_color(BOLD, this);
      sendln(u"You have found your target!"_s);
      ansi_color(NTEXT, this);

      //      remove_memory(this, 't');
      return ReturnValue::eSUCCESS;
    }
  }

  else if (quarry)
  {
    sendln(u"There's one right here ;)"_s);
    //  remove_memory(this, 't');
    return ReturnValue::eSUCCESS;
  }

  if (!victim.isEmpty() && isPlayer() && GET_CLASS(this) != CLASS_RANGER && GET_CLASS(this) != CLASS_DRUID && getLevel() < ANGEL)
  {
    sendln(u"Only a ranger could track someone by name."_s);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(SKILL_TRACK))
    return ReturnValue::eSUCCESS;

  act_to_room("$n walks about slowly, searching for signs of $s quarry", this, 0, 0, INVIS_NULL);
  sendln(u"You search for signs of your quarry...\r\n"_s);

  if (learned)
    skill_increase_check(SKILL_TRACK, learned, SKILL_INCREASE_MEDIUM);

  // TODO - once we're sure that act_mob is properly checking for this,
  // and that it isn't call from anywhere else, we can probably remove it.
  // That way possessing imms can track.
  if (isNonPlayer() && ISSET(mobdata->actflags, ACT_STUPID))
  {
    sendln(u"Being stupid, you cannot find any.."_s);
    return ReturnValue::eFAILURE;
  }

  if (dc_->world[in_room].tracks_.size() < 1)
  {
    if (!hunting.isEmpty())
    {
      ansi_color(RED, this);
      ansi_color(BOLD, this);
      sendln(u"You have lost the trail."_s);
      ansi_color(NTEXT, this);
      // remove_memory(this, 't');
    }
    else
      sendln(u"There are no distinct scents here."_s);
    return ReturnValue::eFAILURE;
  }

  if (isNonPlayer())
    how_deep = 10;

  if (!victim.isEmpty())
  {
    for (x = 1; x <= how_deep; x++)
    {

      if ((x > dc_->world[in_room].tracks_.size()) || !(pScent = dc_->world[in_room].TrackItem(x)))
      {
        if (!hunting.isEmpty())
        {
          ansi_color(RED, this);
          ansi_color(BOLD, this);
          sendln(u"You have lost the trail."_s);
          ansi_color(NTEXT, this);
        }
        else
          sendln(u"You can't find any traces of such a scent."_s);
        //         remove_memory(this, 't');
        if (isNonPlayer())
          swap_hate_memory();
        return ReturnValue::eFAILURE;
      }

      if (isexact(victim, pScent->trackee))
      {
        y = pScent->direction;
        add_memory(pScent->trackee, 't');
        ansi_color(RED, this);
        ansi_color(BOLD, this);
        send(u"You sense traces of your quarry to the %s.\r\n"_s.arg(dirs[y]));
        ansi_color(NTEXT, this);

        if (isNonPlayer())
        {
          // temp disable tracking mobs into town
          if (dc_->zones_.value(dc_->world[EXIT(this, y)->to_room].zone).isTown() == false && !isSet(dc_->world[EXIT(this, y)->to_room]->room_flags_, NO_TRACK))
          {
            mobdata->last_direction = y;
            auto cmd_dir = getCommandFromDirection(y);
            if (cmd_dir)
            {
              retval = do_move(this, u""_s, *cmd_dir);
              if (isSet(retval, ReturnValue::eCH_DIED))
                return retval;
            }
          }

          if (hunting.isEmpty())
            return ReturnValue::eFAILURE;

          // Here's the deal: if the mob can't see the character in
          // the room, but the character IS in the room, then the
          // mob can't see the character and we need to stop tracking.
          // It does, however, leave the mob open to be taken apart
          // by, say, a thief.  I'll let he who wrote it fix that.
          // Morc 28 July 96

          if ((tmp_ch = get_char(hunting)) == 0)
            return ReturnValue::eFAILURE;
          if (!(get_char_room_vis(hunting)))
          {
            if (tmp_ch->in_room == in_room)
            {
              // The mob can't see him
              act("$n says 'Damn, must have lost $M!'", this, 0, tmp_ch,
                  TO_ROOM, 0);
              //                    remove_memory(this, 't');
            }
            return ReturnValue::eFAILURE;
          }

          if (!isSet(dc_->world[in_room]->room_flags_, SAFE))
          {
            act("$n screams 'YOU CAN RUN, BUT YOU CAN'T HIDE!'",
                this, 0, 0, TO_ROOM, 0);
            retval = ReturnValue::eSUCCESS;
            if (tmp_ch)
            {
              retval = mprog_attack_trigger(this, tmp_ch);
            }
            if (SOMEONE_DIED(retval) || fighting || hunting.isEmpty())
              return retval;
            else
              return do_hit(hunting.split(' '));
          }
          else
            act("$n says 'You can't stay here forever.'",
                this, 0, 0, TO_ROOM, 0);
        } // if IS_NPC

        return ReturnValue::eSUCCESS;
      } // if isname
    } // for

    if (!hunting.isEmpty())
    {
      ansi_color(RED, this);
      ansi_color(BOLD, this);
      sendln(u"You have lost the trail."_s);
      ansi_color(NTEXT, this);
    }
    else
      sendln(u"You can't find any traces of such a scent."_s);

    //    remove_memory(this, 't');
    return ReturnValue::eFAILURE;
  } // if victim

  for (x = 1; x <= how_deep; x++)
  {
    if ((x > dc_->world[in_room].tracks_.size()) || !(pScent = dc_->world[in_room].TrackItem(x)))
    {
      if (x == 1)
        sendln(u"There are no distinct smells here."_s);
      break;
    }

    y = pScent->direction;

    if (pScent->weight < 50)
      dc_strcpy(weight, " small,");
    else if (pScent->weight <= 201)
      dc_strcpy(weight, " medium-sized,");
    else if (pScent->weight < 350)
      dc_strcpy(weight, " big,");
    else if (pScent->weight < 500)
      dc_strcpy(weight, " large,");
    else if (pScent->weight < 800)
      dc_strcpy(weight, " huge,");
    else if (pScent->weight < 1500)
      dc_strcpy(weight, " very large,");
    else
      dc_strcpy(weight, " gigantic,");

    if (pScent->condition < 10)
      dc_strcpy(condition, " severely injured,");
    else if (pScent->condition < 25)
      dc_strcpy(condition, " badly wounded,");
    else if (pScent->condition < 40)
      dc_strcpy(condition, " injured,");
    else if (pScent->condition < 60)
      dc_strcpy(condition, " wounded,");
    else if (pScent->condition < 80)
      dc_strcpy(condition, " slightly injured,");
    else
      dc_strcpy(condition, " healthy,");

    if (pScent->sex == 1)
      dc_strcpy(sex, " male");
    else if (pScent->sex == 2)
      dc_strcpy(sex, " female");
    else
      dc_strcpy(sex, "");

    if (pScent->race >= 1 && pScent->race <= 30)
      dc_sprintf(race, " %s", races[pScent->race].singular_name);
    else
      dc_strcpy(race, " non-descript race");

    if (ch->dc_->number(1, 101) >= (how_deep * 10))
      dc_strcpy(weight, "");
    if (ch->dc_->number(1, 101) >= (how_deep * 10))
      dc_strcpy(race, " non-descript race");
    if (ch->dc_->number(1, 101) >= (how_deep * 10))
      dc_strcpy(condition, "");
    if (ch->dc_->number(1, 101) >= (how_deep * 10))
      dc_strcpy(sex, "");

    if (x == 1)
      sendln(u"Freshest scents first..."_s);

    dc_sprintf(buf, "The scent of a%s%s%s%s leads %s.\r\n",
               weight,
               condition,
               sex,
               race,
               dirs[y]);
    send(buf);
  }
  return ReturnValue::eSUCCESS;
}
ReturnValues Character::do_ambush(QStringList arguments, cmd_t cmd)
{
  if (!canPerform(SKILL_AMBUSH))
  {
    sendln(u"You don't know how to ambush people!"_s);
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0);

  if (arg1.isEmpty())
  {
    sendln(u"You will ambush %1 on sight."_s.arg(ambush.isEmpty() ? "no one" : ambush));
    return ReturnValue::eSUCCESS;
  }

  if (ambush == arg1)
  {
    ambush.clear();
    sendln(u"You will no longer ambush %1 on sight."_s.arg(arg1));
    return ReturnValue::eSUCCESS;
  }

  ambush = arg1;
  sendln(u"You will now ambush %1 on sight."_s.arg(arg1));
  return ReturnValue::eSUCCESS;
}
ReturnValues Character::do_backstab(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim;

  qint32 was_in = {};
  ReturnValues retval;

  QString name = arguments.value(0);

  if (!has_skill(SKILL_BACKSTAB) && !isNonPlayer())
  {
    sendln(u"You don't know how to backstab people!"_s);
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_char_room_vis(name)))
  {
    sendln(u"Backstab whom?"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln(u"How can you sneak up on yourself?"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_HUGE))
  {
    sendln(u"You cannot backstab someone that HUGE!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_SWARM))
  {
    sendln(u"You cannot target just one to backstab!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && ISSET(victim->mobdata->actflags, ACT_TINY))
  {
    sendln(u"You cannot target someone that tiny to backstab!"_s);
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_ALERT))
  {
    act_to_character("$E is too alert and nervous looking; you are unable to sneak behind!", this, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(SKILL_BACKSTAB))
    return ReturnValue::eSUCCESS;

  qint32 min_hp = (qint32)(GET_MAX_HIT(this) / 5);
  min_hp = MIN(min_hp, 25);

  if (getHP() < min_hp)
  {
    sendln(u"You are feeling too weak right now to attempt such a bold maneuver."_s);
    return ReturnValue::eFAILURE;
  }

  if (!equipment[WEAR_WIELD])
  {
    sendln(u"You need to wield a weapon to make it a success."_s);
    return ReturnValue::eFAILURE;
  }

  if (equipment[WEAR_WIELD]->flags_.value[3] != 11 && equipment[WEAR_WIELD]->flags_.value[3] != 9)
  {
    sendln(u"You can't stab without a stabbing weapon..."_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->fighting)
  {
    sendln(u"You can't backstab a fighting person, they are too alert!"_s);
    return ReturnValue::eFAILURE;
  }

  // Check the killer/victim
  if ((getLevel() < G_POWER) || isNonPlayer())
  {
    if (!can_attack(this) || !can_be_attacked(this, victim))
      return ReturnValue::eFAILURE;
  }

  qint32 itemp = dc_->number(1, 100);
  if (isPlayer() && victim->isPlayer())
  {
    if (victim->getLevel() > getLevel())
      itemp = {}; // not gonna happen
    else if (GET_MAX_HIT(victim) > GET_MAX_HIT(this))
    {
      if (GET_MAX_HIT(victim) * 0.85 > GET_MAX_HIT(this))
        itemp--;
      if (GET_MAX_HIT(victim) * 0.70 > GET_MAX_HIT(this))
        itemp--;
      if (GET_MAX_HIT(victim) * 0.55 > GET_MAX_HIT(this))
        itemp--;
      if (GET_MAX_HIT(victim) * 0.40 > GET_MAX_HIT(this))
        itemp--;
    }
  }

  // record the room I'm in.  Used to make sure a dual can go off.
  was_in = in_room;

  // Will this be a single or dual backstab this round?
  bool perform_dual_backstab = false;
  if ((((isPlayer() && GET_CLASS(this) == CLASS_THIEF && has_skill(SKILL_DUAL_BACKSTAB)) || getLevel() >= ARCHANGEL) || (isNonPlayer() && getLevel() > 70)) && (equipment[WEAR_SECOND_WIELD]) && ((equipment[WEAR_SECOND_WIELD]->flags_.value[3] == 11) || (equipment[WEAR_SECOND_WIELD]->flags_.value[3] == 9)) && (cmd != cmd_t::SBS))
  {
    if (skill_success(victim, SKILL_DUAL_BACKSTAB) || isNonPlayer())
    {
      perform_dual_backstab = true;
    }
  }

  WAIT_STATE(this, DC::PULSE_VIOLENCE * 1);

  // failure
  if (AWAKE(victim) && !skill_success(victim, SKILL_BACKSTAB))
  {
    // If this is stab 1 of 2 for a dual backstab, we dont want people autojoining on the first stab
    if (perform_dual_backstab && isPlayer())
    {
      player->unjoinable = true;
      retval = damage(this, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB);
      player->unjoinable = false;
    }
    else
    {
      retval = damage(this, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB);
    }
  }
  // success
  else if (!victim->isImmortalPlayer() &&
           victim->getLevel() <= (getLevel() + 19) &&
           (isImmortalPlayer() || itemp > 95 || (victim->isPlayer() && isSet(victim->player->punish, PUNISH_UNLUCKY))) &&
           ((equipment[WEAR_WIELD]->flags_.value[3] == 11 && !isSet(victim->immune, ISR_PIERCE)) || (equipment[WEAR_WIELD]->flags_.value[3] == 9 && !isSet(victim->immune, ISR_STING))))
  {
    act("$N crumples to the ground, $S body still quivering from "
        "$n's brutal assassination.",
        this, 0, victim, TO_ROOM, NOTVICT);
    act("You feel $n's blade slip into your heart, and all goes black.",
        this, 0, victim, TO_VICT, 0);
    act("BINGO! You brutally assassinate $N, and $S body crumples "
        "before you.",
        this, 0, victim, TO_CHAR, 0);
    return damage(this, victim, 9999999, TYPE_UNDEFINED, SKILL_BACKSTAB);
  }
  else
  {
    // If this is stab 1 of 2 for a dual backstab, we dont want people autojoining on the first stab
    if (perform_dual_backstab && isPlayer())
    {
      player->unjoinable = true;
      retval = attack(this, victim, SKILL_BACKSTAB, WEAR_WIELD);
      player->unjoinable = false;
    }
    else
    {
      retval = attack(this, victim, SKILL_BACKSTAB, WEAR_WIELD);
    }
  }

  if ((retval & ReturnValue::eVICT_DIED) && !(retval & ReturnValue::eCH_DIED))
  {
    return retval;
  }

  if (retval & ReturnValue::eCH_DIED)
    return retval;

  if (retval & ReturnValue::eVICT_DIED)
  {
    return retval;
  }

  if (!charExists(victim)) // heh
  {
    return ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED;
  }

  // If we're intended to have a dual backstab AND we still can
  if (perform_dual_backstab == true && charge_moves(SKILL_BACKSTAB) && GET_POS(victim) != position_t::DEAD && victim->in_room != INVALID_ROOM)
  {
    if (was_in == in_room)
    {
      if (AWAKE(victim) && !skill_success(victim, SKILL_BACKSTAB))
      {
        retval = damage(this, victim, 0, TYPE_UNDEFINED, SKILL_BACKSTAB, WEAR_SECOND_WIELD);
      }
      else
      {
        retval = attack(this, victim, SKILL_BACKSTAB, WEAR_SECOND_WIELD);
      }

      //     if (!SOMEONE_DIED(retval)) {
      // check_autojoiners(this, 0);
      //     }
    }
  }

  if (!SOMEONE_DIED(retval))
  {
    // SET_BIT(retval, check_autojoiners(this,1));
    // if (!SOMEONE_DIED(retval))

    // if (IS_AFFECTED(this, AFF_CHARM)) SET_BIT(retval, check_joincharmie(this,1));
    // if (SOMEONE_DIED(retval)) return retval;
    if (c_class == CLASS_THIEF && victim->isPlayer())
    {
      WAIT_STATE(this, DC::PULSE_VIOLENCE * 2);
    }
  }

  return retval;
}
ReturnValues Character::do_kick(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim{}, next_victim = {};
  QString name;
  qint32 dam = {};
  ReturnValue retval = {};

  if (!canPerform(SKILL_KICK))
  {
    sendln(u"You will have to study from a master before you can use this."_s);
    return ReturnValue::eFAILURE;
  }

  name = arguments.value(0);

  if (!(victim = get_char_room_vis(name)))
  {
    if (fighting)
    {
      victim = fighting;
    }
    else
    {
      sendln(u"Your foot comes up, but there's nobody there..."_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (victim == this)
  {
    sendln(u"You kick yourself, metaphorically speaking."_s);
    return ReturnValue::eFAILURE;
  }

  if (!can_attack(this) || !can_be_attacked(this, victim))
    return ReturnValue::eFAILURE;

  if (isSet(victim->combat, COMBAT_BLADESHIELD1) || isSet(victim->combat, COMBAT_BLADESHIELD2))
  {
    sendln(u"Kicking a bladeshielded opponent would be a good way to lose a leg!"_s);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(SKILL_KICK))
    return ReturnValue::eSUCCESS;

  WAIT_STATE(this, (qint32)(DC::PULSE_VIOLENCE * 1.5));

  if (!skill_success(victim, SKILL_KICK))
  {
    dam = {};
    retval = damage(this, victim, 0, TYPE_BLUDGEON, SKILL_KICK);
    if (SOMEONE_DIED(retval))
      return retval;
  }
  else
  {
    if (victim->affected_by_spell(SKILL_BATTLESENSE) && dc_->number(1, 100) < victim->affected_by_spell(SKILL_BATTLESENSE)->modifier)
    {
      act_to_character("$N's heightened battlesense sees your kick coming from a mile away.", this, 0, victim, 0);
      act_to_victim("Your heightened battlesense sees $n's kick coming from a mile away.", this, 0, victim, 0);
      act_to_room("$N's heightened battlesense sees $n's kick coming from a mile away.", this, 0, victim, NOTVICT);
      dam = {};
    }
    else
      dam = (GET_DEX(this) * 3) + (GET_STR(this) * 2) + (has_skill(SKILL_KICK));
    retval = damage(this, victim, dam, TYPE_BLUDGEON, SKILL_KICK);
    if (SOMEONE_DIED(retval))
      return retval;
  }

  // if our boots have a combat proc, and we did damage, let'um have it!
  if (dam && equipment[WEAR_FEET])
  {
    retval = weapon_spells(this, victim, WEAR_FEET);
    if (SOMEONE_DIED(retval))
      return retval;
    // leaving this built in proc here incase some new stuff is added, like kick_their_head_off
    if (dc_->obj_index_[equipment[WEAR_FEET]->item_number].combat_func)
    {
      retval = ((*dc_->obj_index_[equipment[WEAR_FEET]->item_number].combat_func)(this, equipment[WEAR_FEET], cmd_t::UNDEFINED, "", this));
    }
    if (SOMEONE_DIED(retval))
      return retval;
  }

  // Extra kick targeting main opponent for monks
  if ((GET_CLASS(this) == CLASS_MONK) && fighting && in_room == fighting->in_room)
  {
    next_victim = fighting;
    if (!skill_success(next_victim, SKILL_KICK))
    {
      dam = {};
      retval = damage(this, next_victim, 0, TYPE_UNDEFINED, SKILL_KICK);
    }
    else
    {
      dam = (GET_DEX(this) * 2) + (GET_STR(this)) + (has_skill(SKILL_KICK) / 2);
      retval = damage(this, next_victim, dam, TYPE_UNDEFINED, SKILL_KICK);
    }
    if (SOMEONE_DIED(retval))
      return retval;
    // if our boots have a combat proc, and we did damage, let'um have it!
    if (dam && equipment[WEAR_FEET])
    {
      retval = weapon_spells(this, next_victim, WEAR_FEET);
      if (SOMEONE_DIED(retval))
        return retval;
      // leaving this built in proc here incase some new stuff is added, like kick_their_head_off
      if (dc_->obj_index_[equipment[WEAR_FEET]->item_number].combat_func)
      {
        retval = ((*dc_->obj_index_[equipment[WEAR_FEET]->item_number].combat_func)(this, equipment[WEAR_FEET], cmd_t::UNDEFINED, "", this));
      }
    }
  }

  return retval;
}
ReturnValues Character::do_rescue(QStringList arguments, cmd_t cmd)
{
  CharacterPtr victim{}, tmp_ch = {};
  QString victim_name = arguments.value(0);

  if (!canPerform(SKILL_RESCUE))
  {
    sendln(u"You've got alot to learn before you try to be a bodyguard."_s);
    return ReturnValue::eFAILURE;
  }

  if (!(victim = get_char_room_vis(victim_name)))
  {
    sendln(u"Whom do you want to rescue?"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    sendln(u"What about fleeing instead?"_s);
    return ReturnValue::eFAILURE;
  }

  if (isPlayer() && (victim->isNonPlayer() && !IS_AFFECTED(victim, AFF_CHARM)))
  {
    sendln(u"Doesn't need your help!"_s);
    return ReturnValue::eFAILURE;
  }

  if (fighting == victim)
  {
    sendln(u"How can you rescue someone you are trying to kill?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!can_be_attacked(this, victim->fighting))
  {
    sendln(u"You cannot complete the rescue!"_s);
    return ReturnValue::eFAILURE;
  }

  for (tmp_ch = dc_->world[in_room]->people_; tmp_ch &&
                                              (tmp_ch->fighting != victim);
       tmp_ch = tmp_ch->next_in_room)
    ;

  if (!tmp_ch)
  {
    act_to_character("But nobody is fighting $M?", this, 0, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(SKILL_RESCUE))
    return ReturnValue::eSUCCESS;

  if (!skill_success(victim, SKILL_RESCUE))
  {
    sendln(u"You fail the rescue."_s);
    return ReturnValue::eFAILURE;
  }

  sendln(u"Banzai! To the rescue..."_s);
  act_to_character("You are rescued by $N, you are confused!", victim, 0, this, 0);
  act_to_room("$n heroically rescues $N.", this, 0, victim, NOTVICT);

  qint32 tempwait = GET_WAIT(this);
  qint32 tempvictwait = GET_WAIT(victim);

  if (victim->fighting == tmp_ch)
    stop_fighting(victim);

  if (tmp_ch->fighting)
    stop_fighting(tmp_ch, 0);
  //    if (ch->fighting)
  //     stop_fighting(ch);

  /*
   * so rescuing an NPC who is fighting a PC does not result in
   * the other guy getting killer flag
   */
  if (!fighting)
    set_fighting(this, tmp_ch);
  set_fighting(tmp_ch, this);

  WAIT_STATE(this, MAX<quint64>(DC::PULSE_VIOLENCE * 2, tempwait));
  WAIT_STATE(victim, MAX<quint64>(DC::PULSE_VIOLENCE * 2, tempvictwait));
  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_outcast(QStringList arguments, cmd_t cmd)
{
  ClanPtr clanPtr = get_clan(this);
  if (!clanPtr)
  {
    sendln(u"You are not a member of any clan!"_s);
    return ReturnValue::eFAILURE;
  }

  if (arguments.isEmpty())
  {
    sendln(u"Cast who out of your clan?"_s);
    return ReturnValue::eFAILURE;
  }

  QString arg1 = arguments.value(0).trimmed().toLower();
  // if (!arg1.isEmpty())
  // {
  //   arg1 = arg1.toUpper();
  // }

  CharacterPtr victim = get_pc(arg1);
  bool victim_connected = true;
  if (!victim)
  {
    bool victim_connected = false;
    auto result = dc_->load_char_obj(arg1);
    if (!result || !result.value())
    {
      if (file_exists(u"../archive/%1.gz"_s.arg(arg1)))
      {
        sendln(u"Character is archived."_s);
      }
      else
      {
        sendln(u"Unable to outcast, type the entire name."_s);
      }
      return ReturnValue::eFAILURE;
    }

    victim = conn->character;
    victim->conn_ = {};

    victim->hometown = START_ROOM;
    victim->in_room = START_ROOM;
    victim_connected = false;
  }

  if (!victim->clan_id_)
  {
    sendln(u"This person isn't in a clan in the first place..."_s);
    return ReturnValue::eFAILURE;
  }

  if (dc_strcmp(clanPtr->leader, qPrintable(name())) && victim != this && !has_right(this, CLAN_RIGHTS_OUTCAST))
  {
    sendln(u"You don't have the right to outcast people from your clan!"_s);
    return ReturnValue::eFAILURE;
  }

  if (clanPtr->leader == victim->name())
  {
    sendln(u"You can't outcast the clan leader!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim == this)
  {
    dc_->logentry(u"%1 just quit clan [%2]."_s.arg(victim->name()).arg(clanPtr->name()), IMPLEMENTER, DC::LogChannel::LOG_CLAN);
    sendln(u"You quit your clan."_s);
    remove_totem_stats(victim);
    victim->clan_id_ = {};
    remove_clan_member(clan, this);
    save_clans();
    return ReturnValue::eSUCCESS;
  }

  if (victim->clan_id_ != clan)
  {
    sendln(u"That person isn't in your clan!"_s);
    return ReturnValue::eFAILURE;
  }

  remove_totem_stats(victim);
  victim->clan_id_ = {};
  remove_clan_member(clan, victim);
  save_clans();
  sendln(u"You cast %1 out of your clan."_s.arg(victim->name()));
  victim->sendln(u"You are cast out of %1."_s.arg(clanPtr->name()));

  dc_->logentry(u"%1 was outcasted from clan [%2]."_s.arg(victim->name()).arg(clanPtr->name()), IMPLEMENTER, DC::LogChannel::LOG_CLAN);

  victim->save(cmd_t::SAVE_SILENTLY);
  if (!victim_connected)
    free_char(victim);

  return ReturnValue::eSUCCESS;
}
// This command deposits gold into a clan bank account
ReturnValues Character::do_cdeposit(QStringList arguments, cmd_t cmd)
{
  QString arg1;

  if (clan == 0 || get_clan(clan) == nullptr)
  {
    send(u"You are not a member of a clan.\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (isPlayerGoldThief())
  {
    send(u"Launder your money elsewhere, thief!\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (dc_->world[in_room]->number_ != DC::SORPIGAL_BANK_ROOM)
  {
    send(u"This can only be done at the Sorpigal bank.\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (arguments.isEmpty())
  {
    send(u"Usage: cdeposit <number>\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  arg1 = arguments.at(0);
  bool ok = false;
  gold_t dep = arg1.toULongLong(&ok);
  if (ok == false)
  {
    send(u"How much do you want to deposit?\r\n"_s);
    return ReturnValue::eFAILURE;
  }

  if (getGold() < dep)
  {
    send(u"You don't have %L1 $B$5gold$R coins to deposit into your clan account.\r\n"_s.arg(dep));
    send(u"You only have %L1 $B$5gold$R coins on you.\r\n"_s.arg(getGold()));
    return ReturnValue::eFAILURE;
  }

  removeGold(dep);
  save(cmd_t::SAVE_SILENTLY);
  get_clan(clan)->cdeposit(dep);
  save_clans();

  QString coin = "coin";
  if (dep > 1)
  {
    coin = "coins";
  }

  send(u"You deposit %L1 $B$5gold$R %2 into your clan's account.\r\n"_s.arg(dep).arg(coin));
  QString log_entry = u"%1 deposited %2 gold %3 in the clan bank account.\r\n"_s.arg(name_).arg(dep).arg(coin);
  ClanPtr clan = get_clan(clan);
  if (clan != nullptr)
  {
    clan->log(log_entry);
  }

  return ReturnValue::eSUCCESS;
}

ReturnValues Character::do_clanarea(QStringList arguments, cmd_t cmd)
{
  bool clanless_challenge = false;

  if (arguments.isEmpty())
  {
    return ReturnValue::eFAILURE;
  }

  QString arg = arguments.at(0);

  if (!clan)
  {
    if (arg == "challenge")
    {
      clanless_challenge = true;
    }
    else
    {
      send(u"You're not in a clan!\r\n"_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (!has_right(this, CLAN_RIGHTS_AREA) && !clanless_challenge)
  {
    sendln(u"You have not been granted that right."_s);
    return ReturnValue::eFAILURE;
  }

  if (arg == "withdraw")
  {
    if (can_collect(dc_->world[in_room].zone))
    {
      sendln(u"There is no challenge to withdraw from."_s);
      return ReturnValue::eFAILURE;
    }

    if (!affected_by_spell(SKILL_CLANAREA_CHALLENGE))
    {
      sendln(u"You did not issue the challenge, or you have waited too long to withdraw."_s);
      return ReturnValue::eFAILURE;
    }

    takeover_pulse_data *take;
    for (take = pulse_list; take; take = take->next)
      if (take->zone == dc_->world[in_room].zone &&
          take->clan2_id_ == clan)
      {
        take->clan1_id_points += 20;
        check_victory(take);
        sendln(u"You withdraw your challenge."_s);
        return ReturnValue::eSUCCESS;
      }
    sendln(u"Your did not issue this challenge."_s);
    return ReturnValue::eFAILURE;
  }
  else if (arg == "claim")
  {
    if (affected_by_spell(SKILL_CLANAREA_CLAIM))
    {
      sendln(u"You need to wait before you can attempt to claim an area."_s);
      return ReturnValue::eFAILURE;
    }

    if (dc_->zones_.value(dc_->world[in_room].zone).clanowner == 0 && !can_challenge(clan, dc_->world[in_room].zone))
    {
      sendln(u"You cannot claim this area right now."_s);
      return ReturnValue::eFAILURE;
    }

    if (dc_->zones_.value(dc_->world[in_room].zone).clanowner > 0)
    {
      send(u"This area is claimed by %s, you need to challenge to obtain ownership.\r\n"_s.arg(qPrintable(get_clan(dc_->zones_.value(dc_->world[in_room].zone).clanowner)->name())));

      return ReturnValue::eFAILURE;
    }
    if (dc_->zones_.value(dc_->world[in_room].zone).isNoClaim())
    {
      sendln(u"This area cannot be claimed."_s);
      return ReturnValue::eFAILURE;
    }
    if (count_controlled_areas(clan) >= online_clan_members(clan))
    {
      sendln(u"You cannot claim any more areas."_s);
      return ReturnValue::eFAILURE;
    }

    affected_type af;
    af.type = SKILL_CLANAREA_CLAIM;
    af.duration = 30;
    af.modifier = {};
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(this, &af, DC::PULSE_TIMER);

    auto zone_key = dc_->world[in_room].zone;
    DC::setZoneClanOwner(zone_key, clan);

    send(u"You claim the area on behalf of your clan.\r\n"_s);
    send(u"\r\n##%1 has been claimed by %2!\r\n"_s.arg(DC::getZoneName(zone_key)).arg(get_clan(clan)->name()));

    return ReturnValue::eSUCCESS;
  }
  else if (arg == "yield")
  {
    if (dc_->zones_.value(dc_->world[in_room].zone).clanowner == 0)
    {
      sendln(u"This zone is not under anyone's control."_s);
      return ReturnValue::eFAILURE;
    }
    if (dc_->zones_.value(dc_->world[in_room].zone).clanowner != clan)
    {
      sendln(u"This zone is not under your clan's control."_s);
      return ReturnValue::eFAILURE;
    }

    takeover_pulse_data *take;
    for (take = pulse_list; take; take = take->next)
      if (take->zone == dc_->world[in_room].zone &&
          take->clan1_id_ == clan && take->clan2_id_ != -2)
      {
        take->clan2_id_points += 20;
        check_victory(take);
        return ReturnValue::eSUCCESS;
      }
    sendln(u"You yield the area on behalf of your clan."_s);
    QString buf;
    dc_sprintf(buf, "\r\n##Clan %s has yielded control of%s!\r\n", qPrintable(get_clan(clan)->name()), qPrintable(dc_->zones_.value(dc_->world[in_room].zone).name()));
    send_info(buf);
    DC::setZoneClanOwner(dc_->world[in_room].zone, 0);

    return ReturnValue::eSUCCESS;
  }
  else if (arg == "collect")
  {
    if (dc_->zones_.value(dc_->world[in_room].zone).clanowner == 0)
    {
      sendln(u"This area is not under anyone's control."_s);
      return ReturnValue::eFAILURE;
    }

    if (dc_->zones_.value(dc_->world[in_room].zone).clanowner != clan)
    {
      sendln(u"This area is not under your clan's control."_s);
      return ReturnValue::eFAILURE;
    }

    if (dc_->zones_.value(dc_->world[in_room].zone).gold == 0)
    {
      sendln(u"There is no $B$5gold$R to collect."_s);
      return ReturnValue::eFAILURE;
    }
    if (!can_collect(dc_->world[in_room].zone))
    {
      sendln(u"There is currently an active challenge for this area, and collecting is not possible."_s);
      return ReturnValue::eFAILURE;
    }
    get_clan(this)->cdeposit(dc_->zones_.value(dc_->world[in_room].zone).gold);
    send(u"You collect %d $B$5gold$R for your clan's treasury.\r\n"_s.arg(dc_->zones_.value(dc_->world[in_room].zone).gold));

    DC::setZoneClanGold(dc_->world[in_room].zone, 0);
    save_clans();
    return ReturnValue::eSUCCESS;
  }
  else if (arg == "list")
  {
    qint32 z = {};
    for (auto [zone_key, zone] : dc_->zones_.asKeyValueRange())
      if (dc_->zones_.value(i).clanowner == clan)
      {
        if (++z == 1)
          send(u"$BAreas Claimed by %s:$R\r\n"_s.arg(qPrintable(get_clan(this)->name())));
        send(u"%d)%s\r\n"_s.arg(z).arg(qPrintable(dc_->zones_.value(i).name())));
      }

    if (z == 0)
    {
      sendln(u"Your clan has not claimed any areas."_s);
      return ReturnValue::eFAILURE;
    }
    return ReturnValue::eSUCCESS;
  }
  else if (arg == "challenge")
  {
    if (affected_by_spell(SKILL_CLANAREA_CHALLENGE))
    {
      sendln(u"You need to wait before you can attempt to challenge an area."_s);
      return ReturnValue::eFAILURE;
    }
    if (level_ < 40)
    {
      sendln(u"You must be level 40 to issue a challenge."_s);
      return ReturnValue::eFAILURE;
    }

    // most annoying one for last.
    if (dc_->zones_.value(dc_->world[in_room].zone).clanowner == 0)
    {
      sendln(u"This area is not under anyone's control, you could simply claim it."_s);
      return ReturnValue::eFAILURE;
    }
    if (!can_challenge(clan, dc_->world[in_room].zone))
    {
      sendln(u"You cannot issue a challenge for this area at the moment."_s);
      return ReturnValue::eFAILURE;
    }
    if (dc_->zones_.value(dc_->world[in_room].zone).clanowner == clan && !clanless_challenge)
    {
      sendln(u"Your clan already controls this area!"_s);
      return ReturnValue::eFAILURE;
    }

    if (count_controlled_areas(clan) >= online_clan_members(clan) && !clanless_challenge)
    {
      sendln(u"You cannot own any more areas."_s);
      return ReturnValue::eFAILURE;
    }

    affected_type af;
    af.type = SKILL_CLANAREA_CHALLENGE;
    af.duration = 60;
    af.modifier = {};
    af.location = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(this, &af, DC::PULSE_TIMER);

    // no point checking for noclaim flag, at this point it already IS under someone's control
    auto pl = new takeover_pulse_data;
    pl->next = pulse_list;
    pl->clan1_id_ = dc_->zones_.value(dc_->world[in_room].zone).clanowner;
    pl->clan2_id_ = clan;
    pl->clan1_id_points = pl->clan2_id_points = {};
    pl->pulse = {};
    pl->zone = dc_->world[in_room].zone;
    pulse_list = pl;
    QString buf;
    if (!clanless_challenge)
      dc_sprintf(buf, "\r\n##Clan %s has challenged clan %s for control of%s!\r\n", qPrintable(get_clan(this)->name()), qPrintable(get_clan(pl->clan1_id_)->name()), qPrintable(dc_->zones_.value(dc_->world[in_room].zone).name()));
    else
      dc_sprintf(buf, "\r\n##Clan %s's control of%s is being challenged!\r\n", qPrintable(get_clan(pl->clan1_id_)->name()), qPrintable(dc_->zones_.value(dc_->world[in_room].zone).name()));
    send_info(buf);
    return ReturnValue::eSUCCESS;
  }
  send_to_char("Clan Area Commands:\r\n"
               "--------------------\r\n"
               "clanarea list         (lists areas currently claimed by your clan)\r\n"
               "clanarea claim        (claim the area you are currently in for your clan)\r\n"
               "clanarea challenge    (challenge for control of the area you are currently in)\r\n"
               "clanarea withdraw     (withdraw a recently issued challenge)\r\n"
               "clanarea yield        (yield an area your clan controls that you are currently in)\r\n"
               "clanarea collect      (collect bounty from an area you are currently in that your clan controls)\r\n",
               this);
  return ReturnValue::eSUCCESS;
}

void Character::add_to_bard_list(void)
{
  if (GET_CLASS(this) != CLASS_BARD)
    return;

  dc_->bard_list_.push_back(this);
}

void Character::remove_from_bard_list(void)
{
  if (dc_->bard_list_.isEmpty())
    return;

  if (dc_->bard_list_.contains(this))
  {
    dc_->bard_list_.remove(dc_->bard_list_.indexOf(this)))
  }
}