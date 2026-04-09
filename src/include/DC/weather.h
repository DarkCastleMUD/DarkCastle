#pragma once
/************************************************************************
| $Id: weather.h,v 1.4 2005/06/06 21:54:02 shane Exp $
| weather.h
| Description:  Header information for weather interaction/info.
*/

/* How much light is in the land ? */

constexpr auto SUN_DARK = 0;
constexpr auto SUN_RISE = 1;
constexpr auto SUN_LIGHT = 2;
constexpr auto SUN_SET = 3;

/* And how is the sky ? */

constexpr auto SKY_CLOUDLESS = 0;
constexpr auto SKY_CLOUDY = 1;
constexpr auto SKY_RAINING = 2;
constexpr auto SKY_HEAVY_RAIN = 3;
constexpr auto SKY_LIGHTNING = 4;
constexpr auto SKY_SNOWING = 6; // unused
