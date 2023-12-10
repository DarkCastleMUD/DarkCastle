#ifndef COMMAND_H
#define COMMAND_H

#include <expected>
#include <QList>
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
    Command(QString name = {}, command_gen1_t ptr1 = {}, command_gen2_t ptr2 = {}, command_gen3_t ptr3 = {}, position_t min_pos = {}, level_t min_lvl = {}, int nr = {}, int flag = {}, uint8_t tog_hid = {}, CommandType typ = {})
        : command_pointer(nullptr), command_pointer2(nullptr), command_pointer3(nullptr), minimum_position(position_t()), minimum_level(level_t()), command_number(CMD_DEFAULT), toggle_hide(0), type(CommandType()), allow_charmie_(false) {}

    bool isCharmieAllowed(void)
    {
        return allow_charmie_ == true;
    }
    QString command_name;                                                               /* Name of ths command             */
    int (*command_pointer)(class Character *ch, char *argument, int cmd);               /* Function that does it            */
    command_return_t (*command_pointer2)(Character *ch, std::string argument, int cmd); /* Function that does it            */
    command_return_t (Character::*command_pointer3)(QStringList arguments, int cmd);    /* Function that does it            */
    position_t minimum_position;                                                        /* Position commander must be in    */
    level_t minimum_level;                                                              /* Minimum level needed             */
    int command_number;                                                                 /* Passed to function as argument   */

    uint8_t toggle_hide;
    CommandType type;

private:
    bool allow_charmie_;
};

class cmd_hash_info
{
public:
    cmd_hash_info(void)
        : left(nullptr), right(nullptr) {}
    Command command;
    cmd_hash_info *left{};
    cmd_hash_info *right{};
};

class Commands
{
public:
    static void add_command_to_radix(Command cmd);
    static void add_commands_to_radix(void);
    static auto find_cmd_in_radix(QString arg) -> std::expected<Command, search_error>;
    void free_command_radix_nodes(cmd_hash_info *curr);

    static const QList<Command> commands;

private:
    static cmd_hash_info *cmd_radix_;
};

#endif