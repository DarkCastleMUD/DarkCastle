// This file takes care of all innate race abilities

#include <innate.h>
#include <race.h>
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

#include <vector>
#include <algorithm>
using namespace std;

////////////////////////////////////////////////////////////////////////////
// external vars
extern CWorld world;
extern struct index_data *obj_index; 

////////////////////////////////////////////////////////////////////////////
// external functs

////////////////////////////////////////////////////////////////////////////
// local function declarations
int do_innate_pixie(CHAR_DATA *ch, char *arg, int cmd);

int do_innate_fly(CHAR_DATA *ch, char *arg, int cmd);

////////////////////////////////////////////////////////////////////////////
// local definitions

char * innate_skills[] = 
{
   "innate fly timer",
   "\n"
};

////////////////////////////////////////////////////////////////////////////
// command functions

int do_innate(CHAR_DATA *ch, char *arg, int cmd)
{
   switch(GET_RACE(ch)) {
      case RACE_PIXIE:    return do_innate_pixie(ch, arg, cmd);
      default:
         send_to_char("You do not have any innate powers!\r\n", ch);
         return eSUCCESS;
   }
}

int do_innate_pixie(CHAR_DATA *ch, char *arg, int cmd)
{
   char buf[MAX_INPUT_LENGTH];

   one_argument(arg, buf);

   if(!*buf)
   {
      send_to_char("As a pixie, you have the following abilities:\r\n"
                   "   fly\r\n"
                   "\r\n"
                   "You can activate your innate ability with innate <skill>.\r\n"
                   , ch);
      return eSUCCESS;
   }

   if(!strcmp(buf, "fly"))
      return do_innate_fly(ch, buf, cmd);
   else
   {
      csendf(ch, "You do not know of any '%s' ability.\r\n", buf);
      return eSUCCESS;
   }
}


///////////////////////////////////////////////////////////////////////
//  actual innate abilities

int do_innate_fly(CHAR_DATA *ch, char *arg, int cmd)
{
   if(affected_by_spell(ch, SKILL_INNATE_FLY))
   {
      send_to_char("It is still too soon for you to be able to call upon your ancestral powers again.\r\n", ch);
      return eSUCCESS;
   }

   if(IS_AFFECTED(ch, AFF_FLYING))
   {
      send_to_char("But you are already flying!  Why waste it?\r\n", ch);
      return eSUCCESS;
   }

   struct affected_type af;
   af.type = SKILL_INNATE_FLY;
   af.duration = 40;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   return spell_fly( ( GET_LEVEL(ch) / 2 ), ch, ch, 0, GET_LEVEL(ch) );
}
