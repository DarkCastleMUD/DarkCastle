#include <QStringTokenizer>

#include "character.h"
#include "levels.h"

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

void mob_data::setObject(obj_data *o)
{
    object = o;
}

obj_data *mob_data::getObject(void)
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

PlayerConfig::PlayerConfig()
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
