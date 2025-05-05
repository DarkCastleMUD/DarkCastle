/***************************************************************************
 *  file: weather.c , Weather and time module              Part of DIKUMUD *
 *  Usage: Performing the clock and the weather                            *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: weather.cpp,v 1.14 2012/05/06 00:55:44 jhhudso Exp $ */

#include <cstdio>
#include <cstring>
#include "DC/timeinfo.h"
#include "DC/weather.h"
#include "DC/character.h"
#include "DC/utility.h"

// TODO - Either rip out the pressure stuff, or make it easier to understand.
// TODO - Add wind, and it's effects (on movement/combat etc)
// TODO - Add varying amounts of rain.  Heavy rain effects movement/combat etc.
// TODO - Add varying amounts of lightning.  On heavy electrical storms, random
//        lightning strikes should take place
// TODO - Add weather to the zonefiles so that each zone could have different weather
// TODO - Add dependancies between the zone weather files so that they interact and
//        effect each other.
// TODO - You can never have too many different messages:)

/* uses */

extern struct weather_data weather_info;

/* In ths part. */

void weather_change(void);

/* Here comes the code */

void DC::time_update(void)
{
   another_hour(1);
}

void weather_update()
{
   weather_change();
}

void DC::another_hour(int mode)
{
   time_info.hours++;

   if (mode)
   {
      switch (time_info.hours)
      {
      case 5:
         weather_info.sunlight = SUN_RISE;
         send_to_outdoor("The morning sky lightens to pale blue.\r\n");
         break;
      case 6:
         weather_info.sunlight = SUN_LIGHT;
         if (weather_info.sky == SKY_CLOUDLESS)
            send_to_outdoor("The sun crests the horizon sending down rays of warmth from the cloudless blue skies.\r\n");
         else
            send_to_outdoor("Orange flares join the sun touching the morning sky.\r\n");
         break;
      /* aderrick added code 5/3/12, case 8 */
      case 8:
         if (weather_info.sky == SKY_CLOUDLESS)
            send_to_outdoor("The sun rises higher into the sky casting brighter light across the land.\r\n");
         else
            send_to_outdoor("The sun continues to rise giving more heat and light.\r\n");
         break;
      case 11:
         send_to_outdoor("The shadows shift imperceptibly as another hour passes.\r\n");
         break;
      case 13:
         if (weather_info.sky == SKY_CLOUDLESS)
            send_to_outdoor("Finishing its daily climb, the sun burns brightly overhead and begins its descent.\r\n");
         break;
      case 15:
         send_to_outdoor("The shadows shift imperceptibly as another hour passes.\r\n");
         break;
      /* aderrick added code 5/3/12 case 19 & 20 */
      case 19:
         send_to_outdoor("The day begins to cool as evening arrives.\r\n");
         break;
      case 20:
         send_to_outdoor("The shadows get longer as the evening grows later.\r\n");
         break;
      case 21:
         weather_info.sunlight = SUN_SET;
         send_to_outdoor("The sun falls below the horizon as a biting cold sets in.\r\n");
         break;
      case 22:
         weather_info.sunlight = SUN_DARK;
         send_to_outdoor("The twin northern moons surface into the night sky.\r\n");
         break;
      default:
         break;
      }
   }

   if (time_info.hours > 23) /* Changed by HHS due to bug ???*/
   {
      time_info.hours -= 24;
      time_info.day++;

      if (time_info.day > 34)
      {
         time_info.day = 0;
         time_info.month++;

         if (time_info.month > 16)
         {
            time_info.month = 0;
            time_info.year++;
         }
      }
   }
}

