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
int do_innate_hobbit(CHAR_DATA *ch, char *arg, int cmd);
int do_innate_dwarf(CHAR_DATA *ch, char *arg, int cmd);
int do_innate_orc(CHAR_DATA *ch, char *arg, int cmd);

int do_innate_fly(CHAR_DATA *ch, char *arg, int cmd);
int do_innate_sneak(CHAR_DATA *ch, char *arg, int cmd);
int do_innate_infra(CHAR_DATA *ch, char *arg, int cmd);
int do_innate_bloodlust(CHAR_DATA *ch, char *arg, int cmd);

////////////////////////////////////////////////////////////////////////////
// local definitions

char * innate_skills[] = 
{
   "innate fly timer",
   "innate sneak timer",
   "innate infra timer",
   "innate blodlst timer",
   "\n"
};

////////////////////////////////////////////////////////////////////////////
// command functions

int do_innate(CHAR_DATA *ch, char *arg, int cmd)
{
   switch(GET_RACE(ch)) {
      case RACE_PIXIE:    return do_innate_pixie(ch, arg, cmd);
      case RACE_HOBBIT:   return do_innate_hobbit(ch, arg, cmd);
      case RACE_DWARVEN:  return do_innate_dwarf(ch, arg, cmd);
      case RACE_ORC:      return do_innate_orc(ch, arg, cmd);
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

int do_innate_hobbit(CHAR_DATA *ch, char *arg, int cmd)
{
   char buf[MAX_INPUT_LENGTH];

   one_argument(arg, buf);

   if(!*buf)
   {
      send_to_char("As a hobbit, you have the following abilities:\r\n"
                   "   sneak\r\n"
                   "\r\n"
                   "You can activate your innate ability with innate <skill>.\r\n"
                   , ch);
      return eSUCCESS;
   }

   if(!strcmp(buf, "sneak"))
      return do_innate_sneak(ch, buf, cmd);
   else
   {
      csendf(ch, "You do not know of any '%s' ability.\r\n", buf);
      return eSUCCESS;
   }
}

int do_innate_dwarf(CHAR_DATA *ch, char *arg, int cmd)
{
   char buf[MAX_INPUT_LENGTH];

   one_argument(arg, buf);

   if(!*buf)
   {
      send_to_char("As a dwarf, you have the following abilities:\r\n"
                   "   infravision\r\n"
                   "\r\n"
                   "You can activate your innate ability with innate <skill>.\r\n"
                   , ch);
      return eSUCCESS;
   }

   if(!strcmp(buf, "infravision"))
      return do_innate_infra(ch, buf, cmd);
   else
   {
      csendf(ch, "You do not know of any '%s' ability.\r\n", buf);
      return eSUCCESS;
   }
}

int do_innate_orc(CHAR_DATA *ch, char *arg, int cmd)
{
   char buf[MAX_INPUT_LENGTH];

   one_argument(arg, buf);

   if(!*buf)
   {
      send_to_char("As an orc, you have the following abilities:\r\n"
                   "   bloodlust\r\n"
                   "\r\n"
                   "You can activate your innate ability with innate <skill>.\r\n"
                   , ch);
      return eSUCCESS;
   }

   if(!strcmp(buf, "bloodlust"))
      return do_innate_bloodlust(ch, buf, cmd);
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

int do_innate_infra(CHAR_DATA *ch, char *arg, int cmd)
{
   if(affected_by_spell(ch, SKILL_INNATE_INFRA))
   {
      send_to_char("It is still too soon for you to be able to call upon your ancestral powers again.\r\n", ch);
      return eSUCCESS;
   }

   if(IS_AFFECTED(ch, AFF_INFRARED))
   {
      send_to_char("But you are already seeing in the dark!  Why waste it?\r\n", ch);
      return eSUCCESS;
   }

   struct affected_type af;
   af.type = SKILL_INNATE_INFRA;
   af.duration = 40;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   return spell_infravision( 4, ch, ch, 0, GET_LEVEL(ch) );
}

int do_innate_sneak(CHAR_DATA *ch, char *arg, int cmd)
{
   if(affected_by_spell(ch, SKILL_INNATE_SNEAK))
   {
      send_to_char("It is still too soon for you to be able to call upon your ancestral powers again.\r\n", ch);
      return eSUCCESS;
   }

   if(IS_AFFECTED(ch, AFF_SNEAK))
   {
      send_to_char("But you are already sneaking!  Why waste it?\r\n", ch);
      return eSUCCESS;
   }

   send_to_char("You begin sneaking.\r\n", ch);

   struct affected_type af;
   af.type = SKILL_SNEAK;
   af.duration = MAX(5, GET_LEVEL(ch) / 2);
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = AFF_SNEAK;
   affect_to_char(ch, &af);

   af.type = SKILL_INNATE_SNEAK;
   af.duration = 40;
   af.modifier = 0;
   af.location = 0;   
   af.bitvector = 0;
   affect_to_char(ch, &af);

   return eSUCCESS;   
}

int do_innate_bloodlust(CHAR_DATA *ch, char *arg, int cmd)
{
   if(affected_by_spell(ch, SKILL_INNATE_BLOODLUST))
   {
      send_to_char("It is still too soon for you to be able to call upon your ancestral powers again.\r\n", ch);
      return eSUCCESS;
   }

   if(!ch->combat)
   {
      send_to_char("You must be in combat to bring forth a bloodlust.\r\n", ch);
      return eSUCCESS;
   }

   struct affected_type af;
   af.type = SKILL_INNATE_BLOODLUST;
   af.duration = 40;
   af.modifier = 0;
   af.location = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   return eSUCCESS;
}

