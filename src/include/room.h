#ifndef __ROOM_H__
#define __ROOM_H__
/************************************************************************
| $Id: room.h,v 1.1 2002/06/13 04:32:22 dcastle Exp $
| room.h
| Description:  This file contains all of the room header file/constant
|   information.  It also contains information about the 'world' structs.
*/
#include <structs.h> // byte
#include <obj.h> // byte

// The following defs are for room_data

#define NOWHERE    -1

/* Bitvector For 'room_flags' */

#define DARK         1
#define DEATH        1<<1    // unused
#define NO_MOB       1<<2
#define INDOORS      1<<3
#define LAWFULL      1<<4
#define NEUTRAL      1<<5
#define CHAOTIC      1<<6
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

/* For 'Sector types' */

#define SECT_INSIDE           0
#define SECT_CITY             1
#define SECT_FIELD            2
#define SECT_FOREST           3
#define SECT_HILLS            4
#define SECT_MOUNTAIN         5
#define SECT_WATER_SWIM       6
#define SECT_WATER_NOSWIM     7
/* These should be rflags
#define SECT_NO_LOW           8
#define SECT_NO_HIGH          9
*/
#define SECT_BEACH            8
#define SECT_PAVED_ROAD       9
#define SECT_DESERT          10
#define SECT_UNDERWATER      11
#define SECT_SWAMP           12
#define SECT_MAX_SECT        12 // update this if you add more

struct room_direction_data
{
    char *general_description;       /* When look DIR.                  */ 
    char *keyword;                   /* for open/close                  */  
    sh_int exit_info;                /* Exit info                       */
    sh_int key;                      /* Key's number (-1 for no key)    */
    sh_int to_room;                  /* Where direction leeds (NOWHERE) */
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


// ========================= Structure for room ==========================
struct room_data
{
    sh_int number;                       // Rooms number
    sh_int zone;                         // Room zone (for resetting)
    int sector_type;                     // sector type (move/hide)
    char * name;                         // Rooms name 'You are ...'
    char * description;                  // Shown when entered
    extra_descr_data * ex_description;   // for examine/look
    room_direction_data * dir_option[MAX_DIRS]; // Directions
    long room_flags;                     // DEATH, DARK ... etc
    sh_int light;                        // Light factor of room
    
    int (*funct)(CHAR_DATA*, int, char*);  // special procedure
	 
    struct obj_data *contents;   // List of items in room
    CHAR_DATA *people;           // List of NPC / PC in room
    
    int              nTracks;    // number of tracks in the room
    room_track_data* tracks;     // beginning of the list of scents
    room_track_data* last_track; // last in the scent list
    
    void              AddTrackItem(room_track_data * newTrack);
    room_track_data * TrackItem(int nIndex);
    void              FreeTracks();
};

#endif // __ROOM_H__
