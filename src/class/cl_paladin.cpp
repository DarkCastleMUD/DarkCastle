/************************************************************************
| cl_warrior.C
| Description:  This file declares implementation for (anti)paladin-specific
|   skills.
|
| File create with do_layhands -Pirahna 7/6/1999
*/
#include <structs.h>
#include <character.h>
#include <player.h>
#include <fight.h>
#include <utility.h>
#include <spells.h>
#include <handler.h>
#include <levels.h>
#include <connect.h>
#include <mobile.h>
#include <room.h>
#include <act.h>
#include <db.h>
#include <returnvals.h>

extern CWorld world;
 

/************************************************************************
| OFFENSIVE commands.  These are commands that should require the
|   victim to retaliate.
*/

// Note that most of the (anti)paladin skills are already in "cl_warrior.C"

int do_harmtouch(struct char_data *ch, char *argument, int cmd)
{
   struct char_data *victim;
   // struct char_data *tmp_ch;
   char victim_name[MAX_INPUT_LENGTH];
   struct affected_type af;
   int retval, dam;

   one_argument(argument, victim_name);

   if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL)
      ;
   else if(!has_skill(ch, SKILL_HARM_TOUCH)) {
      send_to_char("You dunno even HOW to harm touch.\r\n", ch);
      return eFAILURE;
   }

   if (!(victim = get_char_room_vis(ch, victim_name))) 
   {
     victim = ch->fighting;
     if(!victim) 
     {
       send_to_char("Whom do you want to harmtouch?\n\r", ch);
       return eFAILURE;
     }
   }

   if (victim == ch) {
     if(GET_SEX(ch) == SEX_MALE)
       send_to_char("You'd wither it!\n\r", ch);
     else if(GET_SEX(ch) == SEX_FEMALE)
       send_to_char("You naughty naughty girl...at least wait until someone's filming.\r\n", ch);
     else 
       send_to_char("Looks like you've already harm touched yourself...\r\n", ch);
     return eFAILURE;
   }

   if (affected_by_spell(ch, SKILL_HARM_TOUCH) && GET_LEVEL(ch) <= IMMORTAL) {
      send_to_char("You have not spend enough time in devotion to your god to warrant such a favor yet.\r\n", ch);
      return eFAILURE;
   }

   // Can't do it in !magic rooms....
   if(IS_SET(world[ch->in_room].room_flags, NO_MAGIC)) {
      send_to_char("The area seems to prohibit you from such magical actions!\r\n", ch);
      return eFAILURE;
   }

   if(GET_HIT(ch) < GET_MAX_HIT(ch) / 4)
   {
      send_to_char("You don't posess the energy to do it!\r\n", ch);
      return eFAILURE;
   }

   skill_increase_check(ch, SKILL_HARM_TOUCH, has_skill(ch,SKILL_HARM_TOUCH), SKILL_INCREASE_EASY);

   af.type = SKILL_HARM_TOUCH;
   af.duration  = 24;
   af.modifier  = 0;
   af.location  = APPLY_NONE;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   if(!skill_success(ch,victim,SKILL_HARM_TOUCH)) {
     send_to_char("Your god refuses you.\r\n", ch);
   }
   else {
     dam = 750;
     retval = damage(ch, victim, dam, TYPE_UNDEFINED, SKILL_HARM_TOUCH, 0);
   }
   if(IS_SET(retval, eVICT_DIED) && !IS_SET(retval, eCH_DIED)) {
     if(has_skill(ch,SKILL_HARM_TOUCH) > 30 && number(1, 3) == 1) {
        send_to_char("Your god basks in your worship of pain and infuses you with life.\r\n", ch);
        GET_HIT(ch) += GET_LEVEL(ch) * 10;
        GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));
     }
   }
   return retval;
}


/************************************************************************
| NON-OFFENSIVE commands.  Below here are commands that should -not-
|   require the victim to retaliate.
*/

// Again note that alot of them are in cl_warrior.C

int do_layhands(struct char_data *ch, char *argument, int cmd)
{
   struct char_data *victim;
   // struct char_data *tmp_ch;
   char victim_name[240];
   struct affected_type af;

   one_argument(argument, victim_name);

   if(IS_MOB(ch) || GET_LEVEL(ch) >= ARCHANGEL )
     ;
   else if(!has_skill(ch, SKILL_LAY_HANDS)) {
     send_to_char("You aren't skilled enough to lay a two-dollar whore with three bucks.\r\n", ch);
     return eFAILURE;
   }

   if (!(victim = get_char_room_vis(ch, victim_name))) {
     send_to_char("Whom do you want to layhands on?\n\r", ch);
     return eFAILURE;
   }

   if (victim == ch) {
     send_to_char("Oh yeah...that's really holy....pervert...\n\r", ch);
     return eFAILURE;
   }

   if (ch->fighting == victim) {
     send_to_char("Aren't you a little busy trying to KILL them right now?\n\r",ch);
     return eFAILURE;
   }

   if (affected_by_spell(ch, SKILL_LAY_HANDS)) {
      send_to_char("You have not spent enough time in devotion to your god to warrant such a favor yet.\r\n", ch);
      return eFAILURE;
   }

   // Can't do it in !magic rooms....
   if(IS_SET(world[ch->in_room].room_flags, NO_MAGIC)) {
      send_to_char("The area seems to prohibit you from giving such magical assistance!\r\n", ch);
      return eFAILURE;
   }

   if(GET_HIT(ch) < GET_MAX_HIT(ch) / 4)
   {
      send_to_char("You don't posess the energy to do it!\r\n", ch);
      return eFAILURE;
   }

   skill_increase_check(ch, SKILL_LAY_HANDS, has_skill(ch,SKILL_LAY_HANDS), SKILL_INCREASE_EASY);

   if(!skill_success(ch,victim, SKILL_LAY_HANDS)) {
     send_to_char("Your god refuses you.\r\n", ch);
   }
   else {
     GET_HIT(victim) += 1000;
     if(GET_HIT(victim) > GET_MAX_HIT(victim))
       GET_HIT(victim) = GET_MAX_HIT(victim);

     send_to_char("Praying fervently to your god, you lay hands as life force streams from your body.", ch);
     act("Your body surges with holy wrath as $N's life force pours into you!", victim, 0, ch, TO_CHAR, 0);
     act("A blinding flash fills the area as $n's life force pours from $s body into $N", ch, 0, victim, TO_ROOM, NOTVICT);
   }

   af.type = SKILL_LAY_HANDS;
   af.duration  = 72;
   af.modifier  = 0;
   af.location  = APPLY_NONE;
   af.bitvector = 0;
   affect_to_char(ch, &af);
   return eSUCCESS;
}

