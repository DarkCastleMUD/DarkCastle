#include <QStringTokenizer>
#include <QFile>

#include "character.h"
#include "levels.h"
#include "db.h"

void set_golem(Character *golem, int golemtype);
class Object *obj_store_to_char(Character *ch, FILE *fpsave, class Object *last_cont);

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

void mob_data::setObject(Object *o)
{
    object = o;
}

Object *mob_data::getObject(void)
{
    return object;
}

bool mob_data::isObject(void)
{
    return object != nullptr;
}

QString Player::getJoining(void)
{
    QString buffer;
    for (joining_t::const_iterator i = joining.begin(); i != joining.end(); ++i)
    {
        if (i.value())
        {
            if (buffer.isEmpty())
            {
                buffer = i.key();
            }
            else
            {
                buffer += " " + i.key();
            }
        }
    }

    return buffer;
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
    joining_t::iterator i = joining.find(key);
    if (i == joining.end())
    {
        joining.insert(key, true);
    }
    else
    {
        joining.insert(key, !i.value());
    }
}

PlayerConfig::PlayerConfig(QObject *parent)
    : QObject(parent)
{
    config["color.good"] = "green";
    config["color.bad"] = "red";
    config["tell.history.timestamp"] = "0";
    config["locale"] = "en_US";
    config["mode"] = "line";
    config["fighting.showdps"] = "0";
}

player_config_value_t PlayerConfig::value(const player_config_key_t &key, const player_config_value_t &defaultValue) const
{
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

bool Character::isMortal(void)
{
    return level_ < IMMORTAL;
}

bool Character::isImmortal(void)
{
    return level_ >= IMMORTAL;
}

bool Character::isImplementer(void)
{
    return level_ == IMPLEMENTER;
}

uint64_t Character::getGold(void)
{
    return gold_;
}

void Character::setGold(uint64_t gold)
{
    gold_ = gold;
}

bool Character::addGold(uint64_t gold)
{
    if (gold_ + gold < gold)
    {
        return false;
    }

    gold_ += gold;
    return true;
}

bool Character::removeGold(uint64_t gold)
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

uint64_t &Character::getGoldReference(void)
{
    return gold_;
}

bool Character::load_charmie_equipment(QString player_name, bool previous)
{
    int golemtype = 0;

    if (player_name.isEmpty())
    {
        return false;
    }

    FILE *fpfile = nullptr;

    if (IS_NPC(this) || level_ < IMMORTAL)
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
    QString filename = QString("%1.%2%3").arg(player_name).arg(0).arg(restored);

    QString path = QString("%1/%2/").arg(FOLLOWER_DIR).arg(player_name[0]);
    QString fullpath = path + filename;
    if (!(fpfile = fopen(fullpath.toStdString().c_str(), "r")))
    {
        send(QString("No charmie save file found at '%1'.").arg(fullpath));
        return false;
    }

    Character *charmie = clone_mobile(real_mobile(8));
    if (charmie == nullptr)
    {
        logentry("Error. clone_mobile(real_mobile(8)) returned nullptr.");
        return false;
    }
    charmie->setLevel(1);
    class Object *last_cont = nullptr; // Last container.
    while (!feof(fpfile))
    {
        last_cont = obj_store_to_char(charmie, fpfile, last_cont);
    }
    fclose(fpfile);

    char_to_room(charmie, in_room);

    QString message = QString("Restored charmie for player %1 with file '%2'.").arg(player_name).arg(fullpath);
    send(message);
    logentry(message);

    if (!previous)
    {
        QFile file(fullpath);
        if (file.rename(fullpath + ".restored"))
        {
            logentry(QString("Renamed '%1' to '%2'.").arg(fullpath).arg(fullpath + ".restored"));
        }
    }

    return true;
}

bool Character::validateName(QString name)
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

    if (on_forbidden_name_list(name.toStdString().c_str()))
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

    if (allowColor && connected != states::EDITING && connected != states::WRITE_BOARD && connected != states::EDIT_MPROG)
    {
        txt = handle_ansi(txt, character);
    }

    if (character != nullptr && IS_AFFECTED(character, AFF_INSANE) && connected == Connection::states::PLAYING)
    {
        txt = scramble_text(txt);
    }

    output += txt.toStdString();
}

QString Connection::getName(void)
{
    if (character)
    {
        return character->getName();
    }
    return "";
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

Sockets::Sockets(Character *ch, QString searchkey)
{
    for (Connection *d = DC::getInstance()->descriptor_list; d; d = d->next)
    {
        if (ch->getLevel() < OVERSEER)
        {
            if (d->character == nullptr)
                continue;
            if (d->character->name == nullptr)
                continue;
        }
        if (d->character)
        {
            if (!CAN_SEE(ch, d->character))
                continue;
            if (ch->getLevel() < d->character->getLevel())
                continue;
            if ((d->connected != Connection::states::PLAYING) && (ch->getLevel() < d->character->getLevel()))
                continue;
        }

        if (!searchkey.isEmpty())
        {
            if (!d->getPeerOriginalAddress().toString().contains(searchkey) && d->character != nullptr && d->character->name != nullptr && QString(GET_NAME(d->character)).contains(searchkey, Qt::CaseInsensitive) == false)
            {
                continue;
            }
        }

        const QString name = d->getName();
        if (name.size() > longest_name_size_)
        {
            longest_name_size_ = name.size();
        }

        const QString IPstr = d->getPeerFullAddressString();
        if (IPstr.size() > longest_IP_size_)
        {
            longest_IP_size_ = IPstr.size();
        }

        const QString state = constindex(d->connected, DC::connected_states);
        if (state.size() > longest_connection_state_size_)
        {
            longest_connection_state_size_ = state.size();
        }

        const QString idle = QString::number(d->idle_time / DC::PASSES_PER_SEC);
        if (idle.size() > longest_idle_size_)
        {
            longest_idle_size_ = idle.size();
        }

        IPs_[d->getPeerAddress().toString()]++;
        connections_.push_back(d);
    }
}

void Character::display_string_list(QStringList list)
{
    QString buf;
    uint64_t count{};
    for (const auto &item : list)
    {
        send(QString("%1").arg(item, 18));
        if (++count % 4 == 0)
        {
            send("\r\n");
        }
    }
    send("\r\n");
}

void Character::display_string_list(const char *list[])
{
    char buf[MAX_STRING_LENGTH]{};
    *buf = '\0';

    for (int i = 1; *list[i - 1] != '\n'; i++)
    {
        sprintf(buf + strlen(buf), "%18s", list[i - 1]);
        if (!(i % 4))
        {
            strcat(buf, "\r\n");
            send_to_char(buf, this);
            *buf = '\0';
        }
    }
    if (*buf)
        send_to_char(buf, this);
    send_to_char("\r\n", this);
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

Toggle::Toggle(QString name, uint64_t shift, command_return_t (Character::*function)(QStringList arguments, int cmd), uint64_t dependency_shift, QString on_message, QString off_message)
    : name_(name), valid_(true), shift_(shift), dependency_shift_(dependency_shift), value_(1U << shift), on_message_(on_message), off_message_(off_message), function_(function)
{
}

level_t Character::getLevel(void) const
{
    if (level_ > 110)
    {
        produce_coredump();
        logentry(QString("Warning: getLevel returned %1.").arg(level_));
    }

    return level_;
}

void Character::setLevel(level_t level)
{
    level_ = level;

    if (level_ > 110)
    {
        produce_coredump();
        logentry(QString("Warning: setLevel(%1).").arg(level_));
    }
}