#ifndef COMMON_H
#define COMMON_H
#include <cstdint>

enum class attribute_t : uint_fast8_t
{
    UNDEFINED = 0,
    STRENGTH = 1,
    DEXTERITY = 2,
    INTELLIGENCE = 3,
    WISDOM = 4,
    CONSTITUTION = 5
};

enum class position_t : uint_fast8_t
{
    DEAD = 0,
    STUNNED = 3,
    SLEEPING = 4,
    RESTING = 5,
    SITTING = 6,
    FIGHTING = 7,
    STANDING = 8
};

enum class CommandType
{
    all,
    players_only,
    non_players_only,
    immortals_only,
    implementors_only
};

enum class inet_protocol_family_t
{
    UNKNOWN,
    TCP4,
    TCP6,
    UNRECOGNIZED
};

enum class vault_search_type
{
    UNDEFINED,
    KEYWORD,
    LEVEL,
    MIN_LEVEL,
    MAX_LEVEL
};

#endif
