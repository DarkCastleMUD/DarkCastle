/* Set eq stuff. Struct, defines. */


struct set_data
{
  char *SetName;
  int amount;
  int vnum[19];
  char *Set_Wear_Message;
  char *Set_Remove_Message;
};

#define BASE_SETS        1200
#define SET_SAIYAN          0
#define SET_VESTMENTS       1
#define SET_HUNTERS         2
#define SET_CAPTAINS        3
#define SET_CELEBRANTS      4
#define SET_RAGER           5
#define SET_FIELDPLATE      6
#define SET_MOAD            7
#define SET_FERAL           8
#define SET_WHITECRYSTAL    9
#define SET_BLACKCRYSTAL   10
#define SET_AQUA           11
#define SET_APPARATUS      12
#define SET_TITANIC        13
#define SET_MOSS           14
#define SET_BLACKSTEEL     15
#define SET_MOAD2          16
#define SET_RAGER2         17
#define SET_TRAPPINGS      18
#define SET_FINERY         19
#define SET_MAX          1219

extern const struct set_data set_list[];
