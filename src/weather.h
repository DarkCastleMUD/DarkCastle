#ifndef WEATHER_H_
#define WEATHER_H_
/************************************************************************
| $Id: weather.h,v 1.4 2005/06/06 21:54:02 shane Exp $
| weather.h
| Description:  Header information for weather interaction/info.
*/

/* How much light is in the land ? */

#define SUN_DARK     0
#define SUN_RISE     1
#define SUN_LIGHT    2
#define SUN_SET      3

/* And how is the sky ? */

#define SKY_CLOUDLESS    0
#define SKY_CLOUDY       1
#define SKY_RAINING      2
#define SKY_HEAVY_RAIN   3
#define SKY_LIGHTNING    4
#define SKY_SNOWING      6   // unused

struct weather_data
{
    int pressure;      // How is the pressure ( Mb ) 
    int change;        // How fast and what way does it change. 
    int sky;           // How is the sky. 
    int sunlight;      // And how much sun. 

    // following are usused at this time
    int windspeed;     // How fast wind is blowing
    int winddirection; // What direction it is blowing in
    int temperature;   // Duh...
    int modifiers;     // fog?  Ice?
};

#endif
