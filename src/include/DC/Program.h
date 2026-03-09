#pragma once
// Copyright 2023 Jared H. Hudson
// Licensed under LGPL
//
#include <QString>
#include <QSharedPointer>
#include <QList>
#include "DC/db.h"

#define ERROR_PROG -1
#define IN_FILE_PROG 0
#define ACT_PROG 1
#define SPEECH_PROG 2
#define RAND_PROG 4
#define FIGHT_PROG 8
#define DEATH_PROG 16
#define HITPRCNT_PROG 32
#define ENTRY_PROG 64
#define GREET_PROG 128
#define ALL_GREET_PROG 256
#define GIVE_PROG 512
#define BRIBE_PROG 1024
#define CATCH_PROG 2048
#define ATTACK_PROG 4096
#define ARAND_PROG 8192
#define LOAD_PROG 16384
#define COMMAND_PROG 16384 << 1
#define WEAPON_PROG 16384 << 2
#define ARMOUR_PROG 16384 << 3
#define CAN_SEE_PROG 16384 << 4
#define DAMAGE_PROG 16384 << 5
#define MPROG_MAX_TYPE_VALUE (16384 << 6)

class Program
{
    bool is_object_{};
    int type_{};
    QString arglist_;
    QString comlist_;

public:
    [[nodiscard]] int type(void) const { return type_; }
    [[nodiscard]] QString typeString(void) const
    {
        if (is_object_)
            return oprog_type_to_name(type_);
        else
            return mprog_type_to_name(type_);
    }
    [[nodiscard]] QString arglist(void) const { return arglist_; }
    [[nodiscard]] QString comlist(void) const { return comlist_; }
    [[nodiscard]] static QString mprog_type_to_name(int type)
    {
        switch (type)
        {
        case IN_FILE_PROG:
            return "in_file_prog";
        case ACT_PROG:
            return "act_prog";
        case SPEECH_PROG:
            return "speech_prog";
        case RAND_PROG:
            return "rand_prog";
        case ARAND_PROG:
            return "arand_prog";
        case FIGHT_PROG:
            return "fight_prog";
        case HITPRCNT_PROG:
            return "hitprcnt_prog";
        case DEATH_PROG:
            return "death_prog";
        case ENTRY_PROG:
            return "entry_prog";
        case GREET_PROG:
            return "greet_prog";
        case ALL_GREET_PROG:
            return "all_greet_prog";
        case GIVE_PROG:
            return "give_prog";
        case BRIBE_PROG:
            return "bribe_prog";
        case CATCH_PROG:
            return "catch_prog";
        case ATTACK_PROG:
            return "attack_prog";
        case LOAD_PROG:
            return "load_prog";
        case CAN_SEE_PROG:
            return "can_see_prog";
        case DAMAGE_PROG:
            return "damage_prog";
        case COMMAND_PROG:
            return "command_prog";
        default:
            return "ERROR_PROG";
        }
    }
    [[nodiscard]] static QString oprog_type_to_name(int type)
    {
        switch (type)
        {
        case ALL_GREET_PROG:
            return "all_greet_prog";
        case WEAPON_PROG:
            return "weapon_prog";
        case ARMOUR_PROG:
            return "armour_prog";
        case LOAD_PROG:
            return "load_prog";
        case COMMAND_PROG:
            return "command_prog";
        case ACT_PROG:
            return "act_prog";
        case ARAND_PROG:
            return "arand_prog";
        case CATCH_PROG:
            return "catch_prog";
        case SPEECH_PROG:
            return "speech_prog";
        case RAND_PROG:
            return "rand_prog";
        case CAN_SEE_PROG:
            return "can_see_prog";
        default:
            return "ERROR_PROG";
        }
    }
};

typedef QSharedPointer<Program> ProgramPtr;

class Programs
{
    bool object{};
    QList<ProgramPtr> list_;

public:
    friend int mprog_wordlist_check(QString arg, Character *mob, Character *actor, Object *obj, void *vo, int type, bool reverse);
    [[nodiscard]] bool isEmpty(void) const { return list_.isEmpty(); }
    [[nodiscard]] ProgramPtr value(qsizetype i) { return list_.value(i); }
    [[nodiscard]] int types(void) const
    {
        int t{};
        for (const auto &program : list_)
            t = t | program->type();
        return t;
    }
    void write(FILE *fl, bool mob);
    void write(auto &fl, bool mob)
    {
        for (const auto &mprg : list_)
        {
            if (mob)
                fl << ">" << mprg->typeString() << " ";
            else
                fl << "\\" << mprg->typeString() << " ";

            if (mprg->arglist().isEmpty())
                string_to_file(fl, "Saved During Edit");
            else
                string_to_file(fl, mprg->arglist());

            if (mprg->comlist().isEmpty())
                string_to_file(fl, "Saved During Edit");
            else
                string_to_file(fl, mprg->comlist());
        }
    }
    QString list(void);
};

auto &operator<<(auto &out, Programs programs)
{
    if (!programs.isEmpty())
    {
        programs.write(out, false);
        out << "|\n";
    }
    return out;
}