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
/* $Id: weather.cpp,v 1.1 2002/06/13 04:32:19 dcastle Exp $ */

extern "C"
{
   #include <stdio.h>
   #include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <timeinfo.h>
#include <weather.h>
#include <utility.h>

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

extern struct time_info_data time_info;
extern struct weather_data weather_info;

/* In ths part. */

void another_hour(int mode);
void weather_change(void);

/* Here comes the code */

void time_update()
{
   another_hour(1);
}

void weather_update()
{
   weather_change();
}

void another_hour(int mode)
{
   time_info.hours++;
   
   if (mode) {
      switch (time_info.hours) 
      {
         case 5 :
            weather_info.sunlight = SUN_LIGHT;
            send_to_outdoor("The morning sky lightens to pale blue.\n\r");
            break;  
         case 6 :
            weather_info.sunlight = SUN_RISE;
            if(weather_info.sky == SKY_CLOUDLESS)
              send_to_outdoor("The sun crests the horizon sending down rays of warmth from the cloudless blue skies.\n\r");
            else send_to_outdoor("Orange flares join the sun touching the morning sky.\n\r");
            break;
         case 11 :
            send_to_outdoor("The shadows shift imperceptably as another hour passes.\n\r");
            break;
         case 13 :
            if(weather_info.sky == SKY_CLOUDLESS)
              send_to_outdoor("Finishing it's daily climb, the sun burns brightly overhead and begins it's descent.\n\r");
            break;
         case 15 :
            send_to_outdoor("The shadows shift imperceptably as another hour passes.\n\r");
            break;
         case 21 :
            weather_info.sunlight = SUN_SET;
            send_to_outdoor("The sun falls below the horizon as a biting cold sets in.\n\r");
            break;
         case 22 :
            weather_info.sunlight = SUN_DARK;
            send_to_outdoor("The twin northern moons surface into the night sky.\n\r");
            break;
         default : break;
      }
   }
   
   if (time_info.hours > 23)  /* Changed by HHS due to bug ???*/
   {
      time_info.hours -= 24;
      time_info.day++;
      
      if (time_info.day>34)
      {
         time_info.day = 0;
         time_info.month++;
         
         if(time_info.month>16)
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
   if((time_info.month>=9)&&(time_info.month<=16))
      diff=(weather_info.pressure>985 ? -2 : 2);
   else
      diff=(weather_info.pressure>1015? -2 : 2);
   
   weather_info.change += (dice(1,4)*diff+dice(2,6)-dice(2,6));
   
   weather_info.change = MIN(weather_info.change,12);
   weather_info.change = MAX(weather_info.change,-12);
   
   weather_info.pressure += weather_info.change;
   
   weather_info.pressure = MIN(weather_info.pressure,1040);
   weather_info.pressure = MAX(weather_info.pressure,960);
   
   change = 0;
   
   switch(weather_info.sky)
   {
      case SKY_CLOUDLESS :
         if (weather_info.pressure<990)
            change = 1;
         else if (weather_info.pressure<1010) {
            if(dice(1,4)==1)
               change = 1;
         }
         break;
      case SKY_CLOUDY :
         if (weather_info.pressure<970)
            change = 2;
         else if (weather_info.pressure<990) {
            if(dice(1,4)==1)
               change = 2;
            else
               change = 0;
         }
         else if (weather_info.pressure>1030) {
            if(dice(1,4)==1)
               change = 3;
         }
         break;
      case SKY_RAINING :
         if (weather_info.pressure<970) {
            if(dice(1,4)==1)
               change = 4;
            else
               change = 0;
         }
         else if (weather_info.pressure>1030)
            change = 5;
         else if (weather_info.pressure>1010) {
            if(dice(1,4)==1)
               change = 5;
         }
         break;
      case SKY_LIGHTNING :
         if (weather_info.pressure>1010)
            change = 6;
         else if (weather_info.pressure>990) {
            if(dice(1,4)==1)
               change = 6;
         }
         break;
      default : 
         change = 0;
         weather_info.sky=SKY_CLOUDLESS;
         break;
   }
   
   switch(change)
   {
      case 0 : 
         break;
      case 1 :
         switch(number(1, 2)) {
         case 1:  send_to_outdoor("A bitter chill sets in as billowing clouds form on the horizon.\n\r"); break;
         case 2:  send_to_outdoor("Dark and heavy clouds begin to crowd the sky, blocking off the light of the heavens.\n\r"); break;
         }
         weather_info.sky=SKY_CLOUDY;
         break;
      case 2 :
         switch(number(1, 2)) {
         case 1:  send_to_outdoor("Small circlets dot the ground as a light rain begins.\n\r"); break;
         case 2:  send_to_outdoor("A light mist hangs in the air and slowly turns to rain.\n\r"); break;
         }
         weather_info.sky=SKY_RAINING;
         break;
      case 3 :
         switch(number(1, 2)) {
         case 1:  send_to_outdoor("The southern sky slips into view as the clouds fade.\n\r"); break;
         case 2:  send_to_outdoor("The clouds overhead slowly move off to the horizon clearing the sky above.\n\r"); break;
         }
         weather_info.sky=SKY_CLOUDLESS;
         break;
      case 4 :
         switch(number(1, 3)) {
         case 1:  send_to_outdoor("An eerie flash of lightning crackles across the sky.\n\r"); break;
         case 2:  send_to_outdoor("The rain begins to fall harder and lightning streaks across the sky.\n\r"); break;
         case 3:  send_to_outdoor("The storm worsens, bringing the crashing of thunder and flashes of lightning.\n\r"); break;
         }
         weather_info.sky=SKY_LIGHTNING;
         break;
      case 5 :
         switch(number(1, 2)) {
         case 1:  send_to_outdoor("Like a hushed whisper, the pattering rain has lifted.\n\r"); break;
         case 2:  send_to_outdoor("The rain begins to slow as the clouds end their assault.\n\r"); break;
         }
         weather_info.sky=SKY_CLOUDY;
         break;
      case 6 :
         switch(number(1, 2)) {
         case 1:  send_to_outdoor("A last ominous rumble and the sky's energy is dispelled.\n\r"); break;
         case 2:  send_to_outdoor("The rain begins to lessen and the rumble of thunder moves off to the distance.\n\r"); break;
         }
         weather_info.sky=SKY_RAINING;
         break;
      default : break;
   }
}
