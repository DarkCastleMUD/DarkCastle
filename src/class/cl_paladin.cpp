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

   if (!charge_moves(ch, SKILL_HARMTOUCH_MOVES, SKILL_HARM_TOUCH)) return eSUCCESS;


   int duration = 24;
   if(!skill_success(ch,victim,SKILL_HARM_TOUCH)) {
     send_to_char("Your god refuses you.\r\n", ch);
     duration = 12;
     WAIT_STATE(ch, PULSE_VIOLENCE/2 + number(1, PULSE_VIOLENCE/2));
   }
   else {
     dam = 750;
     retval = damage(ch, victim, dam, TYPE_ACID, SKILL_HARM_TOUCH, 0);
     WAIT_STATE(ch, PULSE_VIOLENCE);
     if(IS_SET(retval, eVICT_DIED) && !IS_SET(retval, eCH_DIED)) {
        if(has_skill(ch,SKILL_HARM_TOUCH) > 30 && number(1, 3) == 1) {
           char dammsg[MAX_STRING_LENGTH];
           int amount = GET_LEVEL(ch) * 10;
           if(amount + GET_HIT(ch) > GET_MAX_HIT(ch)) amount = GET_MAX_HIT(ch) - GET_HIT(ch);
           sprintf(dammsg, "$B%d$R", amount);
           send_damage("Your god basks in your worship of pain and infuses you with | life.", ch, 0, victim, dammsg, "You god basks in your worship of pain and infuses you with life.", TO_CHAR);
           GET_HIT(ch) += amount;
        }
     }
   }
   af.type = SKILL_HARM_TOUCH;
   af.duration  = duration;
   af.modifier  = 0;
   af.location  = APPLY_NONE;
   af.bitvector = -1;
   affect_to_char(ch, &af);


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
   int duration = 24;
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

//   if (ch->fighting == victim) {
//     send_to_char("Aren't you a little busy trying to KILL them right now?\n\r",ch);
//     return eFAILURE;
//   }

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

   if (!charge_moves(ch, SKILL_LAYHANDS_MOVES, SKILL_LAY_HANDS)) return eSUCCESS;

   if(!skill_success(ch,victim, SKILL_LAY_HANDS)) {
     send_to_char("Your god refuses you.\r\n", ch);
     duration /= 2;
   }
   else {
     char dammsg[MAX_STRING_LENGTH];
     int amount = 500 + (has_skill(ch, SKILL_LAY_HANDS)*10);
     if(amount + GET_HIT(victim) > GET_MAX_HIT(victim))
       amount = GET_MAX_HIT(victim) - GET_HIT(victim);
     GET_HIT(victim) += amount;
     sprintf(dammsg, "$B%d$R", amount);
     send_damage("Praying fervently, you lay hands as life force granted by your god streams from your body healing $N for | health.",
ch, 0, victim, dammsg, "Praying fervently, you lay hands as life force granted by your god streams from your body into $N.", TO_CHAR);
     send_damage("Your body surges with holy energies as | points of life force granted by $n's god pours into you!", 
ch, 0, victim, dammsg, "Your body surges with holy energies as life force granted by $n's god pours into you!", TO_VICT);
     send_damage("A blinding flash fills the area as | points of life force granted from $n's god pours into $N!", ch, 0, 
victim, dammsg, "A blinding flash fills the area as life force granted from $n's god pours into $N!", TO_ROOM);
   }

   af.type = SKILL_LAY_HANDS;
   af.duration  = duration;
   af.modifier  = 0;
   af.location  = APPLY_NONE;
   af.bitvector = -1;
   affect_to_char(ch, &af);
   return eSUCCESS;
}

