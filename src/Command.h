#ifndef COMMAND_H
#define COMMAND_H

#include <expected>
#include <QList>
#include <QMap>
#include "common.h"

enum class CommandType
{
    all,
    players_only,
    non_players_only,
    immortals_only,
    implementors_only
};

class Command
{
public:
    /*
        Command(QString name = QString(),
                command_gen1_t ptr1 = nullptr, command_gen2_t ptr2 = nullptr, command_gen3_t ptr3 = nullptr,
                position_t min_pos = {}, level_t min_lvl = {}, int nr = CMD_DEFAULT,
                bool allow_charmie = false, uint8_t toggle_hide = {}, CommandType type = {})
            : name_(name),
              command_pointer_(ptr1), command_pointer2_(ptr2), command_pointer3_(ptr3),
              minimum_position_(min_pos), minimum_level_(min_lvl), command_number_(nr),
              allow_charmie_(allow_charmie), toggle_hide_(toggle_hide), type_(type) {}*/

    [[nodiscard]] inline QString getName(void) const { return name_; }
    void setName(const QString name) { name_ = name; }

    [[nodiscard]] inline command_gen1_t getFunction1(void) const { return command_pointer_; }
    void setFunction1(const command_gen1_t function) { command_pointer_ = function; }

    [[nodiscard]] inline command_gen2_t getFunction2(void) const { return command_pointer2_; }
    void setFunction2(const command_gen2_t function) { command_pointer2_ = function; }

    [[nodiscard]] inline command_gen3_t getFunction3(void) const { return command_pointer3_; }
    void setFunction3(const command_gen3_t function) { command_pointer3_ = function; }

    [[nodiscard]] inline level_t getNumber(void) const { return command_number_; }
    void setNumber(const level_t number) { command_number_ = number; }

    [[nodiscard]] inline level_t getMinimumLevel(void) const { return minimum_level_; }
    void setMinimumLevel(const level_t level) { minimum_level_ = level; }

    [[nodiscard]] inline position_t getMinimumPosition(void) const { return minimum_position_; }
    void setMinimumPosition(const position_t minimum_position) { minimum_position_ = minimum_position; }

    bool isCharmieAllowed(void) const { return allow_charmie_ == true; }

    [[nodiscard]] inline CommandType getType(void) const { return type_; }
    void setType(const CommandType type) { type_ = type; }

    QString name_;

    // int (*command_pointer_)(class Character *ch, char *argument, int cmd);               /* Function that does it            */
    // command_return_t (*command_pointer2_)(Character *ch, std::string argument, int cmd); /* Function that does it            */
    // command_return_t (Character::*command_pointer3_)(QStringList arguments, int cmd);    /* Function that does it            */

    command_gen1_t command_pointer_;
    command_gen2_t command_pointer2_;
    command_gen3_t command_pointer3_;
    position_t minimum_position_; /* Position commander must be in    */
    level_t minimum_level_;       /* Minimum level needed             */
    int command_number_;          /* Passed to function as argument   */
    bool allow_charmie_;
    uint8_t toggle_hide_;
    CommandType type_;
};
class Commands
{
public:
    Commands(void);
    void add(Command cmd);
    auto find(QString arg) -> std::expected<Command, search_error>;
    const static QList<Command> commands_;
    static QMap<QString, Command> map_;
};

#endif