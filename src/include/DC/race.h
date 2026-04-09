#pragma once
/************************************************************************
| $Id: race.h,v 1.12 2007/02/12 00:14:34 dcastle Exp $
| race.h
| This file defines racial information.
*/
constexpr auto RACE_NONE = 0;
constexpr auto RACE_HUMAN = 1;
constexpr auto RACE_ELVEN = 2;
constexpr auto RACE_DWARVEN = 3;
constexpr auto RACE_HOBBIT = 4;
constexpr auto RACE_PIXIE = 5;
constexpr auto RACE_GIANT = 6;
constexpr auto RACE_GNOME = 7;
constexpr auto RACE_ORC = 8;
constexpr auto RACE_TROLL = 9;

constexpr auto MAX_PC_RACE = 9;
/* Not player races from here down */
constexpr auto RACE_GOBLIN = 10;
constexpr auto RACE_REPTILE = 11;
constexpr auto RACE_DRAGON = 12;
constexpr auto RACE_SNAKE = 13;
constexpr auto RACE_HORSE = 14;
constexpr auto RACE_BIRD = 15;
constexpr auto RACE_RODENT = 16;
constexpr auto RACE_FISH = 17;
constexpr auto RACE_ARACHNID = 18;
constexpr auto RACE_INSECT = 19;
constexpr auto RACE_SLIME = 20;
constexpr auto RACE_ANIMAL = 21;
constexpr auto RACE_TREE = 22;
constexpr auto RACE_ENFAN = 23;
constexpr auto RACE_UNDEAD = 24;
constexpr auto RACE_GHOST = 25;
constexpr auto RACE_GOLEM = 26;
constexpr auto RACE_ELEMENT = 27;
constexpr auto RACE_PLANAR = 28;
constexpr auto RACE_DEMON = 29;
constexpr auto RACE_YRNALI = 30;
constexpr auto RACE_IMMORTAL = 31;
constexpr auto RACE_FELINE = 32;
constexpr auto MAX_RACE = 32;

/* Bitvectors for racial shit */
constexpr auto BITV_HUMAN = 1;
constexpr auto BITV_ELVEN = 1 << 1;
constexpr auto BITV_DWARVEN = 1 << 2;
constexpr auto BITV_HOBBIT = 1 << 3;
constexpr auto BITV_PIXIE = 1 << 4;
constexpr auto BITV_GIANT = 1 << 5;
constexpr auto BITV_GNOME = 1 << 6;
constexpr auto BITV_ORC = 1 << 7;
constexpr auto BITV_TROLL = 1 << 8;
constexpr auto BITV_GOBLIN = 1 << 9;
constexpr auto BITV_REPTILE = 1 << 10;
constexpr auto BITV_DRAGON = 1 << 11;
constexpr auto BITV_SNAKE = 1 << 12;
constexpr auto BITV_HORSE = 1 << 13;
constexpr auto BITV_BIRD = 1 << 14;
constexpr auto BITV_RODENT = 1 << 15;
constexpr auto BITV_FISH = 1 << 16;
constexpr auto BITV_ARACHNID = 1 << 17;
constexpr auto BITV_INSECT = 1 << 18;
constexpr auto BITV_SLIME = 1 << 19;
constexpr auto BITV_ANIMAL = 1 << 20;
constexpr auto BITV_TREE = 1 << 21;
constexpr auto BITV_ENFAN = 1 << 22;
constexpr auto BITV_UNDEAD = 1 << 23;
constexpr auto BITV_GHOST = 1 << 24;
constexpr auto BITV_GOLEM = 1 << 25;
constexpr auto BITV_ELEMENT = 1 << 26;
constexpr auto BITV_PLANAR = 1 << 27;
constexpr auto BITV_DEMON = 1 << 28;
constexpr auto BITV_YRNALI = 1 << 29;
constexpr auto BITV_IMMORTAL = 1 << 30;

// Following are modifiers from the base maximum stat for a race
constexpr auto BASE_MAX_STAT = 27;
// Modifiers below only happen on creation and has little influence
// On actual maxes

constexpr auto RACE_HUMAN_STR_MOD = 0;
constexpr auto RACE_HUMAN_DEX_MOD = 0;
constexpr auto RACE_HUMAN_INT_MOD = 0;
constexpr auto RACE_HUMAN_WIS_MOD = 0;
constexpr auto RACE_HUMAN_CON_MOD = 0;

constexpr auto RACE_ELVEN_STR_MOD = -1;
constexpr auto RACE_ELVEN_DEX_MOD = 1;
constexpr auto RACE_ELVEN_INT_MOD = 1;
constexpr auto RACE_ELVEN_WIS_MOD = 0;
constexpr auto RACE_ELVEN_CON_MOD = -1;

constexpr auto RACE_DWARVEN_STR_MOD = 2;
constexpr auto RACE_DWARVEN_DEX_MOD = -2;
constexpr auto RACE_DWARVEN_WIS_MOD = 1;
constexpr auto RACE_DWARVEN_INT_MOD = -2;
constexpr auto RACE_DWARVEN_CON_MOD = 1;

constexpr auto RACE_HOBBIT_STR_MOD = -2;
constexpr auto RACE_HOBBIT_DEX_MOD = 3;
constexpr auto RACE_HOBBIT_CON_MOD = -1;
constexpr auto RACE_HOBBIT_INT_MOD = 0;
constexpr auto RACE_HOBBIT_WIS_MOD = 0;

