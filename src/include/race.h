#ifndef RACE_H_
#define RACE_H_
/************************************************************************
| $Id: race.h,v 1.3 2004/04/18 14:46:47 urizen Exp $
| race.h
| This file defines racial information.
*/
#define RACE_NONE         0
#define RACE_HUMAN        1
#define RACE_ELVEN        2
#define RACE_DWARVEN      3
#define RACE_HOBBIT       4
#define RACE_PIXIE        5
#define RACE_GIANT        6
#define RACE_GNOME        7
#define RACE_ORC          8
/* Not player races from here down */ 
#define RACE_TROLL        9
#define RACE_GOBLIN      10
#define RACE_REPTILE     11
#define RACE_DRAGON      12
#define RACE_SNAKE       13
#define RACE_HORSE       14
#define RACE_BIRD        15
#define RACE_RODENT      16
#define RACE_FISH        17
#define RACE_ARACHNID    18
#define RACE_INSECT      19
#define RACE_SLIME       20
#define RACE_ANIMAL      21
#define RACE_TREE        22
#define RACE_ENFAN       23
#define RACE_UNDEAD      24
#define RACE_GHOST       25
#define RACE_GOLEM       26
#define RACE_ELEMENT     27
#define RACE_ASTRAL      28
#define RACE_DEMON       29
#define RACE_YRNALI      30
#define MAX_RACE         30

/* Bitvectors for racial shit */
#define BITV_HUMAN       1
#define BITV_ELVEN       1<<1
#define BITV_DWARVEN     1<<2
#define BITV_HOBBIT      1<<3
#define BITV_PIXIE       1<<4
#define BITV_GIANT       1<<5
#define BITV_GNOME       1<<6
#define BITV_ORC         1<<7
#define BITV_TROLL       1<<8
#define BITV_GOBLIN      1<<9
#define BITV_REPTILE     1<<10
#define BITV_DRAGON      1<<11
#define BITV_SNAKE       1<<12
#define BITV_HORSE       1<<13
#define BITV_BIRD        1<<14
#define BITV_RODENT      1<<15
#define BITV_FISH        1<<16
#define BITV_ARACHNID    1<<17
#define BITV_INSECT      1<<18
#define BITV_SLIME       1<<19
#define BITV_ANIMAL      1<<20
#define BITV_TREE        1<<21
#define BITV_ENFAN       1<<22
#define BITV_UNDEAD      1<<23
#define BITV_GHOST       1<<24
#define BITV_GOLEM       1<<25
#define BITV_ELEMENT     1<<26
#define BITV_ASTRAL      1<<27
#define BITV_DEMON       1<<28

// Following are modifiers from the base maximum stat for a race
#define BASE_MAX_STAT              27

#define RACE_HUMAN_STR_MOD          0
#define RACE_HUMAN_DEX_MOD          0
#define RACE_HUMAN_INT_MOD          0
#define RACE_HUMAN_WIS_MOD          0
#define RACE_HUMAN_CON_MOD          0

#define RACE_ELVEN_STR_MOD         -1
#define RACE_ELVEN_DEX_MOD          1
#define RACE_ELVEN_INT_MOD          1
#ifdef COMILE_WITH_CHANGES
  #define RACE_ELVEN_WIS_MOD        0
#else
  #define RACE_ELVEN_WIS_MOD          1
#endif
#define RACE_ELVEN_CON_MOD         -1

#ifdef COMPILE_WITH_CHANGES
  #define RACE_DWARVEN_STR_MOD 	2
  #define RACE_DWARVEN_DEX_MOD -2
  #define RACE_DWARVEN_WIS_MOD  1
  #define RACE_DWARVEN_INT_MOD -2
#else
  #define RACE_DWARVEN_STR_MOD        1
  #define RACE_DWARVEN_DEX_MOD       -1
  #define RACE_DWARVEN_INT_MOD       -1
  #define RACE_DWARVEN_WIS_MOD        0
#endif
#define RACE_DWARVEN_CON_MOD        1

#ifdef COMPILE_WITH_CHANGES
  #define RACE_HOBBIT_STR_MOD 	    -2
  #define RACE_HOBBIT_DEX_MOD	    3
  #define RACE_HOBBIT_CON_MOD	    -1
  #define RACE_HOBBIT_INT_MOD       0
  #define RACE_HOBBIT_WIS_MOD       0
#else
  #define RACE_HOBBIT_STR_MOD        -1
  #define RACE_HOBBIT_DEX_MOD         2
  #define RACE_HOBBIT_INT_MOD        -1
  #define RACE_HOBBIT_WIS_MOD         1
  #define RACE_HOBBIT_CON_MOD         0
#endif

#ifdef COMPILE_WITH_CHANGES
#define RACE_PIXIE_STR_MOD	   -4
#define RACE_PIXIE_CON_MOD	   -2
#define RACE_PIXIE_WIS_MOD	    1
#define RACE_PIXIE_INT_MOD	    3
#else
#define RACE_PIXIE_STR_MOD         -3
#define RACE_PIXIE_CON_MOD         -1
#define RACE_PIXIE_INT_MOD          2
#define RACE_PIXIE_WIS_MOD          2
#endif
#define RACE_PIXIE_DEX_MOD         2

