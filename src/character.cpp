#include "DC/DC.h"

void set_golem(CharacterPtr golem, qint32 golemtype);
ObjectPtr obj_store_to_char(CharacterPtr ch, FILE *fpsave, ObjectPtr last_cont);

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

  FILE *fpfile = {};

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

  if (character != nullptr && IS_AFFECTED(character, AFF_INSANE) && isPlaying())
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

Toggle::Toggle(QString name, quint64 shift, ReturnValue (Character::*function)(QStringList arguments, cmd_t cmd), quint64 dependency_shift, QString on_message, QString off_message)
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
