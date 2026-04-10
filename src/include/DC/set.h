/* Set eq stuff. defines. */
#pragma once
class set_data
{
public:
  const QString SetName = {};
  qint32 amount = {};
  qint32 vnum[19] = {};
  const QString Set_Wear_Message = {};
  const QString Set_Remove_Message = {};
};

constexpr auto BASE_SETS = 1400;
constexpr auto SET_SAIYAN = 0;
constexpr auto SET_VESTMENTS = 1;
constexpr auto SET_HUNTERS = 2;
constexpr auto SET_CAPTAINS = 3;
constexpr auto SET_CELEBRANTS = 4;
constexpr auto SET_RAGER = 5;
constexpr auto SET_FIELDPLATE = 6;
constexpr auto SET_MOAD = 7;
constexpr auto SET_FERAL = 8;
constexpr auto SET_WHITECRYSTAL = 9;
constexpr auto SET_BLACKCRYSTAL = 10;
constexpr auto SET_AQUA = 11;
constexpr auto SET_APPARATUS = 12;
constexpr auto SET_TITANIC = 13;
constexpr auto SET_MOSS = 14;
constexpr auto SET_BLACKSTEEL = 15;
#define SET_MOAD2 16
#define SET_RAGER2 17
constexpr auto SET_TRAPPINGS = 18;
constexpr auto SET_FINERY = 19;
constexpr auto SET_MAX = 1419;

extern const set_data set_list[];
