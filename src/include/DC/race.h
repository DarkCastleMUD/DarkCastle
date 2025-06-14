#ifndef RACE_H_
#define RACE_H_
/************************************************************************
| $Id: race.h,v 1.12 2007/02/12 00:14:34 dcastle Exp $
| race.h
| This file defines racial information.
*/
constexpr auto RACE_NONE = 0U;
constexpr auto RACE_HUMAN = 1U;
constexpr auto RACE_ELVEN = 2U;
constexpr auto RACE_DWARVEN = 3U;
constexpr auto RACE_HOBBIT = 4U;
constexpr auto RACE_PIXIE = 5U;
constexpr auto RACE_GIANT = 6U;
constexpr auto RACE_GNOME = 7U;
constexpr auto RACE_ORC = 8U;
constexpr auto RACE_TROLL = 9U;

#define MAX_PC_RACE 9
/* Not player races from here down */
constexpr auto RACE_GOBLIN = 10U;
constexpr auto RACE_REPTILE = 11U;
constexpr auto RACE_DRAGON = 12U;
constexpr auto RACE_SNAKE = 13U;
constexpr auto RACE_HORSE = 14U;
constexpr auto RACE_BIRD = 15U;
constexpr auto RACE_RODENT = 16U;
constexpr auto RACE_FISH = 17U;
constexpr auto RACE_ARACHNID = 18U;
constexpr auto RACE_INSECT = 19U;
constexpr auto RACE_SLIME = 20U;
constexpr auto RACE_ANIMAL = 21U;
constexpr auto RACE_TREE = 22U;
constexpr auto RACE_ENFAN = 23U;
constexpr auto RACE_UNDEAD = 24U;
constexpr auto RACE_GHOST = 25U;
constexpr auto RACE_GOLEM = 26U;
constexpr auto RACE_ELEMENT = 27U;
constexpr auto RACE_ASTRAL = 28U;
constexpr auto RACE_DEMON = 29U;
constexpr auto RACE_YRNALI = 30U;
constexpr auto RACE_IMMORTAL = 31U;
constexpr auto RACE_FELINE = 32U;
// #define MAX_RACE         32

/* Bitvectors for racial shit */
#define BITV_HUMAN 1
#define BITV_ELVEN 1 << 1
#define BITV_DWARVEN 1 << 2
#define BITV_HOBBIT 1 << 3
#define BITV_PIXIE 1 << 4
#define BITV_GIANT 1 << 5
#define BITV_GNOME 1 << 6
#define BITV_ORC 1 << 7
#define BITV_TROLL 1 << 8
#define BITV_GOBLIN 1 << 9
#define BITV_REPTILE 1 << 10
#define BITV_DRAGON 1 << 11
#define BITV_SNAKE 1 << 12
#define BITV_HORSE 1 << 13
#define BITV_BIRD 1 << 14
#define BITV_RODENT 1 << 15
#define BITV_FISH 1 << 16
#define BITV_ARACHNID 1 << 17
#define BITV_INSECT 1 << 18
#define BITV_SLIME 1 << 19
#define BITV_ANIMAL 1 << 20
#define BITV_TREE 1 << 21
#define BITV_ENFAN 1 << 22
#define BITV_UNDEAD 1 << 23
#define BITV_GHOST 1 << 24
#define BITV_GOLEM 1 << 25
#define BITV_ELEMENT 1 << 26
#define BITV_ASTRAL 1 << 27
#define BITV_DEMON 1 << 28
#define BITV_YRNALI 1 << 29
#define BITV_IMMORTAL 1 << 30

// Following are modifiers from the base maximum stat for a race
#define BASE_MAX_STAT 27
// Modifiers below only happen on creation and has little influence
// On actual maxes

#define RACE_HUMAN_STR_MOD 0
#define RACE_HUMAN_DEX_MOD 0
#define RACE_HUMAN_INT_MOD 0
#define RACE_HUMAN_WIS_MOD 0
#define RACE_HUMAN_CON_MOD 0

#define RACE_ELVEN_STR_MOD -1
#define RACE_ELVEN_DEX_MOD 1
#define RACE_ELVEN_INT_MOD 1
#define RACE_ELVEN_WIS_MOD 0
#define RACE_ELVEN_CON_MOD -1

#define RACE_DWARVEN_STR_MOD 2
#define RACE_DWARVEN_DEX_MOD -2
#define RACE_DWARVEN_WIS_MOD 1
#define RACE_DWARVEN_INT_MOD -2
#define RACE_DWARVEN_CON_MOD 1

