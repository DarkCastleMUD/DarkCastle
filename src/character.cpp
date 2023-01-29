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

bool Character::load_charmie_equipment(QString player_name, bool previous)
{
    int golemtype = 0;

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