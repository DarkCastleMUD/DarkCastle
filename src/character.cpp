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

void Mobile::setObject(Object *o)
{
    object = o;
}

Object *Mobile::getObject(void)
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
    config["tell.history.timestamp"] = "1";
    config["locale"] = "en_US";
    config["mode"] = "line";
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
    return GET_LEVEL(this) < IMMORTAL;
}

bool Character::isImmortal(void)
{
    return GET_LEVEL(this) >= IMMORTAL;
}

bool Character::isImplementer(void)
{
    return GET_LEVEL(this) == IMPLEMENTER;
}

uint64_t Character::getGold(void)
{
    return data_.gold_;
}

void Character::setGold(uint64_t gold)
{
    data_.gold_ = gold;
}

bool Character::addGold(uint64_t gold)
{
    if (data_.gold_ + gold < gold)
    {
        return false;
    }

    data_.gold_ += gold;
    return true;
}

bool Character::removeGold(uint64_t gold)
{
    if (gold > data_.gold_)
    {
        return false;
    }

    data_.gold_ -= gold;
    return true;
}

bool Character::multiplyGold(double mult)
{
    if (data_.gold_ * mult < data_.gold_)
    {
        return false;
    }

    data_.gold_ *= mult;
    return true;
}

uint64_t &Character::getGoldReference(void)
{
    return data_.gold_;
}

sex_t &Character::getSexReference(void)
{
    return data_.sex_;
}

bool Character::load_charmie_equipment(QString player_name, bool previous)
{
    if (player_name.isEmpty())
    {
        return false;
    }

    FILE *fpfile = nullptr;

    if (IS_NPC(this) || GET_LEVEL(this) < IMMORTAL)
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
    charmie->level = 1;
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
        string buffer = handle_ansi(txt.toStdString(), character);
        txt = buffer.c_str();
    }

    if (character != nullptr && IS_AFFECTED(character, AFF_INSANE) && connected == Connection::states::PLAYING)
    {
        txt = scramble_text(txt);
    }

    output += txt.toStdString();
}

void Character::clear(void)
{
    Character ch;
    duplicate(ch);
}

void Character::duplicate(const Character &source)
{
    mobile = duplicateClass<Mobile>(source.mobile);
    player = duplicateClass<Player>(source.player);
    object = duplicateClass<Object>(source.object);
    desc = duplicateClass<Connection>(source.desc);
    name = duplicateCString(source.getName().toStdString().c_str());
    short_desc = duplicateCString(source.short_desc);
    long_desc = duplicateCString(source.long_desc);
    description = duplicateCString(source.description);
    title = duplicateCString(source.title);

    data_ = source.data_;
}

bool Character::isMobile(void)
{
    return IS_SET(getMisc(), MISC_IS_MOB);
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