constexpr auto RACE_PIXIE_STR_MOD = -4;
constexpr auto RACE_PIXIE_CON_MOD = -2;
constexpr auto RACE_PIXIE_WIS_MOD = 1;
constexpr auto RACE_PIXIE_INT_MOD = 3;
constexpr auto RACE_PIXIE_DEX_MOD = 2;

constexpr auto RACE_GIANT_STR_MOD = 3;
constexpr auto RACE_GIANT_DEX_MOD = -2;
constexpr auto RACE_GIANT_CON_MOD = 1;
constexpr auto RACE_GIANT_WIS_MOD = 0;
constexpr auto RACE_GIANT_INT_MOD = -2;

constexpr auto RACE_GNOME_STR_MOD = -2;
constexpr auto RACE_GNOME_DEX_MOD = -2;
constexpr auto RACE_GNOME_INT_MOD = 1;
constexpr auto RACE_GNOME_WIS_MOD = 3;
constexpr auto RACE_GNOME_CON_MOD = 0;

constexpr auto RACE_ORC_STR_MOD = 1;
constexpr auto RACE_ORC_DEX_MOD = 0;
constexpr auto RACE_ORC_INT_MOD = 0;
constexpr auto RACE_ORC_WIS_MOD = -2;
constexpr auto RACE_ORC_CON_MOD = 1;

constexpr auto RACE_TROLL_STR_MOD = 2;
constexpr auto RACE_TROLL_DEX_MOD = 0;
constexpr auto RACE_TROLL_CON_MOD = 3;
constexpr auto RACE_TROLL_WIS_MOD = -2;
constexpr auto RACE_TROLL_INT_MOD = -3;

//       Save modifications dependant upon race
constexpr auto RACE_HUMAN_FIRE_MOD = 0;
constexpr auto RACE_HUMAN_COLD_MOD = 0;
constexpr auto RACE_HUMAN_ENERGY_MOD = 0;
constexpr auto RACE_HUMAN_ACID_MOD = 0;
constexpr auto RACE_HUMAN_MAGIC_MOD = 0;
constexpr auto RACE_HUMAN_POISON_MOD = 0;

constexpr auto RACE_TROLL_FIRE_MOD = -6;
constexpr auto RACE_TROLL_COLD_MOD = 0;
constexpr auto RACE_TROLL_ENERGY_MOD = 3;
constexpr auto RACE_TROLL_ACID_MOD = -3;
constexpr auto RACE_TROLL_MAGIC_MOD = 0;
constexpr auto RACE_TROLL_POISON_MOD = 6;

constexpr auto RACE_ELVEN_FIRE_MOD = 0;
constexpr auto RACE_ELVEN_COLD_MOD = 0;
constexpr auto RACE_ELVEN_ENERGY_MOD = 3;
constexpr auto RACE_ELVEN_ACID_MOD = -3;
constexpr auto RACE_ELVEN_MAGIC_MOD = 3;
constexpr auto RACE_ELVEN_POISON_MOD = -3;

constexpr auto RACE_DWARVEN_FIRE_MOD = -3;
constexpr auto RACE_DWARVEN_COLD_MOD = -3;
constexpr auto RACE_DWARVEN_ENERGY_MOD = 0;
constexpr auto RACE_DWARVEN_ACID_MOD = 3;
constexpr auto RACE_DWARVEN_MAGIC_MOD = 0;
constexpr auto RACE_DWARVEN_POISON_MOD = 3;

constexpr auto RACE_GIANT_FIRE_MOD = 3;
constexpr auto RACE_GIANT_COLD_MOD = 6;
constexpr auto RACE_GIANT_ENERGY_MOD = -3;
constexpr auto RACE_GIANT_ACID_MOD = 0;
constexpr auto RACE_GIANT_MAGIC_MOD = -6;
constexpr auto RACE_GIANT_POISON_MOD = 0;

constexpr auto RACE_PIXIE_FIRE_MOD = -3;
constexpr auto RACE_PIXIE_COLD_MOD = 0;
constexpr auto RACE_PIXIE_ENERGY_MOD = 3;
constexpr auto RACE_PIXIE_ACID_MOD = 0;
constexpr auto RACE_PIXIE_MAGIC_MOD = 6;
constexpr auto RACE_PIXIE_POISON_MOD = -6;

constexpr auto RACE_HOBBIT_FIRE_MOD = 6;
constexpr auto RACE_HOBBIT_COLD_MOD = -6;
constexpr auto RACE_HOBBIT_ENERGY_MOD = 0;
constexpr auto RACE_HOBBIT_ACID_MOD = 0;
constexpr auto RACE_HOBBIT_MAGIC_MOD = -3;
constexpr auto RACE_HOBBIT_POISON_MOD = 3;

constexpr auto RACE_GNOME_FIRE_MOD = 0;
constexpr auto RACE_GNOME_COLD_MOD = 0;
constexpr auto RACE_GNOME_ENERGY_MOD = -3;
constexpr auto RACE_GNOME_ACID_MOD = 3;
constexpr auto RACE_GNOME_MAGIC_MOD = 3;
constexpr auto RACE_GNOME_POISON_MOD = -3;

constexpr auto RACE_ORC_FIRE_MOD = 3;
constexpr auto RACE_ORC_COLD_MOD = 3;
constexpr auto RACE_ORC_ENERGY_MOD = -3;
constexpr auto RACE_ORC_ACID_MOD = 0;
constexpr auto RACE_ORC_MAGIC_MOD = -6;
constexpr auto RACE_ORC_POISON_MOD = 0;
