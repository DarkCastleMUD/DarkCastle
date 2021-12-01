#ifndef CLASS_H
#define CLASS_H
#include <string>
#include <vector>

using namespace std;

/************************************************************************
| Class types for PCs
*/
#define CLASS_MAGIC_USER   1
#define CLASS_MAGE 	       1 // Laziness > consistency
#define CLASS_CLERIC       2
#define CLASS_THIEF        3
#define CLASS_WARRIOR      4
#define CLASS_ANTI_PAL     5
#define CLASS_PALADIN      6
#define CLASS_BARBARIAN    7
#define CLASS_MONK         8
#define CLASS_RANGER       9
#define CLASS_BARD        10
#define CLASS_DRUID       11
#define CLASS_MAX_PROD    11
#define CLASS_PSIONIC     12
#define CLASS_NECROMANCER 13
#define CLASS_MAX         13

struct class_data
{
    string name;
    string lname;
    string abbrev;
    bool playable;
    int8_t min_str;
    int8_t min_dex;
    int8_t min_con;
    int8_t min_int;
    int8_t min_wis;
};

typedef vector<struct class_data> classes_t;

extern classes_t classes;

#endif