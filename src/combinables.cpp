// This file takes care of all the skills that make stuff by combining it
// in it's container type.  For example, poison making.


#include <db.h>
#include <fight.h>
#include <room.h>
#include <obj.h>
#include <connect.h>
#include <utility.h>
#include <character.h>
#include <handler.h>
#include <db.h>
#include <player.h>
#include <levels.h>
#include <interp.h>
#include <magic.h>
#include <act.h>
#include <mobile.h>
#include <spells.h>
#include <string.h> // strstr()
#include <returnvals.h>

extern CWorld world;
extern struct index_data *obj_index; 

char * tradeskills[] = 
{
  "poison making",
  "\n"
};