#define RACE_HOBBIT_STR_MOD -2
#define RACE_HOBBIT_DEX_MOD 3
#define RACE_HOBBIT_CON_MOD -1
#define RACE_HOBBIT_INT_MOD 0
#define RACE_HOBBIT_WIS_MOD 0

#define RACE_PIXIE_STR_MOD -4
#define RACE_PIXIE_CON_MOD -2
#define RACE_PIXIE_WIS_MOD 1
#define RACE_PIXIE_INT_MOD 3
#define RACE_PIXIE_DEX_MOD 2

#define RACE_GIANT_STR_MOD 3
#define RACE_GIANT_DEX_MOD -2
#define RACE_GIANT_CON_MOD 1
#define RACE_GIANT_WIS_MOD 0
#define RACE_GIANT_INT_MOD -2

#define RACE_GNOME_STR_MOD -2
#define RACE_GNOME_DEX_MOD -2
#define RACE_GNOME_INT_MOD 1
#define RACE_GNOME_WIS_MOD 3
#define RACE_GNOME_CON_MOD 0

#define RACE_ORC_STR_MOD 1
#define RACE_ORC_DEX_MOD 0
#define RACE_ORC_INT_MOD 0
#define RACE_ORC_WIS_MOD -2
#define RACE_ORC_CON_MOD 1

#define RACE_TROLL_STR_MOD 2
#define RACE_TROLL_DEX_MOD 0
#define RACE_TROLL_CON_MOD 3
#define RACE_TROLL_WIS_MOD -2
#define RACE_TROLL_INT_MOD -3

//       Save modifications dependant upon race
#define RACE_HUMAN_FIRE_MOD 0
#define RACE_HUMAN_COLD_MOD 0
#define RACE_HUMAN_ENERGY_MOD 0
#define RACE_HUMAN_ACID_MOD 0
#define RACE_HUMAN_MAGIC_MOD 0
#define RACE_HUMAN_POISON_MOD 0

#define RACE_TROLL_FIRE_MOD -6
#define RACE_TROLL_COLD_MOD 0
#define RACE_TROLL_ENERGY_MOD 3
#define RACE_TROLL_ACID_MOD -3
#define RACE_TROLL_MAGIC_MOD 0
#define RACE_TROLL_POISON_MOD 6

#define RACE_ELVEN_FIRE_MOD 0
#define RACE_ELVEN_COLD_MOD 0
#define RACE_ELVEN_ENERGY_MOD 3
#define RACE_ELVEN_ACID_MOD -3
#define RACE_ELVEN_MAGIC_MOD 3
#define RACE_ELVEN_POISON_MOD -3

#define RACE_DWARVEN_FIRE_MOD -3
#define RACE_DWARVEN_COLD_MOD -3
#define RACE_DWARVEN_ENERGY_MOD 0
#define RACE_DWARVEN_ACID_MOD 3
#define RACE_DWARVEN_MAGIC_MOD 0
#define RACE_DWARVEN_POISON_MOD 3

#define RACE_GIANT_FIRE_MOD 3
#define RACE_GIANT_COLD_MOD 6
#define RACE_GIANT_ENERGY_MOD -3
#define RACE_GIANT_ACID_MOD 0
#define RACE_GIANT_MAGIC_MOD -6
#define RACE_GIANT_POISON_MOD 0

#define RACE_PIXIE_FIRE_MOD -3
#define RACE_PIXIE_COLD_MOD 0
#define RACE_PIXIE_ENERGY_MOD 3
#define RACE_PIXIE_ACID_MOD 0
#define RACE_PIXIE_MAGIC_MOD 6
#define RACE_PIXIE_POISON_MOD -6

#define RACE_HOBBIT_FIRE_MOD 6
#define RACE_HOBBIT_COLD_MOD -6
#define RACE_HOBBIT_ENERGY_MOD 0
#define RACE_HOBBIT_ACID_MOD 0
#define RACE_HOBBIT_MAGIC_MOD -3
#define RACE_HOBBIT_POISON_MOD 3

#define RACE_GNOME_FIRE_MOD 0
#define RACE_GNOME_COLD_MOD 0
#define RACE_GNOME_ENERGY_MOD -3
#define RACE_GNOME_ACID_MOD 3
#define RACE_GNOME_MAGIC_MOD 3
#define RACE_GNOME_POISON_MOD -3

#define RACE_ORC_FIRE_MOD 3
#define RACE_ORC_COLD_MOD 3
#define RACE_ORC_ENERGY_MOD -3
#define RACE_ORC_ACID_MOD 0
#define RACE_ORC_MAGIC_MOD -6
#define RACE_ORC_POISON_MOD 0

#endif
