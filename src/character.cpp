#include <QStringTokenizer>

#include "character.h"
#include "levels.h"
#include "db.h"

void set_golem(Character *golem, int golemtype);
class Object *obj_store_to_char(Character *ch, FILE *fpsave, class Object *last_cont);

char_file_u4::char_file_u4()
{
}

QString Character::getSetting(QString key, QString defaultValue)
{
    if (pcdata != nullptr)
    {
        if (pcdata->config == nullptr)
        {
            pcdata->config = new PlayerConfig;
        }
        return pcdata->config->value(key, defaultValue);
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

QString pc_data::getJoining(void)
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

void pc_data::setJoining(QString list)
{
    joining.clear();
    auto parts = list.split(' ');
    for (auto &i : parts)
    {
        joining.insert(i, true);
    }
}

void pc_data::toggleJoining(QString key)
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

bool Character::load_charmie_equipment(QString name)
{
    int golemtype = 0;

    if (name.isEmpty())
    {
        return false;
    }

    char file[200];
    FILE *fpfile = nullptr;
    Character *golem;
    if (IS_NPC(this) || GET_LEVEL(this) < IMMORTAL)
    {
        return false;
    }

    sprintf(file, "%s/%c/%s.%d", FOLLOWER_DIR, name[0], name.toStdString().c_str(), 0);
    if (!(fpfile = fopen(file, "r")))
    { // No golem. Create a new one.
        this->send("No charmie save file found.\r\n");
        return false;
    }

    golem = clone_mobile(real_mobile(8));
    set_golem(golem, golemtype); // Basics
    this->pcdata->golem = golem;
    golem->level = 1;
    class Object *last_cont = nullptr; // Last container.
    while (!feof(fpfile))
    {
        last_cont = obj_store_to_char(golem, fpfile, last_cont);
    }
    fclose(fpfile);
    char_to_room(golem, this->in_room);

    return true;
}