#define RACE_GIANT_STR_MOD          3
#ifdef COMPILE_WITH_CHANGES
  #define RACE_GIANT_DEX_MOD	    -2
  #define RACE_GIANT_CON_MOD	    1
  #define RACE_GIANT_WIS_MOD	    0
#else
  #define RACE_GIANT_DEX_MOD        -1
  #define RACE_GIANT_CON_MOD	    3
  #define RACE_GIANT_WIS_MOD	   -2
#endif
#define RACE_GIANT_INT_MOD         -2

#ifdef COMPILE_WITH_CHANGES
 #define RACE_GNOME_STR_MOD         -2
 #define RACE_GNOME_DEX_MOD         -2
 #define RACE_GNOME_INT_MOD          1
 #define RACE_GNOME_WIS_MOD          3
 #define RACE_GNOME_CON_MOD          0
#else
 #define RACE_GNOME_STR_MOD         -2
 #define RACE_GNOME_DEX_MOD          0
 #define RACE_GNOME_INT_MOD          2
 #define RACE_GNOME_WIS_MOD         -2
 #define RACE_GNOME_CON_MOD          2
#endif

#define RACE_ORC_STR_MOD            1
#define RACE_ORC_DEX_MOD            0
#ifdef COMPILE_WITH_CHANGES
  #define RACE_ORC_CON_MOD	1
  #define RACE_ORC_WIS_MOD	-2
  #define RACE_ORC_INT_MOD	0
#else
  #define RACE_ORC_INT_MOD           -2
  #define RACE_ORC_WIS_MOD           -1
  #define RACE_ORC_CON_MOD            2
#endif

#define RACE_TROLL_STR_MOD 2
#define RACE_TROLL_DEX_MOD 0
#define RACE_TROLL_CON_MOD 3
#define RACE_TROLL_WIS_MOD -2
#define RACE_TROLL_INT_MOD -3

//       Save modifications dependant upon race
#define RACE_HUMAN_FIRE_MOD         0
#define RACE_HUMAN_COLD_MOD         0
#define RACE_HUMAN_ENERGY_MOD       0
#define RACE_HUMAN_ACID_MOD         0
#define RACE_HUMAN_MAGIC_MOD        0
#define RACE_HUMAN_POISON_MOD       0

#define RACE_ELVEN_FIRE_MOD         -4
#define RACE_ELVEN_COLD_MOD         0
#define RACE_ELVEN_ENERGY_MOD       3
#define RACE_ELVEN_ACID_MOD         0
#define RACE_ELVEN_MAGIC_MOD        5
#define RACE_ELVEN_POISON_MOD       -4

#define RACE_DWARVEN_FIRE_MOD       5
#define RACE_DWARVEN_COLD_MOD       4
#define RACE_DWARVEN_ENERGY_MOD     -5
#define RACE_DWARVEN_ACID_MOD       -4
#define RACE_DWARVEN_MAGIC_MOD      0
#define RACE_DWARVEN_POISON_MOD     0

#define RACE_GIANT_FIRE_MOD         0
#define RACE_GIANT_COLD_MOD         0
#define RACE_GIANT_ENERGY_MOD       0
#define RACE_GIANT_ACID_MOD         -3
#define RACE_GIANT_MAGIC_MOD        -3
#define RACE_GIANT_POISON_MOD       10

#define RACE_PIXIE_FIRE_MOD         0
#define RACE_PIXIE_COLD_MOD         0
#define RACE_PIXIE_ENERGY_MOD       2
#define RACE_PIXIE_ACID_MOD         0
#define RACE_PIXIE_MAGIC_MOD        2
#define RACE_PIXIE_POISON_MOD       0

#define RACE_HOBBIT_FIRE_MOD        -1
#define RACE_HOBBIT_COLD_MOD        2
#define RACE_HOBBIT_ENERGY_MOD      0
#define RACE_HOBBIT_ACID_MOD        1
#define RACE_HOBBIT_MAGIC_MOD       0
#define RACE_HOBBIT_POISON_MOD      -1

#define RACE_GNOME_FIRE_MOD         -2
#define RACE_GNOME_COLD_MOD         -2
#define RACE_GNOME_ENERGY_MOD       3
#define RACE_GNOME_ACID_MOD         3
#define RACE_GNOME_MAGIC_MOD        0
#define RACE_GNOME_POISON_MOD       0

#define RACE_ORC_FIRE_MOD           6
#define RACE_ORC_COLD_MOD           -1
#define RACE_ORC_ENERGY_MOD         0
#define RACE_ORC_ACID_MOD           0
#define RACE_ORC_MAGIC_MOD          -3
#define RACE_ORC_POISON_MOD         0

#endif
