#ifndef __ROOM_H__
#define __ROOM_H__
/************************************************************************
| $Id: room.h,v 1.21 2015/06/14 02:38:12 pirahna Exp $
| room.h
| Description:  This file contains all of the room header file/constant
|   information.  It also contains information about the 'world' structs.
*/
#include "structs.h" // ubyte
#include "obj.h" // ubyte
#include "MobActivity.h" // struct path_data
#include "player.h" // CLASS_MAX

// The following defs are for room_data

const unsigned int NOWHERE = 0;

/* Bitvector For 'room_flags' */

#define DARK         1
#define NOHOME        1<<1    
#define NO_MOB       1<<2
#define INDOORS      1<<3
#define TELEPORT_BLOCK  1<<4
#define NO_KI      1<<5
#define NOLEARN      1<<6
#define NO_MAGIC     1<<7
#define TUNNEL       1<<8
#define PRIVATE      1<<9
#define SAFE         1<<10
#define NO_SUMMON    1<<11
#define NO_ASTRAL    1<<12    // usused
#define NO_PORTAL    1<<13
#define IMP_ONLY     1<<14
#define FALL_DOWN    1<<15
#define ARENA        1<<16
#define QUIET        1<<17
#define UNSTABLE     1<<18
#define NO_QUIT      1<<19
#define FALL_UP      1<<20
#define FALL_EAST    1<<21
#define FALL_WEST    1<<22
#define FALL_SOUTH   1<<23
#define FALL_NORTH   1<<24
#define NO_TELEPORT  1<<25
#define NO_TRACK     1<<26
#define CLAN_ROOM    1<<27
#define NO_SCAN      1<<28
#define NO_WHERE     1<<29
#define LIGHT_ROOM   1<<30
//#define NOHOME       1<<31
//#define NO_KI        1<<31

/* Bitvector For 'temp_room_flags' 
   temp_room_flags are NOT in the world file and cannot be saved or added at boot time
   These are for runtime flags, such as ETHEREAL_FOCUS
*/

#define ROOM_ETHEREAL_FOCUS   1
#define TEMP_ROOM_FLAG_AVAILABLE 1<<1

/* Internal flags */
#define iNO_TRACK   1
#define iNO_MAGIC   1<<1

/* For 'dir_option' */

#define NORTH           0
#define EAST            1
#define SOUTH           2
#define WEST            3
#define UP              4
#define DOWN            5
#define MAX_DIRS        6

#define EX_ISDOOR       1
#define EX_CLOSED       2
#define EX_LOCKED       4
#define EX_HIDDEN       8
#define EX_IMM_ONLY    16
#define EX_PICKPROOF   32
#define EX_BROKEN      64

/* For 'Sector types' */

#define SECT_INSIDE           0
#define SECT_CITY             1
#define SECT_FIELD            2
#define SECT_FOREST           3
#define SECT_HILLS            4
#define SECT_MOUNTAIN         5
#define SECT_WATER_SWIM       6
#define SECT_WATER_NOSWIM     7
#define SECT_BEACH            8
#define SECT_PAVED_ROAD       9
#define SECT_DESERT          10
#define SECT_UNDERWATER      11
#define SECT_SWAMP           12
#define SECT_AIR             13
#define SECT_FROZEN_TUNDRA   14
#define SECT_ARCTIC          15
#define SECT_MAX_SECT        15 // update this if you add more
                                // and const.c stuff for sectors

struct room_direction_data
{
    char *general_description;      /* When look DIR.                  */ 
    char *keyword;                  /* for open/close                  */  
    int16 exit_info;                /* Exit info                       */
    CHAR_DATA *bracee;		    /* This is who is bracing the door */
    int16 key;                      /* Key's number (-1 for no key)    */
    int16 to_room;                  /* Where direction leeds (NOWHERE) */
};

struct room_track_data
{
    int weight;
    int race;
    int direction; 
    int sex;
    int condition;
    char *trackee;
    
    room_track_data * next;
    room_track_data * previous;
}; 

struct deny_data
{
  struct deny_data *next;
  int vnum;
};

// ========================= Structure for room ==========================
struct room_data
{
    int16 number;                       // Rooms number
    int16 zone;                         // Room zone (for resetting)
    int sector_type;                     // sector type (move/hide)
    struct deny_data *denied;
    char * name;                         // Rooms name 'You are ...'
    char * description;                  // Shown when entered
    extra_descr_data * ex_description;   // for examine/look
    room_direction_data * dir_option[MAX_DIRS]; // Directions
    uint32 room_flags;                     // DEATH, DARK ... etc
    uint32 temp_room_flags;             // A second bitvector for flags that do NOT get saved.  These are temporary runtime flags.
    int16 light;                        // Light factor of room
    
    int (*funct)(CHAR_DATA*, int, char*);  // special procedure
	 
    struct obj_data *contents;   // List of items in room
    CHAR_DATA *people;           // List of NPC / PC in room
    
    int              nTracks;    // number of tracks in the room
    room_track_data* tracks;     // beginning of the list of scents
    room_track_data* last_track; // last in the scent list
    
    void              AddTrackItem(room_track_data * newTrack);
    room_track_data * TrackItem(int nIndex);
    void              FreeTracks();
    int 	      iFlags; // Internal flags. These do NOT save.
    struct path_data *paths;
    bool              allow_class[CLASS_MAX];
};

#endif // __ROOM_H__
