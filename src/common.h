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

#endif
