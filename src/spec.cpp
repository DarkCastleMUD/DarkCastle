/*

  Specilization stuff

*/
extern "C"
{
#include <ctype.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <structs.h>
#include <room.h>
#include <character.h>
#include <obj.h>
#include <utility.h>
#include <terminal.h>
#include <player.h>
#include <levels.h>
#include <mobile.h>
#include <clan.h>
#include <handler.h>
#include <db.h>
#include <interp.h>
#include <connect.h>
#include <spells.h>
#include <race.h>
#include <act.h>
#include <set.h>
#include <returnvals.h>
#include <timeinfo.h>

extern CHAR_DATA *character_list;
extern OBJ_DATA *object_list;
extern CWorld world;
extern struct index_data *obj_index;


struct spec_data
{
  char *name;
  int pcclass;
  int skills[10];
};

const struct spec_data spec_list[] =
{
   { "Blah", 1, {0,0,0,0,0,0,0,0,0,0} },
   { NULL, 0,   {0,0,0,0,0,0,0,0,0,0} }

};