void weather_change(void)
{
   int diff, change;
   if ((DC::getInstance()->time_info.month >= 9) && (DC::getInstance()->time_info.month <= 16))
      diff = (weather_info.pressure > 985 ? -2 : 2);
   else
      diff = (weather_info.pressure > 1015 ? -2 : 2);

   weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

   weather_info.change = MIN(weather_info.change, 12);
   weather_info.change = MAX(weather_info.change, -12);

   weather_info.pressure += weather_info.change;

   weather_info.pressure = MIN(weather_info.pressure, 1040);
   weather_info.pressure = MAX(weather_info.pressure, 960);

   change = 0;

   switch (weather_info.sky)
   {
   case SKY_CLOUDLESS:
      if (weather_info.pressure < 990)
         change = 1;
      else if (weather_info.pressure < 1010)
      {
         if (dice(1, 4) == 1)
            change = 1;
      }
      break;
   case SKY_CLOUDY:
      if (weather_info.pressure < 970)
         change = 2;
      else if (weather_info.pressure < 990)
      {
         if (dice(1, 4) == 1)
            change = 2;
         else
            change = 0;
      }
      else if (weather_info.pressure > 1030)
      {
         if (dice(1, 4) == 1)
            change = 3;
      }
      break;
   case SKY_RAINING:
      if (weather_info.pressure < 970)
      {
         if (dice(1, 4) == 1)
            change = 4;
         else if (dice(1, 5) == 1)
            change = 7;
         else
            change = 0;
      }
      else if (weather_info.pressure > 1030)
         change = 5;
      else if (weather_info.pressure > 1010)
      {
         if (dice(1, 4) == 1)
            change = 5;
      }
      break;
   case SKY_HEAVY_RAIN:
      if (weather_info.pressure < 960)
      {
         if (dice(1, 4) == 1)
            change = 8;
         else if (dice(1, 3) == 1)
            change = 9;
         else
            change = 0;
      }
      else if (weather_info.pressure < 1010)
      {
         if (dice(1, 4) == 1)
            change = 8;
         else
            change = 0;
      }
      else if (weather_info.pressure > 1030)
         change = 5;
      else if (weather_info.pressure > 1010)
         if (dice(1, 3) == 1)
            change = 5;
      break;
   case SKY_LIGHTNING:
      if (weather_info.pressure > 1010)
         change = 6;
      else if (weather_info.pressure > 990)
         if (dice(1, 4) == 1)
            change = 6;
      break;
   default:
      change = 0;
      weather_info.sky = SKY_CLOUDLESS;
      break;
   }

   switch (change)
   {
   case 0:
      break;
   case 1:
      switch (number(1, 3))
      {
      case 1:
         send_to_outdoor("A bitter chill sets in as billowing clouds form on the horizon.\r\n");
         break;
      case 2:
         send_to_outdoor("Dark and heavy clouds begin to crowd the sky, blocking off the light of the heavens.\r\n");
         break;
      case 3:
         if (weather_info.sunlight != SUN_DARK)
            send_to_outdoor("The sun's light diffuses as clouds roll in to create a pale and dreary scape.\r\n");
         break;
      }
      weather_info.sky = SKY_CLOUDY;
      break;
   case 2:
      switch (number(1, 2))
      {
      case 1:
         send_to_outdoor("Small circlets dot the ground as a light rain begins.\r\n");
         break;
      case 2:
         send_to_outdoor("A light mist hangs in the air and slowly turns to rain.\r\n");
         break;
      }
      weather_info.sky = SKY_RAINING;
      break;
   case 3:
      switch (number(1, 2))
      {
      case 1:
         send_to_outdoor("The southern sky slips into view as the clouds fade.\r\n");
         break;
      case 2:
         send_to_outdoor("The clouds overhead slowly move off to the horizon clearing the sky above.\r\n");
         break;
      }
      weather_info.sky = SKY_CLOUDLESS;
      break;
   case 4:
      switch (number(1, 3))
      {
      case 1:
         send_to_outdoor("An eerie flash of lightning crackles across the sky.\r\n");
         break;
      case 2:
         send_to_outdoor("The rain begins to fall harder and lightning streaks across the sky.\r\n");
         break;
      case 3:
         send_to_outdoor("The storm worsens, bringing the crashing of thunder and flashes of lightning.\r\n");
         break;
      }
      weather_info.sky = SKY_LIGHTNING;
      break;
   case 5:
      switch (number(1, 2))
      {
      case 1:
         send_to_outdoor("Like a hushed whisper, the pattering rain has lifted.\r\n");
         break;
      case 2:
         send_to_outdoor("The rain begins to slow as the clouds end their assault.\r\n");
         break;
      }
      weather_info.sky = SKY_CLOUDY;
      break;
   case 6:
      switch (number(1, 2))
      {
      case 1:
         send_to_outdoor("A last ominous rumble and the sky's energy is dispelled.\r\n");
         break;
      case 2:
         send_to_outdoor("The rain begins to lessen and the rumble of thunder moves off to the distance.\r\n");
         break;
      }
      weather_info.sky = SKY_RAINING;
      break;
   case 7:
      switch (number(1, 2))
      {
      case 1:
         send_to_outdoor("The clouds open up and send a torrent of rain to the planet.\r\n");
         break;
      case 2:
         send_to_outdoor("The rain drops begin slamming down heavily.\r\n");
         break;
      }
      weather_info.sky = SKY_HEAVY_RAIN;
      break;
   case 8:
      switch (number(1, 2))
      {
      case 1:
         send_to_outdoor("The rain eases slightly, assailing the ground with less force.\r\n");
         break;
      case 2:
         send_to_outdoor("The clouds overhead become a few shades lighter, lessening the downpour.\r\n");
         break;
      }
      weather_info.sky = SKY_RAINING;
      break;
   case 9:
      switch (number(1, 3))
      {
      case 1:
         send_to_outdoor("The energy of the clouds increases as lightning bolts rain down.\r\n");
         break;
      case 2:
         send_to_outdoor("Occasional sounds of thunder are heard over the deluge.\r\n");
         break;
      case 3:
         send_to_outdoor("The electrical activity within the clouds rises noticeably.\r\n");
         break;
      }
      weather_info.sky = SKY_LIGHTNING;
      break;
   default:
      break;
   }
}
