// File: obj_proc.C
// Usage: This file contains special procedures pertaining to objects, except 
// for the boards which are in board.C

#include <vector>
#include <string>
#include <sstream>

#include <string.h> // strstr()

#include "db.h"
#include "fight.h"
#include "room.h"
#include "obj.h"
#include "connect.h"
#include "timeinfo.h"
#include "utility.h"
#include "character.h"
#include "handler.h"
#include "db.h"
#include "player.h"
#include "levels.h"
#include "sing.h"
#include "interp.h"
#include "magic.h"
#include "act.h"
#include "mobile.h"
#include "spells.h"
#include "returnvals.h"
#include "set.h"
#include "arena.h"
#include "race.h"
#include "const.h"
#include "inventory.h"
#include "guild.h"

#define EMOTING_FILE "emoting-objects.txt"

extern CWorld world;
extern struct index_data *obj_index; 
extern struct index_data *mob_index; 
extern struct obj_data *object_list;

extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;

extern CHAR_DATA *initiate_oproc(CHAR_DATA *ch, OBJ_DATA *obj);

extern void reset_zone(int zone);

extern struct mprog_throw_type *g_mprog_throw_list;



// TODO - go over emoting object stuff and make sure it's as efficient as we can get it
 
struct obj_emote_data {
    char *emote_text;
    obj_emote_data *next;
};

struct obj_emote_index {
    obj_emote_data *data;
    obj_emote_index *next;
    short room_number;
    short emote_index_length;
    short frequency;
};
struct obj_emote_index obj_emote_head = {
    NULL,
    NULL,
    -1,
    -1 
    -1
};

void free_emoting_obj_data(obj_emote_index * myobj)
{
   obj_emote_data * curr_data = NULL;

   while(myobj->data)
   {
     curr_data = myobj->data;
     myobj->data = curr_data->next;

     dc_free(curr_data->emote_text);
     dc_free(curr_data);
   }
}

void free_emoting_objects_from_memory()
{
  obj_emote_index * curr_index = NULL;

  while(obj_emote_head.next)
  {
    curr_index = obj_emote_head.next;
    obj_emote_head.next = curr_index->next;

    free_emoting_obj_data(curr_index);
    dc_free(curr_index);
  }

  free_emoting_obj_data(&obj_emote_head);  
}

void load_emoting_objects() 
{
    obj_emote_index *index_cursor = NULL;
    obj_emote_data *data_cursor = NULL;
    FILE *fl; 
    // short i;
    char fromfile;
    bool done = false,
         done2 = false;
    short offset;

    fl = dc_fopen(EMOTING_FILE, "r");
#ifdef LEAK_CHECK
    obj_emote_head.next = (struct obj_emote_index *)
                          calloc(1, sizeof(struct obj_emote_index));
#else
    obj_emote_head.next = (struct obj_emote_index *)
                          dc_alloc(1, sizeof(struct obj_emote_index));
#endif
    index_cursor = obj_emote_head.next;
    index_cursor->next = NULL;
    index_cursor->data = NULL;
    index_cursor->room_number = -1;
    index_cursor->emote_index_length = -1;
    index_cursor->frequency = 0;
#ifdef LEAK_CHECK
    data_cursor =(struct obj_emote_data *)
                 calloc(1, sizeof(struct obj_emote_index));
#else
    data_cursor =(struct obj_emote_data *)
                 dc_alloc(1, sizeof(struct obj_emote_index));
#endif
    index_cursor->data = data_cursor;
    data_cursor->next = NULL;
    while(!done2) {
        index_cursor->room_number = fread_int(fl, 0, 1000000);
        index_cursor->frequency = fread_int(fl, 0, 1000000);
        done = false;
        while(!done) {
            // Why are we dc_alloc'ing the space when fread_string is returning us
            // a pointer to the space IT allocs?  Azrack you silly goose.  I fixed it.
            // -pir 05/03/00
            //data_cursor->emote_text = (char *)dc_alloc(100, sizeof(char));
            data_cursor->emote_text = fread_string(fl, 0);
            index_cursor->emote_index_length++;
            if((offset = 1) && ((fromfile = fgetc(fl)) == 'S') && ((offset = 2) && (fromfile = fgetc(fl)) == '\n')) {
                done = true;
            } else {
#ifdef LEAK_CHECK
                data_cursor->next = (struct obj_emote_data *)
                                    calloc(1, sizeof(struct obj_emote_data));
#else
                data_cursor->next = (struct obj_emote_data *)
                                    dc_alloc(1, sizeof(struct obj_emote_data));
#endif
                data_cursor = data_cursor->next;
                data_cursor->next = NULL;
				// Azrack -- fseek had a -1 * offset * sizeof(char) which is going to send us to EOF immmediately
				// because fseek takes an unsigned int.
                fseek(fl, (-1 * offset * sizeof(char)), SEEK_CUR);
            }
        }
        if((fromfile = fgetc(fl)) == '$') {
            done2 = true;
        } else {
            fseek(fl, (1 * sizeof(char)), SEEK_CUR);
#ifdef LEAK_CHECK
            index_cursor->next = (struct obj_emote_index *)
                                 calloc(1, sizeof(struct obj_emote_index));
#else
            index_cursor->next = (struct obj_emote_index *)
                                 dc_alloc(1, sizeof(struct obj_emote_index));
#endif
            index_cursor = index_cursor->next;
            index_cursor->next = NULL;
#ifdef LEAK_CHECK
            index_cursor->data = (struct obj_emote_data *)
                                 calloc(1, sizeof(struct obj_emote_data));
#else
            index_cursor->data = (struct obj_emote_data *)
                                 dc_alloc(1, sizeof(struct obj_emote_data));
#endif
            index_cursor->room_number = -1;
            index_cursor->emote_index_length = -1;
            index_cursor->frequency = -1;
            data_cursor = index_cursor->data;
            data_cursor->next = NULL;
        }
    }
    dc_fclose(fl);
    return;
}

int emoting_object(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,
                   CHAR_DATA *invoker)
{
    obj_emote_index *index_cursor = NULL;
    obj_emote_data  *data_cursor = NULL;
    short i = 0;
    if(cmd) {
        return eFAILURE;
    }
    if(!obj) {
        return eFAILURE;
    }
    if(-1 == obj->in_room) {
         return eFAILURE;
    }
    if(!world[obj->in_room].people) {
        return eFAILURE;
    }
    ch = world[obj->in_room].people;
    for(index_cursor = &obj_emote_head; index_cursor; index_cursor = index_cursor->next) {
        if(real_room(index_cursor->room_number) == obj->in_room) {
            if(real_room(index_cursor->room_number) == NOWHERE) {
                return eFAILURE;
            }
            i = number(0, (index_cursor->emote_index_length));
            data_cursor = index_cursor->data; 
            for(i = 0; i < number(0, index_cursor->emote_index_length); i++) {
                data_cursor = data_cursor->next;
            }
            if(number(0, (index_cursor->frequency))  == 0) {
                act(data_cursor->emote_text, ch, 0, 0, TO_ROOM, 0);
                act(data_cursor->emote_text, ch, 0, 0, TO_CHAR, 0);
            }
        }
    }
    return eFAILURE;
}

int barbweap(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,
		CHAR_DATA *invoker)
{ // Cestus
  if (cmd != CMD_PUSH) return eFAILURE;
  if (str_cmp(arg," cestus")) return eFAILURE;
  switch (obj->obj_flags.value[3])
  {
    case 4:
    case 5:
	send_to_char("You twist your wrists quickly causing sharp spikes to spring forth from your weapon!\r\n",ch);
	obj->obj_flags.value[3] = 10;
	return eSUCCESS;
    case 10:
	send_to_char("You twist your wrists quickly, retracting the spikes from your weapon.\r\n",ch);
	obj->obj_flags.value[3] = 5;
	return eSUCCESS;
  }
 return eFAILURE;
}
int souldrainer(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker) 
{
    // struct obj_data *wielded;
    int percent, chance;
    CHAR_DATA *vict;

    if(!(vict = ch->fighting)) {
	return eFAILURE;
    }
    if(GET_HIT(vict) < 3500) {
        percent = (100 * GET_HIT(vict)) / GET_MAX_HIT(vict);
	chance = number(0, 101);
	if(chance > (1.3 * percent)) {
	    chance = number(0, 101);
	    if(chance > (2 * percent)) {
		chance = number(0, 101);
		if(chance > (2 * percent)) {
		    chance = number(0, 101);
		    if(chance > (2 * percent)) {
			act("You feel your soul being drained as $n's magics swirl around you",
			    ch, 0, vict, TO_VICT, 0);
                        act("$N gasps in agony as $n's dark magics grasp at $S soul.", 
			    ch, 0, vict, TO_ROOM, NOTVICT);
                        act("Evil energy surges into you as you wrench at $N's soul.",
			    ch, 0, vict, TO_CHAR, 0);
                        act("The Darkness has triumphed! You drain away $N's esseence.", 
			    ch, 0, vict, TO_CHAR, 0);
                        act("Everything goes black as your soul is pulled into the abyss.", 
			    ch, 0, vict, TO_VICT, 0);
                        act("$N screams as his soul is destroyed by $n's dark magics",
			    ch, 0, vict, TO_ROOM, NOTVICT);
                        GET_HIT(vict) = -20; 
			group_gain(ch, vict);
			fight_kill(ch, vict, TYPE_CHOOSE, 0);
			return eSUCCESS;

                   } else { // Missed the fucker
		       act("You braveley resist as $n pulls at the very essence of your soul.", 
			   ch, 0, vict, TO_VICT, 0);
                       act("$N stands his ground as dark magic coils around $n",
			   ch, 0, vict, TO_ROOM, NOTVICT);
                       act("$N shudders but resists your grasp upon $S soul.",
			   ch, 0, vict, TO_CHAR, 0);
                   }
               }
	   }
       }
   }
   return eFAILURE;
}


int pushwand(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,
		CHAR_DATA *invoker)
{
   if (cmd != CMD_SAY && cmd != CMD_PUSH)
     return eFAILURE;
   if (cmd == CMD_PUSH)
   {
     if (str_cmp(arg, " wand") && str_cmp(arg, " ivory") && str_cmp(arg, " ebony"))
       return eFAILURE;
     if (GET_STR(ch) < 20)
     {
	 send_to_char("You try to separate the wand pieces, but you find yourself too weak to do so.\r\n",ch);
         return eSUCCESS;
     }
     int newspell;
     switch (obj->obj_flags.value[3])
     {
	case 17: newspell = 28; break;
	case 28: newspell = 59; break;
	case 59: newspell = 113; break;
	case 113: newspell = 17; break;
	default: newspell = 17;break;
     }
     obj->obj_flags.value[3] = newspell;
     send_to_char("You push the ivory so that the ivory and ebony separate.\r\nReassembling them, you hear a \"click\" as they snap back into place.\r\n",ch);
     return eSUCCESS;
   } else if (cmd == CMD_SAY) {
     if (str_cmp(arg, " recharge")) return eFAILURE;
     struct obj_data *curr;
     for (curr = ch->carrying; curr; curr = curr->next_content)
     {
       if (obj_index[curr->item_number].virt == 22427) // red dragon snout
       {
	 obj->obj_flags.value[2] = 5;
	 send_to_char("The wand absorbs the dragon snout and pulses with energy.\r\n",ch);
	 obj_from_char(curr);
	 extract_obj(curr);
	 return eSUCCESS;
       }
     }
     return eFAILURE;
   } else return eFAILURE;
}


int dawnsword(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
	if (cmd != CMD_SAY)
		return eFAILURE;
	if (str_cmp(arg, " liberate me ab inferis"))
		return eFAILURE;
	if (GET_ALIGNMENT(ch) < 350)
	{
		send_to_char("Dawn refuses your impure prayer.\r\n",ch);
		return eSUCCESS;
	}
	if (isTimer(ch, OBJ_DAWNSWORD))
	{
		send_to_char("Dawn needs more time to recharge and is not ready to hear your prayer yet.",ch);
		return eSUCCESS;
	}
	if (!ch->in_room || IS_SET(world[ch->in_room].room_flags, SAFE) || IS_SET(world[ch->in_room].room_flags, NO_MAGIC))
	{
		send_to_char("Something about this room blocks your command.\r\n", ch);
		return eSUCCESS;
	}
	addTimer(ch, OBJ_DAWNSWORD, 24);

	send_to_char("You whisper a prayer to Dawn and it responds in a brilliant flash of light!\r\n",ch);
	CHAR_DATA *v;
	struct affected_type af;
	for (v = world[ch->in_room].people; v; v = v->next_in_room)
	{
		if (v==ch) continue;
		if (GET_ALIGNMENT(v) >= 350)
		{
			act("$n whispers a quiet prayer and a glorious flash of holy light explodes from their weapon!",ch, 0, v, TO_VICT, 0);
			continue;
		}
		if (IS_AFFECTED(v, AFF_BLIND)) continue; // no doubleblind
		act("$n whispers a quiet prayer and a searing blast of white light suddenly blinds you!",ch, 0, v, TO_VICT, 0);
		af.type      = SPELL_BLINDNESS;
	        af.location  = APPLY_HITROLL;
      		af.modifier  = has_skill(v,SKILL_BLINDFIGHTING)?skill_success(v,0,SKILL_BLINDFIGHTING)?-10:-20:-20;
	        af.duration  = 2;
      		af.bitvector = AFF_BLIND;
   	 	affect_to_char(v, &af);
    		af.location = APPLY_AC;
 		af.modifier  = has_skill(v,SKILL_BLINDFIGHTING)?skill_success(v,0,SKILL_BLINDFIGHTING)?20:40:40;
	        affect_to_char(v, &af);
	}

	return eSUCCESS;

}
int songstaff(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
	if (cmd) return eFAILURE;
	if (obj->equipped_by == NULL || !obj->equipped_by->in_room) return eFAILURE;
	ch = obj->equipped_by;
	char buf[MAX_STRING_LENGTH];
	if ((IS_SET(world[ch->in_room].room_flags, SAFE) || IS_SET(world[ch->in_room].room_flags, NO_MAGIC) || IS_SET(world[ch->in_room].room_flags, QUIET) )) return eFAILURE;

	obj->obj_flags.timer--;
	if (obj->obj_flags.timer > 0) return eFAILURE;
	obj->obj_flags.timer = 5;

	int heal;
        for (char_data * tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room) {
                if (!ARE_GROUPED(ch, tmp_char))
                        continue;

                heal = 50 / 2 + ((GET_MAX_MOVE(tmp_char) * 2) / 100) + (number(0, 20) - 10);
                if (heal < 5)
                        heal = 5;

                if (IS_PC(tmp_char) && IS_SET(tmp_char->pcdata->toggles, PLR_DAMAGE)) {
                        if (tmp_char == ch) {
                                csendf(tmp_char, "You feel your Travelling March recover %d moves for you.\r\n", heal);
                        } else {
                                sprintf(buf, "You feel %s's Travelling March recovering %d moves for you.\r\n", GET_NAME(ch), heal);
                                send_to_char(buf, tmp_char);
                        }
                } else
                        send_to_char("Your feet feel lighter.\r\n", tmp_char);
                GET_MOVE(tmp_char) += heal;
                if (GET_MOVE(tmp_char) > GET_MAX_MOVE(tmp_char))
                        GET_MOVE(tmp_char) = GET_MAX_MOVE(tmp_char);
        }


	return eSUCCESS;
}

void check_eq(CHAR_DATA *ch);

int lilithring(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
	if (cmd != CMD_SAY)
		return eFAILURE;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	arg = one_argument(arg, arg1);
	arg = one_argument(arg, arg2);
	if (str_cmp(arg1, "ateni"))
		return eFAILURE;
	CHAR_DATA *victim;
	if (!(victim = get_char_room_vis(ch, arg2))) {
		send_to_char("Noone here by that name.\r\n", ch);
		return eSUCCESS;
	}
	if (!IS_NPC(victim))
	{
		send_to_char("The Gods prohibit such evil.\r\n", ch);
		return eSUCCESS;
	}
	if (isTimer(ch, OBJ_LILITHRING))
	{
		send_to_char("The ring remains dark and your command goes unheeded.\r\n",ch);
		return eSUCCESS;
	}

       if (circle_follow(victim, ch)) {
                send_to_char("Sorry, following in circles can not be allowed.\n\r", ch);
		return eSUCCESS;
        }

        if ((!ISSET(victim->mobdata->actflags, ACT_BARDCHARM) && !ISSET(victim->mobdata->actflags, ACT_CHARM)) || GET_LEVEL(victim)>50) {
		act("$N's soul is too powerful for you to command.", ch, 0, victim, TO_CHAR, 0);
		return eSUCCESS;
        }
	if (GET_ALIGNMENT(ch) > -350)
	{
		send_to_char("Your soul is too pure for such an unclean act.\r\n",ch);
		return eSUCCESS;
	}

	if (!ch->in_room || IS_SET(world[ch->in_room].room_flags, SAFE))
	{
		send_to_char("Something about this room blocks your command.\r\n", ch);
		return eSUCCESS;
	}
	act("You speak the command and $N must comply. Lilith's Ring of Command glows with mirthful malevolence.", ch, 0, victim, TO_CHAR, 0);
	act("$n whispers softly to $N. $N's eyes go blank and they now follow $n.", ch, 0, victim, TO_ROOM, 0);

      addTimer(ch, OBJ_LILITHRING, 24);
      if (victim->master)
                stop_follower(victim, 0);

        remove_memory(victim, 'h');

        add_follower(victim, ch, 0);
	struct affected_type af;

        af.type = OBJ_LILITHRING;
        af.duration = 3;
        af.modifier = 0;
        af.location = 0;
        af.bitvector = AFF_CHARM;
        affect_to_char(victim, &af);

        /* remove any !charmie eq the charmie is wearing */
        check_eq(victim);

	return eSUCCESS;
}


int orrowand(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
	if (cmd != CMD_SAY || str_cmp(arg, " recharge"))
		return eFAILURE;

	struct obj_data *curr;
	struct obj_data *firstP = NULL, *secondP = NULL, *vial = NULL, *diamond = NULL;

	for (curr = ch->carrying; curr; curr=curr->next_content)
	{
		if (obj_index[curr->item_number].virt == 17399) diamond = curr;
		else if (obj_index[curr->item_number].virt == 27903 && firstP != NULL) secondP = curr;
		else if (obj_index[curr->item_number].virt == 27903 )  firstP = curr;
		else if (obj_index[curr->item_number].virt == 27904) vial = curr;
	}

	if (!firstP || !secondP || !vial || !diamond)
	{
		send_to_char("Recharge unsuccessful:  Missing required components.\r\n",ch);
		return eSUCCESS;
	}
	obj_from_char(firstP); extract_obj(firstP);
	obj_from_char(secondP); extract_obj(secondP);
	obj_from_char(vial); extract_obj(vial);
	obj_from_char(diamond); extract_obj(diamond);
	obj->obj_flags.value[2] = 5;
	send_to_char("The wand emits a soft \"beep\" and the display flashes \"Wand Recharged\"\r\n",ch);
	return eSUCCESS;
}



int holyavenger(CHAR_DATA *ch, struct obj_data *obj,  int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
  // struct obj_data *wielded;
   int percent, chance;
   CHAR_DATA *vict; 

   if(!(vict = ch->fighting)) {
     return eFAILURE;
   }
   if(GET_HIT(vict) < 3500) {
       percent = (100 * GET_HIT(vict)) / GET_MAX_HIT(vict);
       chance = number(1, 100);
       if(chance > (1.3 * percent)) {
           percent = (100 * GET_HIT(vict)) / GET_MAX_HIT(vict);
           chance = number(1, 100);
           if(chance > (2 * percent)) {
               chance = number(1, 100);
               if(chance > (2 * percent)) {
                   chance = number(1, 100);
                   if(chance > (2 * percent) && !IS_SET(vict->immune, ISR_SLASH)) {
		     if ((
                         (vict->equipment[WEAR_NECK_1] && obj_index[vict->equipment[WEAR_NECK_1]->item_number].virt == 518) ||
			 (vict->equipment[WEAR_NECK_2] && obj_index[vict->equipment[WEAR_NECK_2]->item_number].virt == 518)) 
			 && !number(0,1))
		       { // tarrasque's leash..
			 act("You attempt to behead $N, but your sword bounces of $S neckwear.",ch, 0, vict, TO_CHAR, 0);
			 act("$n attempts to behead $N, but fails.", ch, 0, vict, TO_ROOM, NOTVICT);
			 act("$n attempts to behead you, but cannot cut through your neckwear.",ch,0,vict,TO_VICT,0);
			 return eSUCCESS;
		       }
		     if(IS_AFFECTED(vict, AFF_NO_BEHEAD)) {
		       act("$N deftly dodges your beheading attempt!", ch, 0, vict, TO_CHAR, 0);
		       act("$N deftly dodges $n's attempt to behead $M!", ch, 0, vict, TO_ROOM, NOTVICT);
		       act("You deftly avoid $n's attempt to lop your head off!", ch, 0, vict, TO_VICT, 0);
		       return eSUCCESS;
		     }
		     act("You feel your life end as $n's sword SLICES YOUR HEAD OFF!", ch, 0, vict, TO_VICT, 0);
		     act("You SLICE $N's head CLEAN OFF $S body!", ch, 0, vict, TO_CHAR, 0);
		     act("$n cleanly slices $N's head off $S body!", ch, 0, vict, TO_ROOM, NOTVICT);
		     GET_HIT(vict) = -20;
		     make_head(vict);
		     group_gain(ch, vict); 
		     fight_kill(ch, vict, TYPE_CHOOSE, 0);
		     return eSUCCESS|eVICT_DIED; /* Zero means kill it! */
		     // it died..
                   } else { /* You MISS the fucker! */
		     act("You hear the SWOOSH sound of wind as $n's sword attempts to slice off your head!", ch, 0, vict, TO_VICT, 0);
		     act("You miss your attempt to behead $N.", ch, 0, vict, TO_CHAR, 0);
		     act("$N jumps back as $n makes an attempt to BEHEAD $M!", ch, 0, vict, TO_ROOM, NOTVICT);
                   }
               }
           }
       }
   }
   return eFAILURE;  
} 

int hooktippedsteelhalberd(CHAR_DATA *ch, struct obj_data *obj, int cmd,
                            char* arg, CHAR_DATA *invoker)
{
   CHAR_DATA *victim;
   if (!(victim = ch->fighting))
     return eFAILURE;
   if (number(1,101) > 2) return eFAILURE;
   int which = number(0, MAX_WEAR-1);
   if (!victim->equipment[which])
     return eFAILURE; // Lucky
   int i = damage_eq_once(victim->equipment[which]);
   if(!victim->equipment[which]) return eSUCCESS;
   if (i >= eq_max_damage(victim->equipment[which]))
     eq_destroyed(victim, victim->equipment[which], which);
   else {
     if(obj_index[obj->item_number].virt == 17904) {
      act("$n's diamond war club cracks your $p!", ch, victim->equipment[which], victim, TO_VICT, 0 );
      act("$n smashes $m diamond war club into $N's $p and cracks it!",ch,victim->equipment[which],victim, TO_ROOM, NOTVICT);
      act("You smash your club into $N's $p, and manage to crack it!",ch,victim->equipment[which],victim,TO_CHAR,0);
     } else {
      act("$n's hook-tipped steel halberd tears your $p!", ch, victim->equipment[which], victim, TO_VICT, 0 );
      act("$n latches $m hook-tipped steel halberd into $N's $p and tears it!",ch,victim->equipment[which],victim, TO_ROOM, NOTVICT);
      act("You latch your halberd into $N's $p, and manage to tear it!",ch,victim->equipment[which],victim,TO_CHAR,0);
     }
    }
   return eSUCCESS;
}

// TODO - I think we actually used this for a while but it was too powerful
int drainingstaff(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker) 
{
    CHAR_DATA *vict;
    obj_data *staff;
    int dam;

    if(!(vict = ch->fighting)) {
        return eFAILURE;
    }
    
    staff = ch->equipment[WIELD + cmd];
    dam = dice(staff->obj_flags.value[1], staff->obj_flags.value[2]);  
    dam += GET_DAMROLL(ch);
    dam = (dam * 2) / 10; // Mages usually have 2-3 attacks
    if(IS_NPC(ch)) { // NPC'S have no mana, so we'll drain hp instead
        if(dam >= GET_HIT(vict)) {
            dam = GET_HIT(vict) - 1;
        }
      if(affected_by_spell(vict, SPELL_DIVINE_FURY) && dam > affected_by_spell(vict, SPELL_DIVINE_FURY)->modifier)
         dam = affected_by_spell(vict, SPELL_DIVINE_FURY)->modifier;
        GET_HIT(vict) -= dam;
        GET_MANA(ch) += dam;
    } else { // We have a character... drain mana 
        if(dam >= GET_MANA(vict)) {
            dam = GET_MANA(vict);
        }
        GET_MANA(vict) -= dam;
        GET_MANA(ch) += dam;
    }
    if(dam == 0) {
        return eFAILURE;
    }
    if(GET_MANA(ch) > GET_MAX_MANA(ch)) {
        GET_MANA(ch) = GET_MAX_MANA(ch); // can't go above the MAX_MANA
    }
    return eSUCCESS;
}



int bank(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
  char buf[MAX_INPUT_LENGTH];
  int32 x;

  if(cmd != CMD_BALANCE && cmd != CMD_DEPOSIT && cmd != CMD_WITHDRAW)
    return eFAILURE;

  /* balance */
  if(cmd == CMD_BALANCE) {
    sprintf(buf, "You have %d coins in the bank.\n\r", GET_BANK(ch));
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  /* deposit */
  if(cmd == CMD_DEPOSIT) {
    if(!IS_MOB(ch) && affected_by_spell(ch, FUCK_GTHIEF)) 
    {
      send_to_char("Your criminal acts prohibit it.\n\r", ch);
      return eFAILURE;
    }

    one_argument(arg, buf);
    if(!*buf || !(x = atoi(buf)) || x < 0) {
      send_to_char("Deposit what?\n\r", ch);
      return eSUCCESS;
    }
    if((uint32)x > GET_GOLD(ch)) {
      send_to_char("You don't have that much gold!\n\r", ch);
      return eSUCCESS;
    }
    if ((uint32)x + GET_BANK(ch) > 2000000000)
    {
	send_to_char("That would bring you over your account maximum!\r\n",ch);
	return eSUCCESS;
    }
    GET_GOLD(ch) -= x;
    GET_BANK(ch) += x;
    sprintf(buf, "You deposit %d coins.\n\r", x);
    send_to_char(buf, ch);
    save_char_obj(ch);
    return eSUCCESS;
  }

  /* withdraw */
  one_argument(arg, buf);
  if(!*buf || !(x = atoi(buf)) || x < 0) {
    send_to_char("Withdraw what?\n\r", ch);
    return eSUCCESS;
  }
  if((uint32)x > GET_BANK(ch)) {
    send_to_char("You don't have that much gold in the bank!\n\r", ch);
    return eSUCCESS;
  }
  GET_GOLD(ch) += x;
  GET_BANK(ch) -= x;
  sprintf(buf, "You withdraw %d coins.\n\r", x);
  send_to_char(buf, ch);
  save_char_obj(ch);
  return eSUCCESS;
}

int casino_atm(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
  char buf[MAX_INPUT_LENGTH];
  int32 x;

  if(cmd != CMD_BALANCE && cmd != CMD_DEPOSIT && cmd != CMD_WITHDRAW)
    return eFAILURE;

  /* balance */
  if(cmd == CMD_BALANCE) {
    sprintf(buf, "You have %d coins in the bank.\n\r", GET_BANK(ch));
    send_to_char(buf, ch);
    return eSUCCESS;
  }

  /* deposit */
  if(cmd == CMD_DEPOSIT) {
    send_to_char("You cannot use this for depositing money.\r\n",ch);
    return eSUCCESS;
  }

  /* withdraw */
  one_argument(arg, buf);
  if(!*buf || !(x = atoi(buf)) || x < 0) {
    send_to_char("Withdraw what?\n\r", ch);
    return eSUCCESS;
  }
  if((uint32)x > GET_BANK(ch)) {
    send_to_char("You don't have that much gold in the bank!\n\r", ch);
    return eSUCCESS;
  }
  GET_GOLD(ch) += x;
  GET_BANK(ch) -= x;
  sprintf(buf, "You withdraw %d coins.\n\r", x);
  send_to_char(buf, ch);
  save_char_obj(ch);
  return eSUCCESS;
}

// if you are in the room with this obj, and you try to go in any
// direction, and you don't have the correct obj in your inventory, it sends
// you to a particular room 
// TODO - get it so that it effects groups properly.  Right now, if a group
// leader moves, he goes to the "bad" room, and the rest of the group goes to
// where the exit is supposed to go
// I should probably just change the exit temporarily to point to the "bad" room
// and then change it back after the move.
int returner(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker) 
{

   if(cmd > CMD_DOWN || cmd < CMD_NORTH)
      return eFAILURE;

   if(!obj)
      return eFAILURE;

   if(!obj->in_room)
      return eFAILURE;

   if(!CAN_GO(ch, cmd-1))
      return eFAILURE;

   if(ch->in_room != obj->in_room)
      return eFAILURE;
   
   if(get_obj_in_list_num(real_object(2200), ch->carrying))
      return eFAILURE;

   if(move_char(ch, real_room(START_ROOM)) == 0)
      return eFAILURE;

   do_look(ch, "\0", 15);

   return eSUCCESS;
}

#define MAX_GEM_ASSEMBLER_ITEM   10

struct assemble_item {
   char * finish_to_char;
   char * finish_to_room;
   char * missing_to_char;
   int components[MAX_GEM_ASSEMBLER_ITEM];
   int item;
};

struct assemble_item assemble_items[] = {
   // Item 0, the crystalline tir stuff
   { "A brilliant flash of light erupts from your hands as the gems mold themselves together and form a cohesive and flawless gem.\r\n",
     "A brilliant flash of light erupts from $n's hands as the gems $e holds form a new cohesive and flawless gem.",
     "One of the gems seems to be missing.\r\n",
     { 2714, 2602, 12607, -1, -1, -1, -1, -1, -1, -1 },
	 1506
   },

   // Item 1, Etala the Shadowblade
   { "Connecting the hilt and gem to the blade, you form a whole sword.\r\n",
     "$n fiddles around with some stuff in $s inventory.",
     "You seem to be missing a piece.\r\n",
     { 181, 182, 183, -1, -1, -1, -1, -1, -1, -1 },
	 184
   },

   // Item 2, Broadhead arrow from forage items
   { "You carefully sand down the pointy stick, adding to its excellent form.\r\n"
     "Splitting the feathers down, you carefully attach them to the pointy stick.\r\n"
     "Finally, you shape the scorpion stinger into a deadly arrowhead and secure it to the front.\r\n",
     "$n sits down with some junk and tries $s hand at fletching.",
     "You don't have all the items required to fletch an arrow.\r\n",
     { 3185, 3186, 3187, -1, -1, -1, -1, -1, -1, -1 },
	 3188
   },

   // Item 3, Wolf tooh arrow from forage items
   { "You carefully sand down the pointy stick, adding to its excellent form.\r\n"
     "Splitting the feathers down, you carefully attach them to the pointy stick.\r\n"
     "Finally, you hone the wolf's tooth stinger into a sharp arrowhead and secure it to the front.\r\n",
     "$n sits down with some junk and tries $s hand at fletching.",
     "You don't have all the items required to fletch an arrow.\r\n",
     { 3185, 28301, 3187, -1, -1, -1, -1, -1, -1, -1 },
	 3190
   },

   // Item 4, Gaiot key in DK
   { "The stone pieces join together to form a small statue of a dragon.\r\n",
     "$n assembles some stones together to form a black statue.\r\n",
     "The pieces click together but fall apart as if something is missing.\r\n",
     { 9502, 9503, 9504, 9505, 9506, -1, -1, -1, -1, -1 },
	 9501
   },

   // Item 5, ventriloquate dummy - rahz
   { "You are able to put the parts together, and create a ventriloquist's dummy.\r\n",
     "$n manages to the parts together, creating a ventriloquist's dummy.\r\n",
     "The pieces don't seem to fit together quite right.\r\n",
     { 17349, 17350, 17351, 17352, 17353, 17354, -1, -1, -1, -1 },
	 17348
   },

   // Item 6, the Shield of the Beholder
   { "You place the two gems into the holes on the shield and it seems to hum with power.\n\r",
     "$n places two precious gemstones into a beholder's carapace to create a shield.\n\r",
     "You seem to be missing a piece.\n\r",
     { 5260, 5261, 5262, -1, -1, -1, -1, -1, -1, -1 },
	 5263
   },

   // Item 7, a curiously notched medallion
   { "With a blinding flash, the gem makes the medallion whole.\n\r",
     "As $n fiddles with the medallion pieces, you are dazed by a bright flash!\n\r",
	 "You attempt to assemble the family medallion but something is missing.\n\r",
     { 30084, 30085, 30086, 30087, 30088, -1, -1, -1, -1, -1 },
	 30083
   },

   // Junk Item.  THIS MUST BE LAST IN THE ARRAY
   { "",
     "",
     "",
     { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	 -1
   }

};

int hellmouth_thing(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
  if (cmd != CMD_SAY) return eFAILURE;
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  
   half_chop(arg, arg1, junk);
   if(*junk)
      return eFAILURE;
  
   if (strncmp(arg1, "vanity", 6))
    return eFAILURE;

  if (invoker->fighting)
    return eFAILURE;
  
  // send_to_char("The hellmouth reaches out and embraces you. As you become completely\r\n"
  // "immersed in its energy your skin burns, your sight fades into darkness,\r\n" 
  // "your nose recoils at the stench of sulphur, and all you can taste is\r\n"
  // "blood.\r\n",invoker);
//  char_from_room(invoker);
//  char_to_room(invoker, real_room(4801));
  GET_KI(invoker) -= 50;
  if (GET_KI(invoker) < 0)
    GET_KI(invoker) = 0;
  GET_HIT(invoker) /= 2;
  GET_MANA(invoker) /= 2;
  GET_MOVE(invoker) /= 2;  
  //  do_look(invoker, "", 9);
  //   send_to_char("In an instant your senses are restored and you are left only temporarily\r\n"
  //                "dazed. Although you appear to be somewhere other than where you were prior\r\n"
  //                "to this experience, your life feels as though it has ebbed to the brink of\r\n"
  // 	        "death and has been only partially restored.\r\n",invoker);

  return eFAILURE; // So normal say function will execute after this
}

int search_assemble_items(int vnum)
{
	// This should never happen
	if (vnum < 1) {
		logf(ANGEL, LOG_BUG, "search_assemble_items passed vnumx=%d\n\r", vnum);
		produce_coredump();
		return -1;
	}

	for (int item_index=0; assemble_items[item_index].item != -1; item_index++) {
		// We search until MAX_GEM_ASSEMBLER_ITEM -1 because we don't want to include the last item
		// which is the finished item vnum
		for (int component_index=0; component_index < MAX_GEM_ASSEMBLER_ITEM; component_index++) {
			if (assemble_items[item_index].components[component_index] == vnum) {
				return item_index;
			}
		}
	}

	return -1;
}


bool assemble_item_index(char_data *ch, int item_index)
{
	// This should never happen
	if (item_index < 0) {
		logf(ANGEL, LOG_BUG, "assemble_item_index passed item_index=%d\n\r", item_index);
		produce_coredump();
		return false;
	}

	// Look through all the components of item_index and see if the player has them
	for (int component_index=0; component_index < MAX_GEM_ASSEMBLER_ITEM; component_index++) {

		int component_virt = assemble_items[item_index].components[component_index];
		if (component_virt < 1) {
			continue;
		}

		int component_real = real_object(component_virt);
		if (component_real < 0) {
			logf (ANGEL, LOG_BUG, "assemble_items[%d], component_index %d refers to invalid rnum %d for vnum %d.",
					item_index, component_index, component_real, component_virt);

			send_to_char("There was an internal malfunction assembling your item. Contact an Immortal.\n\r", ch);
			produce_coredump();
			return true;
		}

	    if (get_obj_in_list_num(component_real, ch->carrying) == 0) {
	    	return false;
	    }
	}

	// If we get to this point then all components for item_index were found
	int item_vnum = assemble_items[item_index].item;
	int item_real = real_object(item_vnum);
	obj_data *item = (obj_data *)obj_index[item_real].item;

	// Check if the item to be assembled is marked UNIQUE but the player already has one
	if (IS_SET(item->obj_flags.more_flags, ITEM_UNIQUE)) {
		if (search_char_for_item(ch, item_real, false)) {
			send_to_char("You already have one of those!\r\n", ch);
			return true;
		}
	}

	// Send item specific assemble messages to the player and room
	send_to_char(assemble_items[item_index].finish_to_char, ch);
	act(assemble_items[item_index].finish_to_room, ch, 0, 0, TO_ROOM, 0);

	// Remove all the components from the player
	for (int component_index=0; component_index < MAX_GEM_ASSEMBLER_ITEM; component_index++) {
		int component_virt = assemble_items[item_index].components[component_index];
		if (component_virt < 1) {
			continue;
		}

		int component_real = real_object(component_virt);
		if (component_real < 0) {
			logf (ANGEL, LOG_BUG, "assemble_items index %d, component_index %d refers to invalid rnum %d for vnum %d.",
					item_index, component_index, component_real, component_virt);

			send_to_char("There was an internal malfunction assembling your item. Contact an Immortal.\n\r", ch);
			return true;
		}

		obj_data *component_obj = get_obj_in_list_num(component_real, ch->carrying);
		obj_from_char(component_obj);
		extract_obj(component_obj);
	}

	// make the new item
	obj_data *reward_item = clone_object(item_real);
	if (reward_item == 0) {
		logf (ANGEL, LOG_BUG, "Unable to clone vnum %d, rnum %d.", item_vnum, item_real);
		send_to_char("There was an internal malfunction cloning the new item. Contact an Immortal.\n\r", ch);
		return true;
	}

	obj_to_char(reward_item, ch);

	return true;
}

int do_assemble(struct char_data *ch, char *argument, int cmd)
{
	bool different_item_components = false;
	int vnum, item_index = -1, last_item_index = -1;
    char arg1[MAX_INPUT_LENGTH+1];
    obj_data *obj;

    one_argument(argument, arg1);

    // if no arguments are given then look through entire inventory for items to assemble.
    if (arg1[0] == 0) {
    	// for each inventory item
    	for (obj = ch->carrying; obj; obj = obj->next_content) {
    		// if we can see it
			if (CAN_SEE_OBJ(ch, obj, false)) {
				vnum = obj_index[obj->item_number].virt;
			    item_index = search_assemble_items(vnum);

				// check if it's a component of an item to be assembled
			    if (item_index != -1) {
			    	// check if the current component is from a different item than the previous
			    	// component that we found
			    	if (last_item_index == -1) {
			    		last_item_index = item_index;
			    	} else if (last_item_index != item_index) {
			    		different_item_components = true;
			    	}

			    	if (different_item_components) {
			    		break;
			    	}
			    }
			}
    	}

    	// If components from multiple items were found then make the player specify
    	// the item that needs to be assembled
    	if (different_item_components) {
    		csendf(ch, "Assemble which object?\n\r");
    		return eFAILURE;
    	} else if (last_item_index == -1) {
    		csendf(ch, "You don't have anything that can be assembled.\n\r");
    		return eFAILURE;
    	} else {
    		// Attempt to assemble all the components
    		if (assemble_item_index(ch, last_item_index) == false) {
    			csendf(ch, "%s", assemble_items[last_item_index].missing_to_char);
    			return eFAILURE;
    		}
    	}

		return eSUCCESS;
    }

    // if arguments are given, find object and see if it can be assembled.
    obj = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if(obj == NULL)
    {
        act("You can't find it!", ch, 0, 0, TO_CHAR, 0);
        return eFAILURE;
    }

    vnum = obj_index[obj->item_number].virt;
    item_index = search_assemble_items(vnum);

    // Object specified is not part of an item that can be assembled
	if (item_index == -1) {
		csendf(ch, "That item can't be assembled into anything.\n\r");
		return eFAILURE;
	}

	// Attempt to assemble all the components
	if (assemble_item_index(ch, item_index) == false) {
		csendf(ch, "%s", assemble_items[item_index].missing_to_char);
		return eFAILURE;
	}

    return eSUCCESS;
}

int stupid_button(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,
		   CHAR_DATA *invoker)
{
  if(cmd != CMD_PUSH) return eFAILURE;

  char vict[MAX_STRING_LENGTH];

  one_argument(arg, vict);
  if(strcmp(vict, "button") && strcmp(vict, "red") && strcmp(vict, "big"))
  {
    send_to_char("Push what?\n\r", ch);
    return eFAILURE;
  }

  send_to_char("You couldn't help but push the $4$Bbutton$R, could you?\n\r", ch);
  send_to_char("The floor drops out beneath you and you find yourself.. er.. somewhere.\n\r", ch);
  move_char(ch, real_room(49));
  do_look(ch, "\0", 15);
  return eSUCCESS;
}

// Fear gaze.
int gazeofgaiot(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,
		   CHAR_DATA *invoker)
{
   CHAR_DATA *victim;
   char vict[MAX_INPUT_LENGTH]; // buffer overflow fix

   one_argument(arg, vict);
   if (cmd != CMD_GAZE) return eFAILURE;
   if (!ch->equipment[WEAR_FACE] || real_object(9603) != ch->equipment[WEAR_FACE]->item_number)
     return eFAILURE;

   if (affected_by_spell(ch,SKILL_FEARGAZE)) {
      send_to_char("You need to build up more hatred before you can unleash it again.\r\n",ch);
      return eSUCCESS;
   }

   if (!(victim = get_char_room_vis(ch, vict))) {
      if (ch->fighting) {
         victim = ch->fighting;
      } else {
         send_to_char("Gaze on whom?\n\r", ch);
         return eSUCCESS;
      }
    }
    if(!can_attack(ch) || !can_be_attacked(ch, victim))
          return eSUCCESS;
    if (IS_SET(world[ch->in_room].room_flags, NO_MAGIC))
    {
	send_to_char("That action is impossible to perform in these restrictive confinements.\r\n",ch);
	return eSUCCESS;
    }
    if (GET_LEVEL(victim) > 70) 
    {
        send_to_char("Some great force prevents you.\r\n",ch);
	return eSUCCESS;
    }
    // All is good, set timer and perform it.
    struct affected_type af;
    af.type = SKILL_FEARGAZE;
    af.duration  = 30;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = -1;
    affect_to_char(ch, &af);
    act("You eyes glow red from hatred, and you discharge it all on $N.",ch,0,victim,TO_CHAR,0);
    act("$n's eyes glow with hatred, and $e directs it all at you. OoO, scary!",ch,0,victim,TO_VICT,0);
    while (number(0,1))
       do_flee(victim, "", 0);
   return eSUCCESS;
}

int pfe_word(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   // char buf[MAX_INPUT_LENGTH];
   char junk[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   struct obj_data *obj_object;
   int j;

   struct obj_data *get_object_in_equip_vis(struct char_data *ch,
         char *arg, struct obj_data *equipment[], int *j, bool blindfighting);

   if(!cmd && obj) // This is where we recharge
      if(obj->obj_flags.value[3])
         obj->obj_flags.value[3]--;

   // 11 = say 69 = remove
   if(cmd != CMD_SAY && cmd != CMD_REMOVE)
      return eFAILURE;

   if(!ch)
      return eFAILURE;
   int pos = HOLD;
   if(!ch->equipment[pos] || real_object(3611) != ch->equipment[pos]->item_number)
   {
	pos = HOLD2;
        if(!ch->equipment[pos] || real_object(3611) != ch->equipment[pos]->item_number)
	 return eFAILURE;
   }

   half_chop(arg, arg1, junk);
   if(*junk)
      return eFAILURE;

   if(cmd == CMD_SAY)
   {
      if(str_cmp("aslexi",arg1))
         return eFAILURE;

      if(ch->equipment[pos]->obj_flags.value[3])
      {
         send_to_char("The item seems to be recharging.\r\n", ch);
         return eSUCCESS;
      }

      ch->equipment[pos]->obj_flags.value[3] = 600;

      act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
      send_to_char("You quietly whisper 'aslexi' into your hands.\r\n", ch);

     // cast_protection_from_evil(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0, 50);
     // changed to spell_type_potion so that the align check doesn't happen for this item
//      cast_protection_from_evil(GET_LEVEL(ch), ch, 0, SPELL_TYPE_POTION, ch, 0, 
//50);
  if (IS_AFFECTED(ch,AFF_PROTECT_EVIL) || affected_by_spell(ch, SPELL_PROTECT_FROM_GOOD))
   {
	send_to_char("You already have alignment protection.\r\n",ch);
         return eFAILURE;
   }

  struct affected_type af;
  if (!affected_by_spell(ch, SPELL_PROTECT_FROM_EVIL) )
  {
         af.type      = SPELL_PROTECT_FROM_EVIL;
         af.duration  = 5;
         af.modifier  = GET_LEVEL(ch)+10;
         af.location  = APPLY_NONE;
         af.bitvector = AFF_PROTECT_EVIL;
         affect_to_char(ch, &af);
         send_to_char("You have a righteous, protected feeling!\n\r", ch);
  }

      act("A pulsing aura springs to life around $n!", ch, 0, 0, TO_ROOM, 0);
      return eSUCCESS;
   }
  else // cmd=69 (remove)
   {
      obj_object = get_object_in_equip_vis(ch, arg1, ch->equipment, &j, FALSE);
      if(!obj_object)
         return eFAILURE;

      if(obj_object != ch->equipment[HOLD] &&
	  obj_object != ch->equipment[HOLD2])
         return eFAILURE;
      if (obj_object->item_number != real_object(3611))
	return eFAILURE;

      if(affected_by_spell(ch, SPELL_PROTECT_FROM_EVIL))
      {
         affect_from_char(ch, SPELL_PROTECT_FROM_EVIL);      
         send_to_char("The power drains away.\r\n", ch);
      }
      // This should remove the pfe unless he has it cast or on EQ
      // We will allow it to return FALSE do the do_remove goes through.
   }
   return eFAILURE;
}

int devilsword(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   // char buf[MAX_INPUT_LENGTH];
   char junk[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   // struct obj_data *obj_object;
   // int j;


   if(cmd != CMD_SAY)
      return eFAILURE;

   if(!ch || !ch->equipment || !ch->equipment[WIELD])
      return eFAILURE;

   if(real_object(185) != ch->equipment[WIELD]->item_number)
      return eFAILURE;

   half_chop(arg, arg1, junk);

   if(*junk)
      return eFAILURE;

   if(!str_cmp("infrendeo",arg1))
   {
      act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
      if(ch->equipment[WIELD]->obj_flags.value[3] == 8)
      {
         send_to_char("Nothing happens.\r\n", ch);
         return TRUE;
      }

      act("Venom-flecked fangs grow and bristle from the bedeviled Cestus!", ch, 0, 0, TO_ROOM, 0);
      send_to_char("Huge fangs spring forth from your weapon!\r\n", ch);

      ch->equipment[WIELD]->obj_flags.value[3] = 8;
      return eSUCCESS;
   }
   if(!str_cmp("pulsus",arg1))
   {
      act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
      if(ch->equipment[WIELD]->obj_flags.value[3] == 7)
      {
         send_to_char("Nothing happens.\r\n", ch);
         return eSUCCESS;
      }

      act("The fangs of $n's weapon retract magically into the metal.", ch, 0, 0, TO_ROOM, 0);
      send_to_char("The fangs retract magically into the metal.\r\n", ch);

      ch->equipment[WIELD]->obj_flags.value[3] = 7;
      return eSUCCESS;
   }
   
   return eFAILURE;
}

// If you have AFFsanct but not the spell, kill it
// This works fine as long as they don't have perma-sanct eq
void remove_eliara(CHAR_DATA *ch)
{

   if(!IS_AFFECTED(ch, AFF_SANCTUARY))
      return;

   if(affected_by_spell(ch, SPELL_SANCTUARY))
      return;

   act("Eliara's glow fades, as she falls dormant once again.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("Eliara's glow fades, as she falls dormant once again.\r\n\r\n", ch);
   REMBIT(ch->affected_by, AFF_SANCTUARY);

}

int dancevest(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
	if (!cmd || cmd != CMD_SAY || !ch || !ch->in_room || str_cmp(arg, " just dance"))
	{
		return eFAILURE;
	}
	do_say(ch, " just dance", CMD_SAY);
	if (obj->obj_flags.timer > 0)
	{
		send_to_char("The vest remains silent.\r\n",ch);
		return eSUCCESS;
	}
	char *command_list [] =
	{
		"dance", // 0
		"shuffle",
		"wiggle",
		"bellydance",
		"bounce",
		"polka", //5
		"waltz",
		"boogie",
		"headbang",
		"showtune" // 9
	};
	CHAR_DATA *v;
	send_to_char("As you intone the sacred words, phantom music swells around you and everyone within earshot joins in!\r\n",ch);
	for (v = world[ch->in_room].people; v; v = v->next_in_room)
	{
		if (GET_POS(v) != POSITION_STANDING)
		{
			continue;
		}
		send_to_char("As phantom music swells around you, you are helpless to resist.  You must obey.\r\n", v);
		char tmp_command[32];
		strcpy(tmp_command, command_list[number(0,9)]);
		command_interpreter(v, tmp_command);


	}
	obj->obj_flags.timer = 48;
	return eSUCCESS;

}

int durendal(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{

	if (!cmd)
	{ // pulse
	  if (obj->obj_flags.timer > 0)
	  {
		obj->obj_flags.timer--;
	  }
	  if (obj->obj_flags.timer < 322 && IS_SET(obj->obj_flags.more_flags, ITEM_TOGGLE))
	  {
		REMOVE_BIT(obj->obj_flags.more_flags, ITEM_TOGGLE);
		if (obj->obj_flags.timer > 0 && obj->equipped_by && obj->equipped_by->in_room)
		{
		  send_to_char("The white fire surrounding Durendal gutters and flickers out.\r\n", obj->equipped_by);
		  act("The flames surrounding $n's weapon gutters and fade.", obj->equipped_by, 0, 0, TO_ROOM, 0);
		}


	  }
	  return eSUCCESS;
	}


	if (cmd != CMD_SAY || !ch || !ch->in_room || str_cmp(arg, " Gods forgive me") || obj->equipped_by != ch)
	{
		return eFAILURE;
	}
	if (obj->obj_flags.timer > 0)
	{
		send_to_char("Your plea goes unanswered. Durendal slumbers.\r\n",ch);
		return eSUCCESS;
	}
	if (GET_ALIGNMENT(ch) < 350)
	{
		send_to_char("Your soul is impure. Durendel ignores your contrition.\r\n", ch);
		return eSUCCESS;
	}
	if (IS_SET(world[ch->in_room].room_flags, SAFE))
	{
		send_to_char("Something about this room prohibits your prayer from being heard.\r\n",ch);
		return eSUCCESS;
	}
	send_to_char("Upon hearing your plea, Durendal suddenly bursts into flame with a blinding flash of searing white heat!\r\n",ch);
	act("$n mutters a quiet prayer and with a blinding flash, their weapon bursts into flame!", ch, 0, 0, TO_ROOM, 0);
	CHAR_DATA *v, *vn;
	for (v = world[ch->in_room].people; v; v = vn)
	{
		vn = v->next_in_room;
		if (GET_ALIGNMENT(v) > -350 || ARE_GROUPED(ch, v))
		{
			continue;
		}
		send_to_char("You feel the evil in your soul being burned away!\r\n", v);
		damage(ch, v, 250, TYPE_COLD, TYPE_UNDEFINED, 0);
		act("The evil in $N's soul is burned away!", ch, 0, v, TO_CHAR, 0);

	}
	SET_BIT(obj->obj_flags.more_flags, ITEM_TOGGLE);
	obj->obj_flags.timer = 360;
	return eSUCCESS;

}


// When fighting an evil opponent, sancts PC
int eliara_combat(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{

   CHAR_DATA *vict = NULL;

   if(cmd) return eFAILURE;

   if(!ch || !ch->equipment || !ch->equipment[WIELD])
      return eFAILURE;

   if(real_object(30627) != ch->equipment[WIELD]->item_number)
   {
      remove_eliara(ch);
      return eFAILURE;
   }

   if(!(vict=ch->fighting)) return eFAILURE;
   if(!IS_EVIL(vict)) return eFAILURE;
   if(IS_AFFECTED(ch, AFF_SANCTUARY)) return eSUCCESS;

   act("Eliara glows brightly for a moment, its incandescent field of light surrounding $n in a glowing aura.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("Eliara glows brightly surrounding you in its protective aura!\r\n", ch);

   SETBIT(ch->affected_by, AFF_SANCTUARY);

   return eSUCCESS;
}

int eliara_non_combat(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{

   if(!ch) return eFAILURE;

   if(cmd == CMD_REMOVE && GET_POS(ch) == POSITION_FIGHTING && ch->equipment
      && ch->equipment[WIELD] && ch->equipment[WIELD]->item_number 
      == real_object(30627))
   {
      send_to_char("Eliara refuses to allow you to remove equipment during battle!\r\n", ch);
      return eSUCCESS;
   }

   if(GET_POS(ch) < POSITION_STANDING)
      return eFAILURE;

   remove_eliara(ch);

   return eFAILURE;
}

int carriage(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker) 
{
  // this ain't done yet...it's natashas half written proc 

   if(cmd > CMD_DOWN || cmd < CMD_NORTH)
      return eFAILURE;

   if(!obj)
      return eFAILURE;

   if(!obj->in_room)
      return eFAILURE;

   if(ch->in_room != obj->in_room)
      return eFAILURE;
   
   if(move_char(ch, real_room(START_ROOM)) == 0)
      return eFAILURE;

   do_look(ch, "\0", 15);

   return eSUCCESS;
}

int arenaporter(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   // only go off when a player types a command
   if(!cmd)
      return eFAILURE;

   // 20% of the time
   if(number(1, 5) == 1)
      return eFAILURE;

   if(!obj || !obj->in_room || !ch)
      return eFAILURE;

   if(ch->in_room != obj->in_room)
      return eFAILURE;

   if(!move_char(ch, real_room(number(17800, 17949))) == 0)
      return eFAILURE;

   if(ch->fighting)
   {
      stop_fighting(ch->fighting);
      stop_fighting(ch);
   }

   send_to_char("A dimensional hole swallows you.\r\nYou reappear elsewhere.\r\n", ch);
   act("$n fades out of existence.", ch, 0,0,TO_ROOM, INVIS_NULL);

   do_look(ch, "\0", 15);

   // return false so their command goes off
   return eFAILURE;
}

int movingarenaporter(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   long room = 17840;

   if(!cmd)
   {
     // move myself 10% of time
     if(number(1, 10) > 1)
       return eFAILURE;
     
     while(room == 17840)
       room = number(17800, 17849);

     move_obj(obj, real_room(room));
     return eSUCCESS;
   }

   if(!obj || !obj->in_room)
      return eFAILURE;

   if(ch->in_room != obj->in_room)
      return eFAILURE;

   if(!move_char(ch, real_room(number(17800, 17949))) == 0)
      return eFAILURE;

   send_to_char("A dimensional hole swallows you.\r\nYou reappear elsewhere.\r\n", ch);
   act("$n fades out of existence.", ch, 0,0,TO_ROOM, INVIS_NULL);

   if(ch->fighting)
   {
      stop_fighting(ch->fighting);
      stop_fighting(ch);
   }

   do_look(ch, "\0", 15);

   return eSUCCESS;
}

int restring_machine(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
  char name[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  struct obj_data * target_obj = NULL;

  if(cmd != CMD_RESTRING)
    return eFAILURE;

  act("The machine flashes and shoots sparks and smoke throughout the room.", ch, 0, 0, TO_ROOM, 0);
  send_to_char("The machine beeps and emits hollow a voice...\n", ch);

  half_chop(arg, name, buf);

  if(!*arg || !*name || !*buf )
  {
    send_to_char("'Restring Machine v2.1' *beep*'\r\n"
                 "'restring <Item> <Description>' *beep*'\r\n"
                 "'This requires up to 50 platinum.  *beep*'\r\n"
                 "'This will only work on Godload. *beep*'\r\n"
                 "'Be careful and type correctly.  No refunds.'\r\n"
                 , ch);
    return eSUCCESS;
  }

  if(!(target_obj = get_obj_in_list_vis(ch, name, ch->carrying)))
  {
    send_to_char("'Cannot find this item in your inventory.  *beep*'\n", ch);
    return eSUCCESS;
  }

  if(!IS_SET(target_obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    send_to_char("'The item must be godload.  *beep*'\n", ch);
    return eSUCCESS;
  }

  if( strlen(buf) > 80 ) {
    send_to_char("'The description cannot be longer than 80 characters. *beep*'\n", ch);
    return eSUCCESS;
  }

  if(GET_PLATINUM(ch) < (uint32)(GET_LEVEL(ch)) )
  {
    send_to_char("'Insufficient platinum.  *beep*'\n", ch);
    return eSUCCESS;
  }

  GET_PLATINUM(ch) -= (GET_LEVEL(ch));

//  dc_free(target_obj->short_description);
//  target_obj->short_description = (char *) dc_alloc(strlen(buf)+1, sizeof(char));
//  strcpy(target_obj->short_description, buf);
  char zarg[MAX_STRING_LENGTH];
  sprintf(zarg, "$B$7%s$R", buf);
  target_obj->short_description = str_hsh(zarg);

  send_to_char("\r\n'Beginning magical transformation process. *beep*'\r\n"
               "You put your item into the machine and close the lid.\r\n"
               "Smoke pours out of the machine and sparks fly out blackening the floor.\r\n"
               "Your item looks new!\r\n\r\n"
               , ch);

  save_char_obj(ch);
  return eSUCCESS;
}

int weenie_weedy(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   if (cmd)
     return eFAILURE;

   if(number(1, 1000) > 990) {
      if(obj->carried_by)
         send_to_room("Someone's weenie weedy doll says, 'BLARG!'\r\n", obj->carried_by->in_room);
      else if(obj->in_room != NOWHERE)
         send_to_room("a weenie weedy doll says, 'BLARG!'\r\n", obj->in_room);
      else if(obj->in_obj && obj->in_obj->carried_by)
         send_to_room("a muffled 'BLARG!' comes from a weenie weedy doll somewhere nearby.\r\n", obj->in_obj->carried_by->in_room);
   }

   return eSUCCESS;
}

// this should really be a azrack megaphone, but i'm too lazy to figure out how they
// work right now
// TODO - figure out how azrack's megaphone's work
int stupid_message(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   if (cmd)
     return eFAILURE;

  if(!obj || obj->in_room < 0)
     return eFAILURE;

  if(!zone_table[world[obj->in_room].zone].players)
     return eFAILURE;

   if(number(1, 10) == 1)
      send_to_room("The shadows swirl to reveal a face before you.\r\n"
                   "It speaks suddenly, 'Only with the key can you unlock the masters name' "
                   "and then fades away.\r\n", obj->in_room, TRUE);

   return eSUCCESS;
}

// If there is a player in rooms 8695-8699 (the pillars), then they cause a
// "balance" and we can remove the imp_only flag from the target room
int pagoda_balance(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)
     return eFAILURE;

   char_data * vict = NULL;
   int found = 0;

   for(int i = 8695; i < 8699; i++)
   {
      // TODO - should probably check to make sure these are valid rooms before we
      // use them.  Proc isn't used yet though, so no biggy.
      for(vict = world[real_room(i)].people; vict; vict = vict->next_in_room)
         if(IS_NPC(vict))
           found = 0;
         else { found = 1; break; }

      if(!found)
         break;
   }

   // If we aren't true here, then one of the rooms didn't have a PC in it
   if(!found)
      return eFAILURE;

   for(int j = 8695; j < 8699; j++)
      send_to_room("The weight of your body helps shift the balances.\r\n"
                   "You hear the poping of a magical barrier dissapating.\r\n\r\n", 
                   real_room(j));

   REMOVE_BIT(world[real_room(8699)].room_flags, IMP_ONLY);
   return eFAILURE;
}

// If players enter the room, pop a "imp_only" flag back on the room.
int pagoda_shield_restorer(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   if(!cmd) return eFAILURE; // only restore if a player is in the room

   if(!IS_SET(world[obj->in_room].room_flags, IMP_ONLY))
   {
      send_to_room("You hear the 'pop' of a magical barrier springing up.\r\n\r\n", obj->in_room);
      SET_BIT(world[obj->in_room].room_flags, IMP_ONLY);
   }

   return eFAILURE;
}

// stupid little item I made for an ex-gf so she could find me when she logged in
int phish_locator(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   char_data * victim = NULL;

   if(cmd != CMD_PUSH) // push
      return eFAILURE;

   if(!strstr(arg, "button"))
      return eSUCCESS;
   

   act("$n fiddles with a small fish-shaped device.", ch, 0,0,TO_ROOM, INVIS_NULL);

   if(!(victim = get_char("Pirahna")))
   {
      send_to_char("The locator beeps angrily and smoke starts to come out.\r\nPirahna is unlocatable.\r\n", ch);
      return eSUCCESS;
   }

   send_to_char("Found him!\r\n", ch);

   do_trans(victim, GET_NAME(ch), 9);
   return eSUCCESS;   
}


int generic_push_proc(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   char_data * victim;
   char_data * next_vict;

   if (cmd != CMD_PUSH) // push
      return eFAILURE;

   int obj_vnum = obj_index[obj->item_number].virt;

   switch(obj_vnum) {
      case 26723: // transporter in star-trek
        send_to_room("You hear a chiming electrical noise as the transporter hums to life.\n\r", obj->in_room);
        for(victim = world[obj->in_room].people; victim; victim = next_vict)
        {
          next_vict = victim->next_in_room;
          send_to_char("Your body is pulled apart and reassembled elsewhere!\r\n", victim);
          act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
          move_char(victim, 26802);
          act("$n slowly fades into existence.", victim, 0, 0, TO_ROOM, 0);
          do_look(victim, "", 9);
        }
        break;

      default:
        send_to_char("Whatever you pushed doesn't have an entry in the button push table.  Tell a god.\r\n", ch);
        logf(LOG_WORLD, IMMORTAL, "'Push' proc on obj %d without entry in proc table. (push_proc)\r\n", obj_vnum);
        break;
   }

   return eSUCCESS;
}

int portal_word(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   char junk[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   char_data * victim = NULL;

   if(!cmd && obj) {  // This is where we recharge
      if(obj->obj_flags.value[3]) {
         obj->obj_flags.value[3]--;
        if(0 == obj->obj_flags.value[3]) {
          if(obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->carried_by);
          else if(obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->equipped_by);
          else if(obj->in_obj && obj->in_obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->carried_by);
          else if(obj->in_obj && obj->in_obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->equipped_by);
        }
      }
   }
   // 11 = say
   if(cmd != CMD_SAY)                            return eFAILURE;
   if(!ch)                                  return eFAILURE;
   if(ch->equipment[HOLD] != obj)           return eFAILURE;

   half_chop(arg, arg1, junk);

   if(str_cmp("magiskhal", arg1))           return eFAILURE;

   if(ch->equipment[HOLD]->obj_flags.value[3] && GET_LEVEL(ch) < IMMORTAL) {
      send_to_char("The item seems to be recharging.\r\n", ch);
      return eSUCCESS;
   }
   act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("You quietly whisper 'magiskhal' into your hands.\r\n", ch);

   if(!(victim = get_char_vis(ch, junk))) {
      send_to_char("The box somehow seems......confused.\r\n", ch);
   } else {
      spell_portal(GET_LEVEL(ch), ch, victim, 0, 0);
      // set charge time
      ch->equipment[HOLD]->obj_flags.value[3] = 600;
   }
   act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
   return eSUCCESS;
}

int full_heal_word(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   char junk[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   char_data * victim = NULL;

   if(!cmd && obj) { // This is where we recharge
      if(obj->obj_flags.value[3]) {
         obj->obj_flags.value[3]--; 
        if(0 == obj->obj_flags.value[3]) {
          if(obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->carried_by);
          else if(obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->equipped_by);
          else if(obj->in_obj && obj->in_obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->carried_by);
          else if(obj->in_obj && obj->in_obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->equipped_by);
        }
      }
   }

   // 11 = say
   if(cmd != CMD_SAY)                            return eFAILURE;
   if(!ch)                                  return eFAILURE;
   if(ch->equipment[HOLD] != obj)           return eFAILURE;

   half_chop(arg, arg1, junk);

   if(str_cmp("heltlaka", arg1))           return eFAILURE;

   if(ch->equipment[HOLD]->obj_flags.value[3] && GET_LEVEL(ch) < IMMORTAL) {
      send_to_char("The item seems to be recharging.\r\n", ch);
      return eSUCCESS;
   }
   act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("You quietly whisper 'heltlaka' into your hands.\r\n", ch);

   if(!(victim = get_char_vis(ch, junk))) {
      send_to_char("The box somehow seems......confused.\r\n", ch);
   } else {
      spell_full_heal(GET_LEVEL(ch), ch, victim, 0, 0);
      spell_full_heal(GET_LEVEL(ch), ch, victim, 0, 0);
      spell_full_heal(GET_LEVEL(ch), ch, victim, 0, 0);
      // set charge time
      ch->equipment[HOLD]->obj_flags.value[3] = 300;
   }
   act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
   return eSUCCESS;
}

int mana_box(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   ch = obj->equipped_by;
   if(!ch)                                  return eFAILURE;
   if(ch->equipment[HOLD] != obj)           return eFAILURE;

   if((GET_MANA(ch)+8) < GET_MAX_MANA(ch))
      GET_MANA(ch) += 8;

   if(0 == number(0, 10))
      send_to_char("The box's magical power eases your mind.\r\n", ch);

   return eSUCCESS;
}

int fireshield_word(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   char junk[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];

   if(!cmd && obj) { // This is where we recharge
      if(obj->obj_flags.value[3]) {
         obj->obj_flags.value[3]--; 
        if(0 == obj->obj_flags.value[3]) {
          if(obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->carried_by);
          else if(obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->equipped_by);
          else if(obj->in_obj && obj->in_obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->carried_by);
          else if(obj->in_obj && obj->in_obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->equipped_by);
        }
     }
   }

   // 11 = say
   if(cmd != CMD_SAY)                            return eFAILURE;
   if(!ch)                                  return eFAILURE;
   if(ch->equipment[HOLD] != obj)           return eFAILURE;

   half_chop(arg, arg1, junk);

   if(str_cmp("feuerschild", arg1))           return eFAILURE;

   if(ch->equipment[HOLD]->obj_flags.value[3] && GET_LEVEL(ch) < IMMORTAL) {
      send_to_char("The item seems to be recharging.\r\n", ch);
      return eSUCCESS;
   }
   act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("You quietly whisper 'feuerschild' into your hands.\r\n", ch);

   spell_fireshield(GET_LEVEL(ch), ch, ch, 0, 0);
   // set charge time
   ch->equipment[HOLD]->obj_flags.value[3] = 900;

   act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
   return eSUCCESS;
}

int teleport_word(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   char junk[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   char_data * victim = NULL;

   if(!cmd && obj) { // This is where we recharge
      if(obj->obj_flags.value[3]) {
         obj->obj_flags.value[3]--; 
        if(0 == obj->obj_flags.value[3]) {
          if(obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->carried_by);
          else if(obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->equipped_by);
          else if(obj->in_obj && obj->in_obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->carried_by);
          else if(obj->in_obj && obj->in_obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->equipped_by);
        }
     }
   }

   // 11 = say
   if(cmd != CMD_SAY)                            return eFAILURE;
   if(!ch)                                  return eFAILURE;
   if(ch->equipment[HOLD] != obj)           return eFAILURE;

   half_chop(arg, arg1, junk);

   if(str_cmp("sbiadirsivia", arg1))           return eFAILURE;

   if(ch->equipment[HOLD]->obj_flags.value[3] && GET_LEVEL(ch) < IMMORTAL) {
      send_to_char("The item seems to be recharging.\r\n", ch);
      return eSUCCESS;
   }
   if(IS_SET(world[ch->in_room].room_flags, SAFE)) {
      send_to_char("The box doesn't respond.\r\n", ch);
      return eSUCCESS;
   }

   act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("You quietly whisper 'sbiadirsivia' into your hands.\r\n", ch);

   if(!(victim = get_char_room_vis(ch, junk))) {
      send_to_char("The box somehow seems......confused.\r\n", ch);
   } else {
      spell_teleport(GET_LEVEL(ch), ch, victim, 0, 0);
      // set charge time
      ch->equipment[HOLD]->obj_flags.value[3] = 1000;
   }
   act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
   return eSUCCESS;
}


int alignment_word(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   char junk[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];

   if(!cmd && obj) { // This is where we recharge
      if(obj->obj_flags.value[3]) {
         obj->obj_flags.value[3]--; 
        if(0 == obj->obj_flags.value[3]) {
          if(obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->carried_by);
          else if(obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->equipped_by);
          else if(obj->in_obj && obj->in_obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->carried_by);
          else if(obj->in_obj && obj->in_obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->equipped_by);
        }
     }
   }

   // 11 = say
   if(cmd != CMD_SAY)                            return eFAILURE;
   if(!ch)                                  return eFAILURE;
   if(ch->equipment[HOLD] != obj)           return eFAILURE;

   half_chop(arg, arg1, junk);

   if(str_cmp("moralevalore", arg1))           return eFAILURE;

   act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("You quietly whisper 'moralevalore' into your hands.\r\n", ch);
   if(ch->equipment[HOLD]->obj_flags.value[3] && GET_LEVEL(ch) < IMMORTAL) {
      send_to_char("The item seems to be recharging.\r\n", ch);
      return eSUCCESS;
   }

   if(!strcmp("good", junk))              GET_ALIGNMENT(ch) = 1000;
   else if(!strcmp("evil", junk))         GET_ALIGNMENT(ch) = -1000;
   else if(!strcmp("neutral", junk))      GET_ALIGNMENT(ch) = 0;
   else
      send_to_char("The box somehow seems......confused.\r\n", ch);

   // set charge time
   ch->equipment[HOLD]->obj_flags.value[3] = 500;

   act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
   return eSUCCESS;
}

int protection_word(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   char junk[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];

   if(!cmd && obj) { // This is where we recharge
      if(obj->obj_flags.value[3]) {
         obj->obj_flags.value[3]--; 
        if(0 == obj->obj_flags.value[3]) {
          if(obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->carried_by);
          else if(obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->equipped_by);
          else if(obj->in_obj && obj->in_obj->carried_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->carried_by);
          else if(obj->in_obj && obj->in_obj->equipped_by)
            send_to_char("Your enchanted box seems to be recharged.\n\r", obj->in_obj->equipped_by);
        }
     }
   }

   // 11 = say
   if(cmd != CMD_SAY)                            return eFAILURE;
   if(!ch)                                  return eFAILURE;
   if(ch->equipment[HOLD] != obj)           return eFAILURE;

   half_chop(arg, arg1, junk);

   if(str_cmp("protezione", arg1))           return eFAILURE;

   if(ch->equipment[HOLD]->obj_flags.value[3] && GET_LEVEL(ch) < IMMORTAL) {
      send_to_char("The item seems to be recharging.\r\n", ch);
      return TRUE;
   }
   act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("You quietly whisper 'protezione' into your hands.\r\n", ch);

   spell_armor(GET_LEVEL(ch), ch, ch, 0, 0);
   spell_bless(GET_LEVEL(ch), ch, ch, 0, 0);
   spell_protection_from_evil(GET_LEVEL(ch), ch, ch, 0, 0);
   spell_invisibility(GET_LEVEL(ch), ch, ch, 0, 0);
   spell_stone_skin(GET_LEVEL(ch), ch, ch, 0, 0);
   spell_resist_fire(GET_LEVEL(ch), ch, ch, 0, 0);
   spell_resist_cold(GET_LEVEL(ch), ch, ch, 0, 0);
   cast_barkskin(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, ch, 0, 0);

   // set charge time
   ch->equipment[HOLD]->obj_flags.value[3] = 1000;

   act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
   return eSUCCESS;
}

// Proc for handling any 'pull' actions.  Just put it on the object and put in an entry
// Lots of things in here will crash if you remove the zone and stuff, but you just have
// to assume some of these things will work.  If they don't, we got bigger problems anyway
int pull_proc(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, CHAR_DATA *invoker)
{
   if (cmd != CMD_PULL) // pull
      return eFAILURE;

   int obj_vnum = obj_index[obj->item_number].virt;

   switch(obj_vnum) {
      case 9529: // DK lever in captain's room
        // unlock the gate
        REMOVE_BIT(world[9531].dir_option[1]->exit_info, EX_LOCKED);
        REMOVE_BIT(world[9532].dir_option[3]->exit_info, EX_LOCKED);
        send_to_room("You hear a large clicking noise.\n\r", 9531, TRUE);
        send_to_room("You hear a large clicking noise.\n\r", 9532, TRUE);
        send_to_room("You hear a large clicking noise.\n\r", ch->in_room, TRUE);
        break;
      case 29203:
	if (obj_index[real_object(29202)].number > 0)
	{
		send_to_room("A compartment in the ceiling opens, but is it empty.\r\n",29258, TRUE);
		break;
	}
	send_to_room("A compartment in the ceiling opens, and a key drops to the ground.\r\n", 29258, TRUE);
	obj_to_room(clone_object(real_object(29202)), 29258);
	break;
      default:
        send_to_char("Whatever you pulled doesn't have an entry in the lever pull table.  Tell a god.\r\n", ch);
        logf(LOG_WORLD, IMMORTAL, "'Pull' proc on obj %d without entry in proc table. (pull_proc)\r\n", obj_vnum);
        break;
   }

   return eSUCCESS;
}

int szrildor_pass(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
  struct obj_data *p;
  // 30097
  if (cmd && cmd == CMD_EXAMINE)
  {
    char target[MAX_INPUT_LENGTH];
    one_argument(arg, target);
    if (!str_cmp(target, "daypass") || !str_cmp(target, "pass"))
    {
      char buf[2000];
      sprintf(buf, "There appears to be approximately %d minutes left of time before the pass expires.\r\n", ((1800 - obj->obj_flags.timer) * 4) / 60);
      send_to_char(buf, ch);
      return eSUCCESS;
    }
  }

  if (cmd)
    return eFAILURE;

  if (obj->obj_flags.timer == 0)
  {       // Just created - check if this is the first pass in existence and if so, repop zone 161
    bool first = TRUE;
    for (p = object_list; p; p = p->next)
    {
      if (obj_index[p->item_number].virt == 30097 && p != obj && p->obj_flags.timer != 0)  // if any exist that are not at 1800 timer
      {
        first = FALSE;
        break;
      }
    }
    if (first && real_room(30000) != -1)
    {
      int zone = world[real_room(30000)].zone;
      auto &character_list = DC::instance().character_list;
      for (auto &tmp_victim : character_list)
      {
        // This should never happen but it has before so we must investigate without crashing the whole MUD
        if (tmp_victim == 0)
        {
          produce_coredump(tmp_victim);
          continue;
        }
        if (GET_POS(tmp_victim) == POSITION_DEAD || tmp_victim->in_room == NOWHERE)
        {
          continue;
        }
        if (world[tmp_victim->in_room].zone == zone)
        {
          if (IS_NPC(tmp_victim))
          {
            for (int l = 0; l < MAX_WEAR; l++)
            {
              if (tmp_victim->equipment[l])
                extract_obj(unequip_char(tmp_victim, l));
            }

            while (tmp_victim->carrying) {
              extract_obj(tmp_victim->carrying);
            }

            extract_char(tmp_victim, TRUE);
          }
        }
      }
      reset_zone(world[real_room(30000)].zone);

    }

  }

  obj->obj_flags.timer++;
  struct obj_data *n;
  if (obj->obj_flags.timer >= 1800)
  {
    // once one expires, ALL expire.
    for (p = object_list; p; p = n)
    {
      n = p->next;
      if (obj_index[p->item_number].virt == 30097)
      {
        CHAR_DATA *v = nullptr;

        if (p->carried_by) {
          v = p->carried_by;
        } else if (p->in_obj) {
          v = p->in_obj->carried_by;
        }

        if (v)
        {
          send_to_char("The Szrildor daypass crumbles into dust.\r\n", v);
          extract_obj(p); // extract handles all variations of obj_from_char etc

          if (IS_PC(v) && v->in_room && real_room(30000) > 0 && world[v->in_room].zone == world[real_room(30000)].zone && v->in_room != real_room(30000) && v->in_room != real_room(30096))
          {
            if (GET_LEVEL(v) >= IMMORTAL)
            {
              act("As your pass expires and crumbles to dust, you begin to feel a bit fuzzy for a moment but due to immortal magics your head becomes clear.", v, 0, 0, TO_CHAR, 0);
              act("$n begins to look blurry for a moment but due to immortal magics they become sharp again.", v, 0, 0, TO_ROOM, 0);
            }
            else
            {
              act("As your pass expires and crumbles to dust, you begin to feel a bit fuzzy for a moment, then vanish into thin air", v, 0, 0, TO_CHAR, 0);
              act("$n begins to look blurry for a moment, then winks out of existence with a \"pop\"!", v, 0, 0, TO_ROOM, 0);
              move_char(v, real_room(30000));
              do_look(v, "", CMD_LOOK);

              struct mprog_throw_type *throwitem = NULL;
              throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct mprog_throw_type));
              throwitem->target_mob_num = 30033;
              strcpy(throwitem->target_mob_name, "");
              throwitem->data_num = 99;
              throwitem->delay = 0;
              throwitem->mob = TRUE; // This is, surprisingly, a mob
              throwitem->actor = v;
              throwitem->obj = NULL;
              throwitem->vo = NULL;
              throwitem->rndm = NULL;
              throwitem->opt = 0;
              throwitem->var = NULL;
              throwitem->next = g_mprog_throw_list;
              g_mprog_throw_list = throwitem;
            }
          }
        }
      }
    }
  }
  return eSUCCESS;
}

int szrildor_pass_checks(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
  // 30096
  if (cmd)
    return eFAILURE;

  int count = 0;
  auto &character_list = DC::instance().character_list;
  for (auto &i : character_list)
  {
    if (IS_NPC(i))
      continue;
    if (!i->in_room)
      continue;
    if (real_room(30000) > 0 && world[i->in_room].zone != world[real_room(30000)].zone)
      continue;
    if (GET_LEVEL(i) >= 100)
      continue;
    if (i->in_room == real_room(30000))
      continue;
    if (i->in_room == real_room(30096))
      continue;

    if (!search_char_for_item(i, real_object(30097), false) || (++count) > 4)
    {
      act("Jeff arrives and frowns.\r\n$B$7Jeff says, 'Hey! You don't have a pass. Get the heck outta here!'$R", i, 0, 0, TO_CHAR, 0);
      act("Jeff arrives and frowns at $n.\r\n$B$7Jeff says, 'Hey! You don't have a pass. Get the heck outta here!'$R", i, 0, 0, TO_ROOM, 0);

      if (GET_LEVEL(i) >= IMMORTAL)
      {
        act("As your pass expires and crumbles to dust, you begin to feel a bit fuzzy for a moment but due to immortal magics your head becomes clear.", i, 0, 0, TO_CHAR, 0);
        act("$n begins to look blurry for a moment but due to immortal magics they become sharp again.", i, 0, 0, TO_ROOM, 0);
      }
      else
      {
        act("As your pass expires and crumbles to dust, you begin to feel a bit fuzzy for a moment, then vanish into thin air", i, 0, 0, TO_CHAR, 0);
        act("$n begins to look blurry for a moment, then winks out of existence with a \"pop\"!", i, 0, 0, TO_ROOM, 0);
        move_char(i, real_room(30000));
        do_look(i, "", CMD_LOOK);
      }
    }
  }
  return eSUCCESS;
}





int moving_portals(struct char_data*ch, struct obj_data *obj, int cmd, 
char*arg, CHAR_DATA *invoker)
{
   char msg1[MAX_STRING_LENGTH],msg2[MAX_STRING_LENGTH];
   int low, high, room,time,sector = 0;
   if (cmd) return eFAILURE;

   switch (obj_index[obj->item_number].virt)
   {
      case 11300:
      case 11301:
      case 11302:
      case 11303: // Sea of Dreams ships
      case 11304:
      case 11305:
          low = 11300;
	  high = 11599;
	  time = 60;
	  sector = SECT_WATER_NOSWIM;
	  sprintf(msg1,"The ship sails off into the distance.");
	  sprintf(msg2,"A ship sails in.");
	  break;
      case 5911: // Carnival gates
	  low = 18100;
	  high = 18213;
	  time = 75;
	  sprintf(msg1,"The carnival breaks off and moves off into the distance.");
	  sprintf(msg2,"A band of wagons enter, and set up a carnival here.");
          break;
      default:
          return eFAILURE;
   }
   obj->obj_flags.timer--;

   if (obj->obj_flags.timer<=0)
   {
      obj->obj_flags.timer = time;
      while ((room = number(low, high))) {
          bool portal = FALSE;
          if (real_room(room) < 0) continue;
	  if (sector)
	    if (world[real_room(room)].sector_type != sector)
		continue;
          struct obj_data *o;
	  for (o = world[real_room(room)].contents; o; o = o->next_content)
		if (o->obj_flags.type_flag == ITEM_PORTAL) portal = TRUE;
        if (!portal) break;
      } // Find a room
      send_to_room(msg1,obj->in_room, TRUE);
      obj_from_room(obj);
      obj_to_room(obj, room);
      send_to_room(msg2,obj->in_room, TRUE);
      return eSUCCESS;
   }
   return eFAILURE;
}


// searches for if a certain mob is alive.  If so, you cannot use magic in this room.
int no_magic_while_alive(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, CHAR_DATA *invoker)
{
   if(cmd)
      return eFAILURE;

   if(obj->in_room < 0)
      return eFAILURE;

   char_data * vict = world[obj->in_room].people;

   for(;vict;vict = vict->next_in_room) {
      if(IS_NPC(vict) && (
          mob_index[vict->mobdata->nr].virt == 9544
            // to add a new mob to this list, just add || and the next check
          )
        )
        break;
   }

   if(vict) {
      if(!IS_SET(world[obj->in_room].room_flags, NO_MAGIC))
         send_to_room("With an audible whoosh, the flow of magic is sucked from the room.\n\r", obj->in_room);
      SET_BIT(world[obj->in_room].room_flags, NO_MAGIC);
   }
   else {
      if(IS_SET(world[obj->in_room].room_flags, NO_MAGIC))
         send_to_room("With a large popping noise, the flow of magic returns to the room.\n\r", obj->in_room);
      REMOVE_BIT(world[obj->in_room].room_flags, NO_MAGIC);
   }
   return eSUCCESS;
}

// Send a message to all rooms on a boat
void send_to_boat(int boat, char * message)
{
  switch(boat) {
    case 9531: // dk boat
       send_to_room(message, 9522, TRUE);
       send_to_room(message, 9523, TRUE);
       send_to_room(message, 9524, TRUE);
       send_to_room(message, 9525, TRUE);
       send_to_room(message, 9587, TRUE);
       break;
    default:
      break;
  }
}

// How many stops, order of stops (will reverse order on way back)
int dk_boat[] = 
{ 8, 9593, 9510, 9509, 9508, 9521, 9507, 9506, 9505 };


// boat values [pos in travel list] [timer] [boat-entry-room] [time-between-moves]
// pos in travel list is NEGATIVE value on return trip

// handles the movement of ocean-going boats or ferries
// make sure you also look at "leave_boat_proc" if you use this

// Also, make sure you update "send_to_boat" for the messages
//
int boat_proc(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, CHAR_DATA *invoker)
{
   if(cmd && cmd != CMD_ENTER)
      return eFAILURE;

   if(obj->in_room < 0)
      return eFAILURE;   // someone loaded me

   // figure out which boat I am
   int * boat_list = NULL;
   switch(obj_index[obj->item_number].virt) {
     case 9531:
       boat_list = dk_boat;
       break;
     default: 
       logf(LOG_BUG, IMMORTAL, "Illegal boat proc.  Item %d.", obj_index[obj->item_number].virt);
       break;
   }

   if(cmd) {
      // get on the boat
      act("$n boldly boards $p.", ch, obj, 0, TO_ROOM, 0);
      act("You board $p.", ch, obj, 0, TO_CHAR, 0);
      move_char(ch, obj->obj_flags.value[2]);
      act("$n climbs aboard.", ch, 0, 0, TO_ROOM, 0);
      do_look(ch, "", 9);
      return eSUCCESS;
   }

   // timer pulsed.  Update
   obj->obj_flags.value[1]--;
   
   // boat pulsed.  Time to move
   if(!obj->obj_flags.value[1]) {
     int move_to;
     // reset timer
     obj->obj_flags.value[1] = obj->obj_flags.value[3];
     if(obj->obj_flags.value[0] < 0) // on way back
     {
       obj->obj_flags.value[0]++;
       move_to = boat_list[(obj->obj_flags.value[0] * -1)];
       if(obj->obj_flags.value[0] == -1) // at beginning
       {
         obj->obj_flags.value[0] = 1;
         send_to_boat(obj_index[obj->item_number].virt, "The ship docks at its destination.\n\r");
       }
     }
     else 
     {
       obj->obj_flags.value[0]++;
       move_to = boat_list[obj->obj_flags.value[0]];
       if(obj->obj_flags.value[0] == boat_list[0]) // at beginning
       {
         obj->obj_flags.value[0] *= -1;
         send_to_boat(obj_index[obj->item_number].virt, "The ship docks at its destination.\n\r");
       }
     }
     send_to_room("The ship sails away.\n\r", obj->in_room, TRUE);
     send_to_boat(obj_index[obj->item_number].virt, "The ship sails onwards.\n\r");
     obj_from_room(obj);
     obj_to_room(obj, move_to);
     send_to_room("A ship sails in.\n\r", obj->in_room, TRUE);
   }
   return eSUCCESS;
}

// Depending on which boat we're on, exit the boat if we're not 'at sea'
int leave_boat_proc(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, CHAR_DATA *invoker)
{
   obj_data * obj2;
   int i;

   if(cmd != CMD_LEAVE)  // leave
      return eFAILURE;

   if(obj->in_room < 0)
      return eFAILURE;   // someone loaded me

   // switch off depending on what item we are
   switch(obj_index[obj->item_number].virt) {
     case 9532: // dk boat ramp
       // find the dk boat (9531)
       i = real_object(9531);
       for(obj2 = object_list; obj2; obj2 = obj2->next) 
       {
         if(obj2->item_number == i)
           break;
       }

       if(obj2 == NULL) 
       {
         send_to_char("Cannot find your boat obj.  BUG.  Tell a god.\n\r", ch);
         return eSUCCESS;
       }

       if(obj2->in_room == dk_boat[1]) {
         act("$n disembarks from $p.", ch, obj2, 0, TO_ROOM, 0);
         act("You disembark from $p.", ch, obj2, 0, TO_CHAR, 0);
         move_char(ch, obj2->in_room);
         act("$n disembarks from $p.", ch, obj2, 0, TO_ROOM, 0);
         do_look(ch, "", 9);
         return eSUCCESS;
       }

       if(obj2->in_room == dk_boat[dk_boat[0]]) {
         act("$n disembarks from $p.", ch, obj2, 0, TO_ROOM, 0);
         act("You disembark from $p.", ch, obj2, 0, TO_CHAR, 0);
         move_char(ch, obj2->in_room);
         act("$n disembarks from $p.", ch, obj2, 0, TO_ROOM, 0);
         do_look(ch, "", 9);
         return eSUCCESS;
       }

       send_to_char("You can't just leave the ship in the middle of the Blood Sea!\n\r", ch);
       return eSUCCESS;
       break;
     default: 
       logf(LOG_BUG, IMMORTAL, "Illegal boat proc.  Item %d.", obj_index[obj->item_number].virt);
       break;
   }

   return eSUCCESS;
}


#define BONEWRACK_ROOM      9597
#define GAIOT_AVATAR        9622

// This proc waits for players to enter the room.  Once they do, it echos messages
// and loads the mob into the room.  Can work for multiple messages with a 'wait' state
// obj values:  [current pulse] [unused] [unused] [unused]
//
int mob_summoner(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, CHAR_DATA *invoker)
{
   char_data * vict;

   if(cmd)
      return eFAILURE;

   if(obj->in_room < 0)
      return eFAILURE;   // someone loaded me

   // see if we have any players in room
   for(vict = world[obj->in_room].people; vict; vict = vict->next_in_room)
     if(!IS_NPC(vict))
       break;

   // no?  reset pulse state and get out
   if(!vict) {
     obj->obj_flags.value[0] = 0;
     return eSUCCESS;
   }

   switch(obj->in_room) {
     case BONEWRACK_ROOM:
       // find bonewrack
       vict = get_mob_vnum(9535);
       if(!vict || vict->in_room == BONEWRACK_ROOM)
         return eSUCCESS;

       switch(obj->obj_flags.value[0]) {
         case 0:
           send_to_room("The shadows in the room begin to shift and slide in tricks of the light.\n\r", BONEWRACK_ROOM, TRUE);
           break;
         case 1:
           send_to_zone("A loud roar echos audibly through the entire kingdom.\n\r", world[obj->in_room].zone);
           break;
         case 2:
           send_to_room("The dragon $B$2Bonewrack$R flies in from above!\n\r", BONEWRACK_ROOM, TRUE);
           move_char(vict, BONEWRACK_ROOM);
           obj->obj_flags.value[0] = 0;
           break;
         default:
           break;
       }
       break;

     case GAIOT_AVATAR:
       // find avatar
       vict = get_mob_vnum(9526);
       if(!vict || vict->in_room == GAIOT_AVATAR)
         return eSUCCESS;

       switch(obj->obj_flags.value[0]) {
         case 0:
           send_to_room("In the distance a winged creature can be seen flying towards you.\n\r", GAIOT_AVATAR, TRUE);
           break;
         case 1:
           send_to_room("The winged creature flies closer and closer.\n\r", GAIOT_AVATAR, TRUE);
           break;
         case 2:
           send_to_room("The creature shatters in illusion!\n\r", GAIOT_AVATAR, TRUE);
           move_char(vict, GAIOT_AVATAR);
           obj->obj_flags.value[0] = 0;
           break;
         default:
           break;
       }
       break;

     default:
       break;
   }

   obj->obj_flags.value[0]++;

   return eSUCCESS;
}

// Takes care of lighting back up a room when globe of darkness wears off
// values:  [time left] [how dark] [unused] [unused]
//
int globe_of_darkness_proc(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, CHAR_DATA *invoker)
{
   if(cmd)
      return eFAILURE;

   if(obj->in_room < 0)
      return eFAILURE;   // someone loaded me

   if(obj->obj_flags.value[0] < 1) {
      // time to kill myself
      world[obj->in_room].light += obj->obj_flags.value[1]; // light back up
      send_to_room("The globe of darkness fades brightening the room some.\n\r", obj->in_room, TRUE);
      extract_obj(obj);
   }
   else obj->obj_flags.value[0]--;

   return eSUCCESS;
}



int hornoplenty(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, CHAR_DATA *invoker)
{
   if(cmd)
      return eFAILURE;

   if(!obj || obj->contains)
      return eFAILURE;

   if(number(0, 100))
      return eFAILURE;

   struct obj_data * newobj = NULL;
   int objnum = real_object(3170); // chewy tuber
   if(objnum < 0)
   {
      logf(LOG_BUG, IMMORTAL, "Horn o plenty load obj incorrent.");
      return eFAILURE;
   }

   newobj = clone_object(objnum);

   obj_to_obj(newobj, obj);
   return eSUCCESS;
}

int gl_dragon_fire(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)   return eFAILURE;

   if(number(1, 100) > 5)
      return eFAILURE;

   send_to_char("The head of your dragon staff animates and breathes $B$4fire$R all around you!\r\n", ch);
   act("The head of the dragon staff in $n's hands animates and begins to breath fire!", 
              ch, obj, 0, TO_ROOM, 0);

   return cast_fire_breath(10, ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, 0);
}

int dk_rend(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)   return eFAILURE;

   if(number(1, 100) > 5)
      return eFAILURE;

   act("The $o rips deeply into $N rending $S body painfully!", 
              ch, obj, ch->fighting, TO_ROOM, 0);

   GET_HIT(ch->fighting) /= 2;
   if(GET_HIT(ch->fighting) < 1)
     GET_HIT(ch->fighting) = 1;

   return eSUCCESS;
}

int magic_missile_boots(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WEAR_FEET] != obj)      return eFAILURE;

   if(number(0, 1))
      return eFAILURE;

   act("The $o around $n's feet glows briefly and releases a magic missle spell!", 
              ch, obj, ch->fighting, TO_ROOM, 0);
   send_to_char("Your boots glow briefly and release a magic missle spell!\r\n", ch);

   return spell_magic_missile((GET_LEVEL(ch)/2), ch, ch->fighting, 0, 0);
}


int shield_combat_procs(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WEAR_SHIELD] != obj)    return eFAILURE;

   switch(obj_index[obj->item_number].virt) {
     case 2715:  // shield of ares
       if(number(0, 3))
         return eFAILURE;

       act("$n's $o glows yellow charging up with electrical energy.", ch, obj, ch->fighting, TO_ROOM, 0);
       send_to_char("Your shield glows yellow as it charges up with electrical energy.\r\n", ch);
       return spell_lightning_bolt((GET_LEVEL(ch)/2), ch, ch->fighting, 0, 0);
     break;
     case 555: // wicked boneshield
	if (number(0,9))
	  return eFAILURE;
	act("The spikes $n's $o glimmer brightly.",ch, obj,ch->fighting,TO_ROOM,0);
	send_to_char("The spikes on your shield glimmer brightly.\r\n",ch);
	return spell_cause_critical(GET_LEVEL(ch), ch, ch->fighting, 0, 0);
	break;
     case 5208: // thalos beholder shield
       if(number(0, 4))
         return eFAILURE;

       act("$n's $o begins to tremble violently upon contact with $N.", ch, obj, ch->fighting, TO_ROOM, NOTVICT);
       act("$n's $o begins to tremble violently upon contact with you!", ch, obj, ch->fighting, TO_VICT, 0);
       send_to_char("Your shield begins to violently shake after the hit!\r\n", ch);
       return spell_cause_serious((GET_LEVEL(ch)/2), ch, ch->fighting, 0, 0);
     break;

     default:
       break; 
   }

   return eFAILURE;
}

int generic_weapon_combat(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   extern int top_of_objt;

   if(cmd)                                    return eFAILURE;
   if(!ch || !ch->fighting)                   return eFAILURE;
   if(!obj || (
       ch->equipment[WIELD] != obj &&
       ch->equipment[SECOND_WIELD] != obj))   return eFAILURE;

   if(obj->item_number < 0 || obj->item_number > top_of_objt) {
      logf(IMMORTAL, LOG_BUG, "generic_weapon_combat: illegal obj->item_number");
      return eFAILURE;
   }

   switch(obj_index[obj->item_number].virt) {
     case 16903:  // valhalla hammer
       if(number(1, 100) > GET_DEX(ch)/4)
         break;
       send_to_char("The hammer begins to hum and strikes out with the power of Thor!\r\n", ch);
       act("$n's hammer begins to hum and strikes out with the power of Thor!", 
              ch, obj, 0, TO_ROOM, 0);
       return spell_lightning_bolt((GET_LEVEL(ch)/2), ch, ch->fighting, 0, 0);

     case 19327:  // EC Icicle
       if(number(1, 100) < GET_DEX(ch)/4)
         break;
       send_to_char("The Icicle begins to pulse repidly...\r\n", ch);
       act("$n's $o begins to pulse rapidly...", 
              ch, obj, 0, TO_ROOM, 0);
       return spell_icestorm((GET_LEVEL(ch)/2), ch, ch->fighting, 0, 0);

     default:
       send_to_char("Weapon with invalid generic_weapon_combat, tell an Immortal.\r\n", ch);
       break;
   }
   return eFAILURE;
}

// item players can buy to find the ToHS
int TOHS_locator(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   obj_data * victim = NULL;

   if(cmd != CMD_PUSH) // push
      return eFAILURE;

   if(!strstr(arg, "button"))
      return eFAILURE;

   act("$n pushes a small button then holds a looking glass to $s face.", ch, 0,0,TO_ROOM, INVIS_NULL);
   send_to_char("You push the small button and then hold the looking glass to your face peering through it.\r\n\r\n", ch);

   // 1406 is the portal 'rock' you enter to get to Tohs
   int searchnum = real_object(1406);

   for(victim = object_list; victim; victim = victim->next)
     if(victim->item_number == searchnum)
       break;

   if(!victim || victim->in_room < 0) // couldn't find it?!
   {
     send_to_char("The tower seems to not be there!?!!\r\n", ch);
     return eSUCCESS;
   }

   searchnum = ch->in_room;
   move_char(ch, victim->in_room);
   do_look(ch, "", 9);
   move_char(ch, searchnum);

   return eSUCCESS;   
}

/*int no_magic_item(struct char_data *ch, struct obj_data *obj, cmd, char 
*arg, CHAR_DATA *invoker)
{ // mobdata last_direction
   if (cmd)  // Not activated through commands..
     return eFAILURE;
//   if (   
}
*/
int gotta_dance_boots(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   void make_person_dance(char_data * ch);

   if(cmd)
      return eFAILURE;

   if(!obj->equipped_by)
      return eFAILURE;

   if(number(0, 3))
      return eFAILURE;

   act("$n eyes widen and $e begins to shake violently.", obj->equipped_by, 0,0,TO_ROOM, INVIS_NULL);
   send_to_char("Your boots grasp violently to your legs and rhythmic urges flood your body.\r\n", obj->equipped_by);
   do_say(obj->equipped_by, "I...I.....I've gotta dance!!!!", 9);
   make_person_dance(obj->equipped_by);
   send_to_char("You slump back down, exhausted.\r\n", obj->equipped_by);
   if(GET_LEVEL(obj->equipped_by) <= MORTAL)
      WAIT_STATE(obj->equipped_by, PULSE_VIOLENCE*3);

   return eSUCCESS;   
}


int random_dir_boots(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)
      return eFAILURE;

   if(!obj->equipped_by)
      return eFAILURE;

   if(number(0, 3))
      return eFAILURE;

   act("$n boots just keep on going!", obj->equipped_by, 0,0,TO_ROOM, INVIS_NULL);
   send_to_char("Your boots just keep on running!\r\n", obj->equipped_by);

   char dothis[32];

   strcpy(dothis, dirs[number(0, 5)]);

   return command_interpreter(obj->equipped_by, dothis);
}

// WARNING - uses obj_flags.value[3] to store stuff

int noremove_eq(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
   if(cmd && cmd != CMD_REMOVE)
      return eFAILURE;
   if(!obj->equipped_by)
      return eFAILURE;
   if(!cmd && obj->obj_flags.value[3] > 0) {
      obj->obj_flags.value[3]--;
      if(!obj->obj_flags.value[3])
         csendf(obj->equipped_by, "The %s loses it's grip on your body.\r\n", obj->short_description);
      return eSUCCESS;
   }
   if(!cmd && obj->obj_flags.value[3] <= 0) {
      if(number(0, 4))
         return eSUCCESS;
      csendf(obj->equipped_by, "The %s clamps down onto your body locking your equipment in place!\r\n",
             obj->short_description);
      obj->obj_flags.value[3] = 5;
      return eSUCCESS;
   }
   if(obj->obj_flags.value[3] > 0) {
      csendf(obj->equipped_by, "The %s refuses to let you remove anything!\r\n", obj->short_description);
      return eSUCCESS;
   }
   return eFAILURE;
}



int glove_combat_procs(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WEAR_HANDS] != obj)     return eFAILURE;

   int dam;

   switch(obj_index[obj->item_number].virt) {
     case 9806:  // muddy gloves
       if(number(0, 17))
         return eFAILURE;

       dam = dice(1, GET_LEVEL(ch));
       act("The mud on $n's gloves spoils $N's flesh causing boils.", ch, obj, ch->fighting, TO_ROOM, NOTVICT);
       act("The mud on $n's gloves spoils your flesh causing boils.", ch, obj, ch->fighting, TO_VICT, 0);
       send_to_char("The mud on your gloves spoils the flesh of your enemy.\r\n", ch);
       return damage(ch, ch->fighting, dam, TYPE_MAGIC, TYPE_UNDEFINED, 0);
     break;

     case 4818:
	if (number(0,19))
	  return eFAILURE;
	return spell_burning_hands(ch->level, ch, ch->fighting, NULL, 50);
     case 4819:
	if (number(0,19))
	  return eFAILURE;
	return spell_chill_touch(ch->level, ch, ch->fighting, NULL, 50);
     case 21718:
      if (affected_by_spell(ch, BASE_SETS+SET_SAIYAN))
      {
        if (number(0,19)) return eFAILURE;
        return spell_sparks(ch->level, ch, ch->fighting, NULL, 0);
      }
      break;
     case 19503: // Gloves of the Dreamer
       if(number(0, 17))
         return eFAILURE;

       act("$n's $o momentarily pulse with a $B$7white light$R.", ch, obj, 0, TO_ROOM, 0);
       send_to_char("Your gloves momentarily pulse with a $B$7white light$R.\r\n", ch);
       return spell_cure_serious(30, ch, ch, 0, 50);
     break;
      case 506:
	if (number(0,33) || !ch->fighting)
	  return eFAILURE;
       act("$n's $o begin pulse with a blinding white light for a moment.", ch, obj, 0, TO_ROOM, 0);
       send_to_char("Your gloves begin to pulse with a blinding white light for a moment.\r\n", ch);
       return spell_souldrain(60, ch, ch->fighting, 0, 100);
     default:
       break; 
   }

   return eFAILURE;
}



vector<string> sword_non_combat;
vector<string> sword_class_specific_combat;
vector<string> sword_combat;

void do_talking_init()
{

  string buf;

  /* GENERIC NONCOMBAT MESSAGE */ 
  buf = "Who wrote such crappy dialogue for me? Pirahna?";
  sword_non_combat.push_back(buf);
  buf = "Etala is my bitch.";
  sword_non_combat.push_back(buf);
  buf = "I used to get drunk all the time with the Avenger until it found religion and got all holy.";
  sword_non_combat.push_back(buf);
  buf = "Do you think these gems on my hilt make me look fat?";
  sword_non_combat.push_back(buf);
  buf = "*sigh*";
  sword_non_combat.push_back(buf);
  buf = "I'm bored. Go kill something.....";
  sword_non_combat.push_back(buf);
  buf = "Dude... just this once. Hold me over your head and shout, \"By the power of Grayskull!\"";
  sword_non_combat.push_back(buf);
  buf = "I wish my hilt could do that cool curly thing like in Thundercats.";
  sword_non_combat.push_back(buf);
  buf = "Ah, I remember the old days fighting with Musashi.";
  sword_non_combat.push_back(buf);
  buf = "The guy that bought me got the material really cheap. He said it was a \"steel\". HA!";
  sword_non_combat.push_back(buf);
  buf = "Really... I'm not really with this guy, we just sorta fighting together at the moment.";
  sword_non_combat.push_back(buf);
  buf = "Would you just stab him in the back and put me out of this misery?";
  sword_non_combat.push_back(buf);
  buf = "10,000 gold if you pinch his ass right now.";
  sword_non_combat.push_back(buf);
  buf = "Seriously. How can you stand grouping with this moron?";
  sword_non_combat.push_back(buf);
  buf = "Psst! RK him if he goes AFK.";
  sword_non_combat.push_back(buf);
  buf = "Pretend to be a chick, get his password, log in as him and just sacrifice me. Please! I'm begging.";
  sword_non_combat.push_back(buf);
  buf = "Hey, later on lets stop by and get the latest copy of Sword & Dagger. They've got a hot centerfold this month.";
  sword_non_combat.push_back(buf);
  buf = "I just love that Gogo Yubari character; \"Or is it I... who has penetrated you?\" Classic!";
  sword_non_combat.push_back(buf);
  buf = "When I was a kid, I wanted to grow up to be a scimitar so I could be a pirate.";
  sword_non_combat.push_back(buf);
  buf = "If you don't wipe me down before putting me in the scabbard, I'm going to slice your throat while you're sleeping.";
  sword_non_combat.push_back(buf);
  buf = "I miss the old days with Mom and Dad back when I was just a little dagger.";
  sword_non_combat.push_back(buf);
  buf = "When they say a sword is an extension of your body, they mean your arm, not penis, you ass.";
  sword_non_combat.push_back(buf);
  buf = "I'm the sword that killed Enigo Montoya's father.";
  sword_non_combat.push_back(buf);
  buf = "Dude.... I totally think Bedbug likes you.";
  sword_non_combat.push_back(buf);
  buf = "That whole sword in the stone thing is bull. Like anyone would ever do that.";
  sword_non_combat.push_back(buf);
  buf = "Some swords prefer the waist, and some swords prefer the back. I'm a breast-sword myself.";
  sword_non_combat.push_back(buf);
  buf = "Fuck sakes, with how much you use me, I may as well team up with a spoon and a fork and head to Uncle Juan's!";
  sword_non_combat.push_back(buf);
  buf = "You know that sweet looking sword Mel Gibson holds in Braveheart? Yeah... I've tagged that.";
  sword_non_combat.push_back(buf);
  buf = "Devil-fang Cestus used to complain to me all the time about it's owner. Hairy palms.";
  sword_non_combat.push_back(buf);
  buf = "Today is one of those days when you just wanna go out and disembowel some bunnies.";
  sword_non_combat.push_back(buf);
  buf = "Hey... Lets head over to Hyperborea and listen to Frosty.";
  sword_non_combat.push_back(buf);

  

  /* GENERIC COMBAT MESSAGES*/
  buf = "After this fight, I want you to use me to play air-guiter. Rock on!";
  sword_combat.push_back(buf);
  buf =  "If you're happy and you know it, shove me deep! *shove* *shove* ...";
  sword_combat.push_back(buf);  
  buf = "No no idiot, swing at his other side!";
  sword_combat.push_back(buf);
  buf = "I'm going to wear your entrails like a scarf.";
  sword_combat.push_back(buf);
  buf = "At least you can do one thing. If you fought as well as you performed in bed, I'd have a new owner by now.";
  sword_combat.push_back(buf);
  buf = "Ooooooh yeah... tickle a little lower under the hilt... OH! I like that!";
  sword_combat.push_back(buf);
  buf = "You want me to penetrate THAT?!?";
  sword_combat.push_back(buf);
  buf =  "Why don't you try shoving your own 'sword' in there first? I don't know where they've been either.";
  sword_combat.push_back(buf);
  buf = "Ohhh gross.... do you know how hard this blood is to get off?";
  sword_combat.push_back(buf);
  buf = "Swordfighting is like a dance and *I* get a partner with two left feet.";
  sword_combat.push_back(buf);
  buf = "Your liver is going to taste lovely with some fava beans and a nice Chianti!";
  sword_combat.push_back(buf);
  buf = "That's MISTER Ghaerad to you, punk!";
  sword_combat.push_back(buf);




}



int chaosblade(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
	if (cmd) return eFAILURE;
	if (!obj->equipped_by) return eFAILURE;

	if ((++obj->obj_flags.timer) > 4)
	{
		int dam = number(175,250);
		obj->obj_flags.timer = 0;
		if (GET_HIT(obj->equipped_by) * 30 / 1000 > dam)
		{
			dam = GET_HIT(obj->equipped_by) * 30 / 1000;
		}
		if (dam >= GET_HIT(obj->equipped_by) )
		{
			dam = GET_HIT(obj->equipped_by) - 1;
		}
		if (dam > 0)
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf,"%d", dam);
			send_damage("The Chaos Blade hungers!  You are drained for | damage.",  obj->equipped_by, 0, 0, buf, "The Chaos Blade hungers!  You feel your life force being drained!", TO_CHAR);
			send_damage("The katana in $n's hand pulses with a dull red glow as it drains their life force for | damage!",  obj->equipped_by, 0, 0, buf, "The katana in $n's hand pulses with a dull red glow as it drains their life force!", TO_ROOM);
			GET_HIT(obj->equipped_by) -= dam;
		}
	}
	return eSUCCESS;
}

int rubybrooch(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
	if (cmd) return eFAILURE;
	if (!obj->equipped_by) return eFAILURE;

	if (obj->obj_flags.timer == 39)
	{
		csendf(obj->equipped_by, "You feel the ruby brooch's grip upon your neck loosen slightly.\r\n");
	}
	++obj->obj_flags.timer;

	if ((obj->obj_flags.timer % 4) == 0 )
	{
		if ((obj->obj_flags.timer) == 44)
		{
			obj->obj_flags.timer = 40; // 40+ = can be removed
		}
		int dam = number(75,150);
		if (dam >= GET_HIT(obj->equipped_by) )
		{
			dam = GET_HIT(obj->equipped_by) - 1;
		}
		if (dam > 0)
		{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "%d", dam);
			send_damage("The ruby brooch squeezes your neck painfully for | damage!",  obj->equipped_by, 0, 0, buf, "The ruby brooch squeezes your neck painfully!", TO_CHAR);
			send_damage("A ruby brooch constricts $n's neck for | damage and they cough violently.",  obj->equipped_by, 0, 0, buf, "A ruby brooch constricts $n's neck and they cough violently.", TO_ROOM);
			GET_HIT(obj->equipped_by) -= dam;
		}
	}
	return eSUCCESS;
}

int eternitystaff(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
	if (cmd) return eFAILURE;
	if (!obj->equipped_by) return eFAILURE;

	if ((++obj->obj_flags.timer) > 4)
	{
		int dam = number(175,200);
		obj->obj_flags.timer = 0;
		if (GET_MANA(obj->equipped_by) * 30 / 1000 > dam)
		{
			dam = GET_MANA(obj->equipped_by) * 30 / 1000;
		}

		if (dam >= GET_MANA(obj->equipped_by) )
		{
			dam = GET_MANA(obj->equipped_by) - 1;
		}
		if (dam > 0)
		{
      GET_MANA(obj->equipped_by) -= dam;
			csendf(obj->equipped_by, "Your body hemorrhages %d mana as you struggle to control The Eternity Staff.\r\n", dam);
			

			act("$n is wracked by magical energies!", obj->equipped_by, 0, 0, TO_ROOM, 0);
		}
	}
	return eSUCCESS;
}

int talkingsword(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
                 CHAR_DATA *invoker)
{
  char_data *vict = NULL;
  int unequip = -1;
  static bool init_done = false;
  if (cmd)
  {
    if (cmd == CMD_GAG && (!str_cmp(arg, " sword") || !str_cmp(arg, " ghaerad")) && obj->equipped_by)
    {
      char buf2[MAX_STRING_LENGTH] = "$B$7Ghaerad, Sword of Legends says, '";

      if (IS_SET(obj->obj_flags.more_flags, ITEM_TOGGLE))
      {
        REMOVE_BIT(obj->obj_flags.more_flags, ITEM_TOGGLE);
        strcat(buf2, "And I'm back! Couldn\'t live without me eh?'$R\n\r");
        send_to_room(buf2, obj->equipped_by->in_room, true);
      }
      else
      {
        SET_BIT(obj->obj_flags.more_flags, ITEM_TOGGLE);
        strcat(buf2, "Fine, I will keep quiet for a while, but you will miss me!'$R\n\r");
        send_to_room(buf2, obj->equipped_by->in_room, true);
      }
      return eSUCCESS;
    }
    else
    {
      return eFAILURE;
    }
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_TOGGLE))
  {
    return eFAILURE;
  }
  if (!init_done)
  {
    init_done = true;
    do_talking_init();
  }

  if (obj->equipped_by)
    vict = obj->equipped_by;

  if (!vict)
    return eFAILURE;

  if (vict->pcdata && vict->desc == nullptr)
  {
    return eFAILURE;
  }

  if (obj->obj_flags.value[0] > 0)
  {
    obj->obj_flags.value[0]--;
  }

  if (obj->obj_flags.value[0] == 0)
  {
    vector<string> tmp;
    string buf;

    if (GET_POS(vict) == POSITION_FIGHTING)
    {
      tmp = sword_combat;
      if (IS_NPC(vict->fighting) && GET_LEVEL(vict->fighting) > 99)
      {
        buf = "Are you sure you can win this one? I mean.... I'll be ok, but I'm pretty sure you're screwed.";
        tmp.push_back(buf);
      }
      if (IS_NPC(vict->fighting) && (GET_LEVEL(vict) - GET_LEVEL(vict->fighting)) > 40)
      {
        buf = "Oh come on... this is fuckin' embarrassing...";
        tmp.push_back(buf);
        unequip = tmp.size() - 1;
      }
      switch (GET_RACE(vict->fighting))
      {
      case RACE_HOBBIT:
        buf = "Oh, that is just gross! Like, seriously, shave your feet some time.";
        tmp.push_back(buf);
        break;
      case RACE_PIXIE:
        buf = "Why are you using me to fight a pixie? Get a fly swatter or something.";
        tmp.push_back(buf);
        break;
      case RACE_ELVEN:
        buf = "I hate elves. Fuckin' skinnier than I am. Go eat a burger.";
        tmp.push_back(buf);
        break;
      case RACE_DWARVEN:
        buf = "Cut him off at the knees! Oh, wait... looks like someone already did.";
        tmp.push_back(buf);
        break;
      case RACE_TROLL:
        buf = "I hate trolls. This smell isn't going to go away for weeks. ARGH!";
        tmp.push_back(buf);
        break;
      }
      switch (GET_CLASS(vict->fighting))
      {
      case CLASS_MONK:
        buf = "LOVE MONK AND STUN!";
        tmp.push_back(buf);
        break;
      case CLASS_WARRIOR:
        buf = "This is horrible... who taught you to fight? That idiot Gireth?";
        tmp.push_back(buf);
        break;
      case CLASS_THIEF:
        buf = "Look who brought a knife to a sword fight? *snicker*";
        tmp.push_back(buf);
        break;
      case CLASS_PALADIN:
        buf = "I hate fighting people in platemail. I get such a headache.";
        tmp.push_back(buf);
        break;
      case CLASS_MAGE:
        buf = "Hey magey, you'd better lose or I'm going to have the gods nerf you again.";
        tmp.push_back(buf);
        break;
      }
    }
    else
    {
      tmp = sword_non_combat;
      if (GET_POS(vict) == POSITION_SLEEPING)
      {
        buf = "Hey... someone steal me already... this guy sucks...";
        tmp.push_back(buf);
        buf = "Ohhh, if I only had some vasoline and arms to move...";
        tmp.push_back(buf);
        buf = "Someone get some markers and write crap on him.";
        tmp.push_back(buf);
        buf = "Quick! Take pictures of him with your bare ass next to his head!";
        tmp.push_back(buf);
        buf = "Oh come on.... after all the ideas from DC parties none of you is gonna fuck with him?";
        tmp.push_back(buf);
      }

      if (vict->in_room == START_ROOM) //tavern
      {
        buf = "Are you going to just sit in the Tavern all day? Great... I'm owned by Avalios.";
        tmp.push_back(buf);
      }
      if (IS_SET(world[vict->in_room].room_flags, SAFE))
      {
        buf = "Oh... I suppose we're just going to sit here and gossip for the next few hours, huh?";
        tmp.push_back(buf);
        buf = "While we're here, why don't we just talk about how badass we are on gossip....";
        tmp.push_back(buf);
      }
      switch (world[vict->in_room].sector_type)
      {
      case SECT_UNDERWATER:
        buf = "Aww man, I hate being under water. I'll rust!";
        tmp.push_back(buf);
        buf = "What the fuck, do I LOOK like a harpoon??";
        tmp.push_back(buf);
        break;
      case SECT_FOREST:
        buf = "If you try to use me like an axe on one of these trees, I am going to shove myself up your ass.";
        tmp.push_back(buf);
        break;
      case SECT_MOUNTAIN:
        buf = "I like mountains. The iron I was made of comes from a mountain. Know what else is made from iron? Trains.";
        tmp.push_back(buf);
        break;
      }
    }

    if (!tmp.empty())
    {
      int rnd = number(0, tmp.size() - 1);
      char buf2[MAX_STRING_LENGTH] = "$B$7Ghaerad, Sword of Legends says, '";
      strcat(buf2, tmp[rnd].c_str());
      strcat(buf2, "'$R\n\r");
      send_to_room(buf2, vict->in_room, true);

      if (rnd == unequip)
      {

        if (vict->equipment[WIELD] && obj_index[vict->equipment[WIELD]->item_number].virt == 27997)
        {

          act("Your $p unequips itself.",
              vict, vict->equipment[WIELD], 0, TO_CHAR, 0);
          act("$n stops using $p.", vict, vict->equipment[WIELD], 0, TO_ROOM, INVIS_NULL);
          obj_to_char(unequip_char(vict, WIELD), vict);
          if (vict->equipment[SECOND_WIELD])
          {
            act("You move your $p to be your primary weapon.", vict, vict->equipment[SECOND_WIELD], 0, TO_CHAR, INVIS_NULL);
            act("$n moves $s $p to be $s primary weapon.", vict, vict->equipment[SECOND_WIELD], 0, TO_ROOM, INVIS_NULL);
            struct obj_data *weapon;
            weapon = unequip_char(vict, SECOND_WIELD);
            equip_char(vict, weapon, WIELD);
          }
        }
        else if (vict->equipment[SECOND_WIELD] && obj_index[vict->equipment[SECOND_WIELD]->item_number].virt == 27997)
        {

          act("Your $p unequips itself.",
              vict, vict->equipment[SECOND_WIELD], 0, TO_CHAR, 0);
          act("$n stops using $p.", vict, vict->equipment[SECOND_WIELD], 0, TO_ROOM, INVIS_NULL);
          obj_to_char(unequip_char(vict, SECOND_WIELD), vict);
        }
      }
    }
    obj->obj_flags.value[0] = number(8, 10);
  }

  return eFAILURE;
}

// Fun item to give to mortals...ticks for a while and then when it blows
// up BOOM!!!
int hot_potato(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, 
                   CHAR_DATA *invoker)
{
   extern int top_of_world;
   int dropped = 0;
   char_data * vict = NULL;

   if(obj->equipped_by)
      vict = obj->equipped_by;
   if(obj->carried_by)
      vict = obj->carried_by;
   if(obj->in_obj && obj->in_obj->carried_by)
      vict = obj->in_obj->carried_by;
   if(obj->in_obj && obj->in_obj->equipped_by)
      vict = obj->in_obj->equipped_by;
   if(!vict)
      return eFAILURE;

   if(cmd == CMD_PUSH) { // push
      if(!strstr(arg, "potatobutton"))
         return eFAILURE;
      if(obj->obj_flags.value[3] > 0) {
         send_to_char("It's already been started!\n\r", vict);
         return eSUCCESS;
      }
      if((vict->in_room >= 0 && vict->in_room <= top_of_world) &&
        IS_SET(world[vict->in_room].room_flags, ARENA) && arena.type == POTATO && ArenaIsOpen()) {
          send_to_char("Wait until the potato arena is open before you try blowing yourself up!\n\r", vict);
          return eSUCCESS;
      }
      send_to_char("The potato starts getting really really hot and burns your hands!!\n\r", vict);
      obj->obj_flags.value[3] = number(1, 100);
      return eSUCCESS;
   }

   if(obj->obj_flags.value[3] < 0) // not active yet:)
      return eFAILURE;

   if(cmd == CMD_SLIP) {
      send_to_char("You can't slip anything when you have a hot potato! (sorry)\n\r", vict);
      return eSUCCESS;
   }
   if(cmd == CMD_DROP) {
      send_to_char("You can't drop anything when you have a hot potato!\n\r", vict);
      return eSUCCESS;
   }
   if(cmd == CMD_DONATE) {
      send_to_char("You can't donate anything when you have a hot potato!\n\r", vict);
      return eSUCCESS;
   }
   if(cmd == CMD_QUIT) {
      send_to_char("You can't quit when you have a hot potato!\n\r", vict);
      return eSUCCESS;
   }
   if(cmd == CMD_SACRIFICE) {
      send_to_char("You can't junk stuff when you have a hot potato!\n\r", vict);
      return eSUCCESS;
   }
   if(cmd == CMD_PUT) {
      send_to_char("You can't 'put' stuff when you have a hot potato!\n\r", vict);
      return eSUCCESS;
   }

   if(cmd == CMD_GIVE) {
      // make sure vict for GIVE/SLIP is a pc
      char obj[MAX_INPUT_LENGTH];
      char target[MAX_INPUT_LENGTH];
      half_chop(arg, obj, target);
      char_data * give_vict;
      if (!(give_vict = get_char_room_vis(ch, target)))
         return eFAILURE; // Not giving to char/mob, so ok
      if(IS_MOB(give_vict)) {
         send_to_char("You can only give things to other players when you have a hot potato!\n\r", vict);
         return eSUCCESS;
      }
      if((vict->in_room >= 0 && vict->in_room <= top_of_world) &&
        IS_SET(world[vict->in_room].room_flags, ARENA) && arena.type == POTATO && ArenaIsOpen() && GET_LEVEL(vict) < IMMORTAL) {
          send_to_char("Wait until the potato arena is open before you start passing out the potatos!\n\r", vict);
          return eSUCCESS;
      }

      // if it's a player, go ahead
      if (number(1, 100) > 90 && GET_LEVEL(vict) < 100) 
        dropped = 1;
      else 
        return eFAILURE;
   }

   if(cmd)
     if (cmd != CMD_GIVE || dropped != 1)
       return eFAILURE;

   if(obj->obj_flags.value[3] > 0 && dropped == 0) 
   {
      obj->obj_flags.value[3]--;
      if(obj->obj_flags.value[3] % 3 == 0)
         send_to_room("You smell a delicious baked potato and hear a faint *beep*.\n\r", vict->in_room, true);
   }
   else {
      if (dropped == 1) {
        send_to_char("OOPS!!! The hot potato burned you and you dropped it!!!\r\n", vict);
        act("$n screams in agony as they are burned by the potato and DROPS it!", vict, 0, 0, TO_ROOM, 0);
      }

      if (!IS_NPC(vict))
        for(descriptor_data * i = descriptor_list; i; i = i->next)
           if(i->character && i->character->in_room != vict->in_room && !i->connected)
              send_to_char("You hear a large BOOM from somewhere in the distance.\n\r", i->character);

       act("The hot potato $n is carrying beeps one final time.\n\r"
           "\n\r$B"
           "BBBB     OOO     OOO    M   M   !!   !!\n\r"
           "B   B   O   O   O   O   MM MM   !!   !!\n\r"
           "BBBB    O   O   O   O   M M M   !!   !!\n\r"
           "B   B   O   O   O   O   M   M \n\r"
           "BBBB     OOO     OOO    M   M   !!   !!\n\r"
           "\n\r$R"
           "Small pieces of $n and mashed potato splatter everywhere!!!\n\r"
           "$n has been KILLED!!"
            , vict, 0, 0, TO_ROOM, 0);

       if(IS_MOB(vict)) {
        act("$n gets back up.", vict, 0, 0, TO_ROOM, 0);
        do_say(vict, "HA!  Fooled ya!", 9);
        extract_obj(obj);
        return eSUCCESS;
       }

       GET_HIT(vict) = -1;
       update_pos(vict);
       send_to_char("$B"
           "BBBB     OOO     OOO    M   M !!   !!\n\r"
           "B   B   O   O   O   O   MM MM !!   !!\n\r"
           "BBBB    O   O   O   O   M M M !!   !!\n\r"
           "B   B   O   O   O   O   M   M \n\r"
           "BBBB     OOO     OOO    M   M !!   !!\n\r\n\r"
           "$R"
           , vict );
       send_to_char("The baked potato you are carrying EXPLODES!!!\n\r"
                    "You have been KILLED!\n\r", vict);
       extract_obj(obj);      
       if(!IS_SET(world[vict->in_room].room_flags, ARENA))
           fight_kill(vict, vict, TYPE_PKILL, KILL_POTATO);
       else 
         if (arena.type == POTATO) 
           fight_kill(vict, vict, TYPE_ARENA_KILL, KILL_MASHED);
         else 
           fight_kill(vict, vict, TYPE_ARENA_KILL, KILL_POTATO);
       return eSUCCESS|eCH_DIED;
   }

   if (dropped == 1)
     return eSUCCESS;
   else
   return eFAILURE;
}


// proc for mortar shells - see object.cpp 
int exploding_mortar_shells(struct char_data*ch, struct obj_data *obj, int cmd, char*arg, CHAR_DATA *invoker)
{
   int dam = 0;
   char buf[MAX_STRING_LENGTH];
   char_data * victim = NULL;
   char_data * next_v = NULL;

   if(cmd)
     return eFAILURE;

   if(obj->in_room <= 0) {
     log("Mortar round without a room?", IMMORTAL, LOG_BUG);
     extract_obj(obj);
     return eFAILURE;
   }


  send_to_room("The mortar shell explodes ripping the area to shreds!\r\n", obj->in_room);

  for(int i = 0; i < 6; i++)
    if( world[obj->in_room].dir_option[i] && world[obj->in_room].dir_option[i]->to_room )
      send_to_room("You hear a loud boom.\r\n", world[obj->in_room].dir_option[i]->to_room);

  for (victim = world[obj->in_room].people ; victim ; victim = next_v ) {
    next_v = victim->next_in_room;
    if(IS_NPC(victim))  // only hurts players
      continue;

    dam = dice( obj->obj_flags.value[1], obj->obj_flags.value[2] );
    GET_HIT(victim) -= dam;
    sprintf(buf, "Pieces of shrapnel rip through your skin inflicting %d damage!\r\n", dam);
    send_to_char(buf, victim);
    if(GET_HIT(victim) < 1) {
      send_to_char("You have been KILLED!!\r\n", victim);
      fight_kill(victim, victim, TYPE_PKILL, KILL_MORTAR);
    }
  }

  extract_obj(obj);
  return eSUCCESS;
}

//565
int godload_banshee(struct char_data *ch, struct obj_data *obj, int cmd, 
      char *arg, CHAR_DATA *invoker)
{
   CHAR_DATA *vict;
   if (cmd) return eFAILURE;
   if (!(vict = ch->fighting)) return eFAILURE;
   if (number(1,101) > 12) return eFAILURE;
   act("$n's instrument takes on a life of its own, sending out a piercing wail.",ch,0,vict, TO_ROOM,0);  
   send_to_char("Your instrument sends out a piercing wail.\r\n",ch);
   return song_whistle_sharp(51,ch,"",vict, 50); 
}
//511
int godload_claws(struct char_data *ch, struct obj_data *obj, int cmd,
   char *arg, CHAR_DATA *invoker)
{
   CHAR_DATA *vict;
   if (cmd) return eFAILURE;
   if (!(vict = ch->fighting)) return eFAILURE;
   if (number(1,101) > 5) return eFAILURE;
   act("$n's claws glow icy blue.",ch,0,vict, TO_ROOM,0);  
   send_to_char("Your claws glow icy blue.\r\n",ch);
   return spell_chill_touch(51,ch,vict,0, 50); 

}

//556
int godload_defender(struct char_data*ch, struct obj_data *obj, int cmd, 
char*arg,
                   CHAR_DATA *invoker)
 {
  char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);
  if (str_cmp(arg1, "arad") || str_cmp(arg2,"tor")) return eFAILURE;
  if (isTimer(ch, SPELL_PROTECT_FROM_EVIL)) 
  {  
     send_to_char("The defender flickers, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_PROTECT_FROM_EVIL, 24);
  return spell_protection_from_evil(50,ch, ch, 0, 150);
}

//500
int godload_stargazer(struct char_data*ch, struct obj_data *obj, int cmd, 
char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
//  arg = one_argument(arg, arg2);
 // arg = one_argument(arg, arg3);
  if (str_cmp(arg1, "cabed")) return eFAILURE;
  if (isTimer(ch, SPELL_MANA)) 
  {  
     send_to_char("The robe glows, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_MANA, 6);
  send_to_char("Your robes glow brightly!\r\n",ch);
  return spell_mana(50, ch, ch, 0, 100);
}

//534
int godload_cassock(struct char_data*ch, struct obj_data *obj, int cmd, 
char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
//  arg = one_argument(arg, arg2);
 // arg = one_argument(arg, arg3);
  if (str_cmp(arg1, "alata")) return eFAILURE;
  if (isTimer(ch, SPELL_GROUP_SANC))
  {  
     send_to_char("The cassock hums, but ends soon after it starts.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_GROUP_SANC, 36);
  send_to_char("Your cassocks begin to hum loudly!\r\n",ch);
  return spell_group_sanc((ubyte)50, ch, ch, 0, 100);
}


//526
int godload_armbands(struct char_data*ch, struct obj_data *obj, int cmd, 
char*arg, CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  if (str_cmp(arg1, "vanesco")) return eFAILURE;
  if (isTimer(ch, SPELL_TELEPORT)) 
  {  
     send_to_char("The armbands crackle, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_TELEPORT, 24);
  send_to_char("Your armbands crackle, and you phase out of existence.\n\r",ch);
  act("$n phases out of existence.",ch, 0, 0, TO_ROOM,0);
  return spell_teleport(50,ch, ch, 0, 100);
}

//548
int godload_gaze(struct char_data*ch, struct obj_data *obj, int cmd, 
char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  if (str_cmp(arg1, "iudicium")) return eFAILURE;
  if (isTimer(ch, SPELL_KNOW_ALIGNMENT)) 
  {  
     send_to_char("The gaze gazes stoically, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_KNOW_ALIGNMENT, 24);
  send_to_char("The gaze reacts to your words, and you feel ready to judge.\r\n",ch);
  return spell_know_alignment(50,ch, ch, 0, 150);
}

//514
int godload_wailka(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);

  if (str_cmp(arg1, "suloaki")) return eFAILURE;
  if (isTimer(ch, SPELL_PARALYZE) || IS_SET(world[ch->in_room].room_flags, SAFE)) 
  {  
     send_to_char("The ring hums, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  CHAR_DATA *vict; 
  if ((vict = get_char_room_vis(ch, arg2)) == NULL)
  {
     send_to_char("You need to tell the item who.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_PARALYZE, 12);
  send_to_char("Your ring radiates evil, and does your bidding.\r\n",ch);
  return spell_paralyze(50, ch, vict, 0, 50);
}

//517
int godload_choker(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "burzum")) return eFAILURE;
  if (isTimer(ch, SPELL_GLOBE_OF_DARKNESS)) 
  {  
     send_to_char("The choker glows black for a moment, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_GLOBE_OF_DARKNESS, 12);
  send_to_char("Your choker glows black, and dampens all light in the room.\r\n",ch);
  return spell_globe_of_darkness(50, ch, ch, 0, 150);
}

//519
int godload_lorne(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "incende")) return eFAILURE;
  if (isTimer(ch, SPELL_CONT_LIGHT)) 
  {  
     send_to_char("The necklace shines, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_CONT_LIGHT, 12);
  send_to_char("The necklace shines brightly.\r\n",ch);
  return spell_cont_light(50, ch, ch, 0, 150);
}

//528
int godload_leprosy(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WEAR_FEET] != obj)      return eFAILURE;

   if(number(0, 1))
      return eFAILURE;

   act("$n's $o release a cloud of disease!",
              ch, obj, ch->fighting, TO_ROOM, 0);
   send_to_char("Your feet releases a cloud of disease!\r\n", ch);

   return spell_harm(GET_LEVEL(ch), ch, ch->fighting, 0, 150);
}


//540
int godload_quiver(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "quant'naith")) return eFAILURE;
  if (isTimer(ch, SPELL_MISANRA_QUIVER)) 
  {  
     send_to_char("The quiver glitters, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_MISANRA_QUIVER, 24);
  struct obj_data *obj2;
  int i;
  send_to_char("The quiver glitters, and hums.\r\n",ch);
  for (i = 0; i < 25; i++)
  {
     if((obj->obj_flags.weight + 1) < obj->obj_flags.value[0]) 
     {
	obj2 = clone_object(real_object(597));
	if (!obj_to_obj(obj2, obj)) {
	  send_to_char("Some arrows appear in your quiver.\r\n",ch);
	  send_to_char("The quiver flickers brightly, and ends unfinished.\r\n",ch);
	  return eSUCCESS;
	}
     } else {
	  send_to_char("Some arrows appear in your quiver.\r\n",ch);
	  send_to_char("The quiver flickers brightly, and ends unfinished.\r\n",ch);
	return eSUCCESS;
     }
  } 
  send_to_char("Some arrows appear in your quiver.\r\n",ch);
  return eSUCCESS;
}

int godload_aligngood(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "aglar")) return eFAILURE;
  if (isTimer(ch, SPELL_ALIGN_GOOD)) 
  {  
     send_to_char("The fire burns, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_ALIGN_GOOD, 48);
  GET_ALIGNMENT(ch) += 400;
  if (GET_ALIGNMENT(ch) > 1000) GET_ALIGNMENT(ch) = 1000;
  send_to_char("You are purified by the light of the fire.\r\n",ch);
  return eSUCCESS;  
}

int godload_alignevil(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY|| !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "dagnir")) return eFAILURE;
  if (isTimer(ch, SPELL_ALIGN_EVIL)) 
  {  
     send_to_char("The blackened heart croaks, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_ALIGN_EVIL, 48);
  GET_ALIGNMENT(ch) -= 400;
  if (GET_ALIGNMENT(ch) < -1000) GET_ALIGNMENT(ch) = -1000;
  send_to_char("You are burnt by the heart's darkness.\r\n",ch);
  return eSUCCESS;
}


int godload_tovmier(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_PULL||!is_wearing(ch,obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "staff") && str_cmp(arg1, "tovmier")) return eFAILURE;

  send_to_char("You twist the handle of the staff.\r\n",ch);
  for(int i = 0; i < obj->num_affects; i++)
    if (obj->affected[i].location == WEP_DISPEL_EVIL)
    {
       obj->affected[i].location = WEP_DISPEL_GOOD;
       return eSUCCESS;
    } else if (obj->affected[i].location == WEP_DISPEL_GOOD)
    {
       obj->affected[i].location = WEP_DISPEL_EVIL;
       return eSUCCESS;
    }
  send_to_char("Something's bugged with this staff. Report it.\r\n",ch);
  return eSUCCESS;
}

int godload_hammer(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  if (cmd != CMD_TREMOR || !ch)
    return eFAILURE;

  if (!is_wearing(ch, obj)) return eFAILURE;
  if (isTimer(ch, SPELL_EARTHQUAKE)) 
  {  
     send_to_char("The hammer glows, but nothing happens.\r\n",ch);
     return eSUCCESS;
  }
  act("$n smashes $s hammer into the ground causing a tectonic blast.", ch, 0, 0, TO_ROOM, 0);
  send_to_char("You smash your hammer into the ground, causing it to shake violently.\r\n",ch);
  addTimer(ch, SPELL_EARTHQUAKE, 24);
  int retval = spell_earthquake(50, ch, ch, 0, 100);
  if (!SOMEONE_DIED(retval)) 
      retval |= spell_earthquake(50, ch, ch, 0, 100);
  return retval;
}

int angie_proc(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
  if (cmd != CMD_OPEN || ch->in_room != 29263)
    return eFAILURE;
  char arg1[MAX_INPUT_LENGTH];
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "door"))
    return eFAILURE;
  if (!IS_SET(world[ch->in_room].dir_option[0]->exit_info, EX_CLOSED))
    return eFAILURE;
  REMOVE_BIT(world[ch->in_room].dir_option[0]->exit_info, EX_CLOSED);
  REMOVE_BIT(world[29265].dir_option[2]->exit_info, EX_CLOSED);
  act("$n turns the doorknob, there is a loud click, and a blinding explosion knocks you on your ass.", ch, NULL, NULL, TO_ROOM, 0);
  act("You turn the doorknob, there is a loud click, and a blinding explosion knocks you on your ass.", ch, NULL, NULL, TO_CHAR, 0);
  CHAR_DATA *a, *b, *c;
  b = initiate_oproc(NULL, obj);
  for (a = world[ch->in_room].people; a; a = c)
  {
    c = a->next_in_room; // 'cause mobs get freed
    if (a == b)
      continue;
    damage(b, a, 1000, TYPE_HIT, 50000, 0);
  }

  end_oproc(b);
  extract_obj(obj);
  return eSUCCESS;
}

int godload_phyraz(struct char_data*ch, struct obj_data *obj, int cmd, char*arg,
                   CHAR_DATA *invoker)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch,obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);

  if (str_cmp(arg1, "katascopse")) return eFAILURE;
  if (isTimer(ch, SPELL_WIZARD_EYE)) 
  {  
     send_to_char("The ball stays murky.\r\n",ch);
     return eSUCCESS;
  }
  addTimer(ch, SPELL_WIZARD_EYE, 24);
  CHAR_DATA *vict = get_char_vis(ch, arg2);
  if (!vict)
  {
    send_to_char("The scrying ball stays murky.\r\n",ch);
   return eSUCCESS;
  }
  send_to_char("You tell the scrying ball your bidding, and an image appears.\r\n",ch);
  return spell_wizard_eye(100, ch, vict, 0, 100);
}

void destroy_spellcraft_glyphs(struct char_data *ch)
{
   struct obj_data * tmp_obj, * loop_obj;

   for(tmp_obj = ch->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if(GET_ITEM_TYPE(tmp_obj) == ITEM_CONTAINER)
         for(loop_obj = tmp_obj->contains; loop_obj; loop_obj = loop_obj->next_content)
            if(obj_index[tmp_obj->item_number].virt == 6351 || obj_index[tmp_obj->item_number].virt == 6352 || obj_index[tmp_obj->item_number].virt == 6353)
               move_obj(loop_obj, ch);
      if(obj_index[tmp_obj->item_number].virt == 6351 || obj_index[tmp_obj->item_number].virt == 6352 || obj_index[tmp_obj->item_number].virt == 6353)
         obj_from_char(tmp_obj);
   }
   ch->spellcraftglyph = 0;
}

int spellcraft_glyphs(struct char_data*ch, struct obj_data *obj, int cmd, char*argi, CHAR_DATA *invoker)
{
   char target[MAX_STRING_LENGTH],arg[MAX_STRING_LENGTH];
   struct obj_data * sunglyph, *  bookglyph, * heartglyph;

   if(cmd != CMD_PUT) return eFAILURE; //put
   
   argi = one_argument(argi, arg);
   argi = one_argument(argi, target);

   sunglyph = get_obj_in_list_vis(ch, 6351, ch->carrying);
   bookglyph = get_obj_in_list_vis(ch, 6352, ch->carrying);
   heartglyph = get_obj_in_list_vis(ch, 6353, ch->carrying);

   if(!str_cmp(arg, "power")) {
     if(ch->in_room != 14060) {
        send_to_char("There's no place around to put this special item.\n\r", ch);
        return eFAILURE;
     }

      if(sunglyph == NULL) {
         send_to_char("Put what where?\n\r", ch);
         return eFAILURE;
      }
      if(!str_cmp(target, "sun")) {
         send_to_room("The sun glows brightly as it releases the energy inside the glyph.\n\r", ch->in_room);
         obj_from_char(sunglyph);
	 SET_BIT(ch->spellcraftglyph, 1);
      } else if(!str_cmp(target, "book")) {
         send_to_room("The sun glows a bright red and shatters the glyphs!\n\r", ch->in_room);
         destroy_spellcraft_glyphs(ch);
      } else if(!str_cmp(target, "heart")) {
         send_to_room("The sun glows a bright red and shatters the glyphs!\n\r", ch->in_room);
         destroy_spellcraft_glyphs(ch);
      } else {
         send_to_char("Put it where?\n\r", ch);
         return eFAILURE;
     }
   } else if(!str_cmp(arg, "wisdom")) {
      if(ch->in_room != 14060) {
        send_to_char("There's no place around to put this special item.\n\r", ch);
        return eFAILURE;
     }
      if(bookglyph == NULL) {
         send_to_char("Put what where?\n\r", ch);
         return eFAILURE;
      }
      if(!str_cmp(target, "sun")) {
         send_to_room("The book slams shut creating a sonic wave that shatters the glyphs!\n\r", ch->in_room);
         destroy_spellcraft_glyphs(ch);
      } else if(!str_cmp(target, "book")) {
         send_to_room("The book closes over the glyph, becoming slightly warm.\n\r", ch->in_room);
         obj_from_char(bookglyph);
	 SET_BIT(ch->spellcraftglyph, 2);
      } else if(!str_cmp(target, "heart")) {
         send_to_room("The book slams shut creating a sonic wave that shatters the glyphs!\n\r", ch->in_room);
         destroy_spellcraft_glyphs(ch);
      } else {
         send_to_char("Put it where?\n\r", ch);
         return eFAILURE;
     }
   } else if(!str_cmp(arg, "will")) {
     if(ch->in_room != 14060) {
        send_to_char("There's no place around to put this special item.\n\r", ch);
        return eFAILURE;
     }

      if(heartglyph == NULL) {
         send_to_char("Put what where?\n\r", ch);
         return eFAILURE;
      }
      if(!str_cmp(target, "sun")) {
         send_to_room("The heart beats seemingly uncontrollibly and shatters the glyphs!\n\r", ch->in_room);
         destroy_spellcraft_glyphs(ch);
      } else if(!str_cmp(target, "book")) {
         send_to_room("The heart beats seemingly uncontrollibly and shatters the glyphs!\n\r", ch->in_room);
         destroy_spellcraft_glyphs(ch);
      } else if(!str_cmp(target, "heart")) {
         send_to_room("You place the glyph next to the heart, and it slowly begins to pulse.\n\r", ch->in_room);
         obj_from_char(heartglyph);
	 SET_BIT(ch->spellcraftglyph, 4);
      } else {
         send_to_char("Put it where?\n\r", ch);
         return eFAILURE;
     }
   } else return eFAILURE;
//      send_to_char("Which glyph?\n\r", ch);
   if(ch->spellcraftglyph == 7) {
      if(GET_CLASS(ch) == CLASS_MAGIC_USER && GET_LEVEL(ch) >= 50 && !has_skill(ch, SKILL_SPELLCRAFT)) {
         send_to_room("The glyph receptacles glow an eerie pale white.\n\rThe book shoots out a beams of light from the pages.\n\r", ch->in_room);
         send_to_char("A beam of light hits you in the head!\n\rYou have learned spellcraft!\n\r", ch);
         learn_skill(ch, SKILL_SPELLCRAFT, 1, 1);
      }
   }
/*   if(ch->spellcraftglyph > 8) {
      send_to_room("The glyph containers ", ch->in_room);
      destroy_spellcraft_glyphs(ch); //just in case
      return eFAILURE;
   }
*/
   return eSUCCESS;   
}

int godload_grathelok(CHAR_DATA *ch, struct obj_data *obj,  int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   CHAR_DATA *vict; 

   if(!(vict = ch->fighting))
       return eFAILURE;

   if(GET_HIT(vict) > 100)
     return eFAILURE;

   switch(number(0, 1)) {
     case 0:
       act("$n's blow rips your leg from your body.  Extreme pain is yours to know until you hit the ground mercifully dead.", ch, 0, vict, TO_VICT, 0);
       act("You attack takes off $N's leg at the knee!  Ahh the blood!", ch, 0, vict, TO_CHAR, 0);
       act("$n fierce swing takes off $N's leg at the knee leaving a bloody stump and a brief scream.", ch, 0, vict, TO_ROOM, NOTVICT);
       make_leg(vict);
       break;
     case 1:
       act("$n's attack takes off your arm at the shoulder.  You stare in shock at the fountaining blood before $e crushes your skull.", ch, 0, vict, TO_VICT, 0);
       act("You violently rip off $S arm with the attack before caving in $N's forehead.", ch, 0, vict, TO_CHAR, 0);
       act("With a grunt of exertion, $n swings with enough force to rip $N's arm off!", ch, 0, vict, TO_ROOM, NOTVICT);
       make_arm(vict);
       break;
   }
   GET_HIT(vict) = -20;
   group_gain(ch, vict); 
   fight_kill(ch, vict, TYPE_CHOOSE, 0);
   return eSUCCESS|eVICT_DIED; 
} 
int goldenbatleth(CHAR_DATA *ch, struct obj_data *obj,  int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   CHAR_DATA *vict; 

   if(!(vict = ch->fighting))
       return eFAILURE;

   if(GET_HIT(vict) > 40)
     return eFAILURE;

   switch(number(0, 1)) {
     case 0:
       act("$n's blow rips your leg from your body.  Extreme pain is yours to know until you hit the ground mercifully dead.", ch, 0, vict, TO_VICT, 0);
       act("You attack takes off $N's leg at the knee!  Ahh the blood!", ch, 0, vict, TO_CHAR, 0);
       act("$n fierce swing takes off $N's leg at the knee leaving a bloody stump and a brief scream.", ch, 0, vict, TO_ROOM, NOTVICT);
       make_leg(vict);
       break;
     case 1:
       act("$n's attack takes off your arm at the shoulder.  You stare in shock at the fountaining blood before $e crushes your skull.", ch, 0, vict, TO_VICT, 0);
       act("You violently rip off $S arm with the attack before caving in $N's forehead.", ch, 0, vict, TO_CHAR, 0);
       act("With a grunt of exertion, $n swings with enough force to rip $N's arm off!", ch, 0, vict, TO_ROOM, NOTVICT);
       make_arm(vict);
       break;
   }
   GET_HIT(vict) = -20;
   group_gain(ch, vict); 
   fight_kill(ch, vict, TYPE_CHOOSE, 0);
   return eSUCCESS|eVICT_DIED; 
} 

int godload_jaelgreth(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)   return eFAILURE;

   if(number(1, 100) > 5)
      return eFAILURE;

   send_to_char("You thrust your sacrificial blade into your victim, leeching their lifeforce!\r\n", ch);
   act("$n's dagger sinks into your flesh, and you feel your life force being drained!", 
              ch, obj, ch->fighting, TO_VICT, 0);
   CHAR_DATA *victim = ch->fighting;

   int dam = 100;

   if(affected_by_spell(victim, SPELL_DIVINE_INTER) 
      && dam > affected_by_spell(victim, SPELL_DIVINE_INTER)->modifier)
      dam = affected_by_spell(victim, SPELL_DIVINE_INTER)->modifier;

   GET_HIT(victim) -= dam;
   GET_HIT(ch) += dam;

   if (GET_HIT(ch) > GET_MAX_HIT(ch)) 
     GET_HIT(ch) = GET_MAX_HIT(ch);

   dam = MIN(100, GET_MANA(victim));
   if(dam < 0) 
     dam = 0;

   GET_MANA(victim) -= dam;
   GET_MANA(ch) += dam;

   if (GET_MANA(ch) > GET_MAX_MANA(ch)) 
     GET_MANA(ch) = GET_MAX_MANA(ch);

   update_pos(victim);

  if (GET_POS(victim) == POSITION_DEAD) {
      act("$n is DEAD!!", victim, 0, 0, TO_ROOM, INVIS_NULL);
      group_gain(ch, victim);
      if(!IS_NPC(victim))
         send_to_char("You have been KILLED!!\n\r\n\r", victim);
      fight_kill(ch, victim, TYPE_CHOOSE, 0);
      return eSUCCESS|eVICT_DIED;
  }
  return eSUCCESS;

}


int godload_foecrusher(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)   return eFAILURE;

   if(number(1, 100) > 5)
      return eFAILURE;


   int dam = 80;
   switch (number(1,2))
   {
     case 1:
      act("$n smashes $s hammer into your face!", 
              ch, obj, ch->fighting, TO_VICT, 0);
      act("You smash your hammer into $N's face!", 
              ch, obj, ch->fighting, TO_CHAR, 0);
	dam = 80;
	break;
     case 2:
	dam= 130;
      act("The force of the blow dealt by $n's Foecrusher inflicts heavy damage on you.", 
              ch, obj, ch->fighting, TO_VICT, 0);
      act("The force of your hammer's blow inflicts heavy damage on $N.", 
              ch, obj, ch->fighting, TO_CHAR, 0);
	break;
   }
   CHAR_DATA *victim = ch->fighting;
   dam = number(50,dam);

   if(affected_by_spell(victim, SPELL_DIVINE_INTER) && dam > affected_by_spell(victim, SPELL_DIVINE_INTER)->modifier)
      dam = affected_by_spell(victim, SPELL_DIVINE_INTER)->modifier;
   GET_HIT(victim) -= dam;

   update_pos(victim);

  if (GET_POS(victim) == POSITION_DEAD) {
      act("$n is DEAD!!", victim, 0, 0, TO_ROOM, INVIS_NULL);
      group_gain(ch, victim);
      if(!IS_NPC(victim))
         send_to_char("You have been KILLED!!\n\r\n\r", victim);
      fight_kill(ch, victim, TYPE_CHOOSE, 0);
      return eSUCCESS|eVICT_DIED;
  }
  return eSUCCESS;

}


int godload_hydratail(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, 
                   CHAR_DATA *invoker)
{
   if(cmd)                                  return eFAILURE;
   if(!ch || !ch->fighting)                 return eFAILURE;
   if(ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)   return eFAILURE;

   if(number(1, 100) > 10)
      return eFAILURE;

   int damtype = 0;
   char dammsg[MAX_STRING_LENGTH];
   int dam = number(50,100);
   string damtypeStr;
   sprintf(dammsg, "$B%d$R", dam);

   switch (number(1,4))
   {
     case 1:
       damtypeStr = "$4$Bfiery$R";
       damtype = TYPE_FIRE;
       break;
     case 2:
       damtypeStr = "$3$Bchilling$R";
       damtype = TYPE_COLD;
       break;
     case 3:
       damtypeStr = "$2$Bacidic$R";
       damtype = TYPE_ACID;
       break;
     case 4:
       damtypeStr = "$5$Bshocking$R"; 
       damtype = TYPE_ENERGY;
       break;
   }
   CHAR_DATA *victim = ch->fighting;

   stringstream strwithdam;
   stringstream strwithoutdam;

   strwithdam << "A head on $n's whip lashes out of its own accord unleashing a " << damtypeStr << " blast at you for | damage!";
   strwithoutdam << "A head on $n's whip lashes out of its own accord unleashing a " << damtypeStr << " blast at you!";
   send_damage(strwithdam.str().c_str(), ch, obj, ch->fighting, dammsg, strwithoutdam.str().c_str(), TO_VICT);

   strwithdam.str("");
   strwithoutdam.str("");

   strwithdam << "A head on your whip lashes out of its own accord unleashing a " << damtypeStr << " blast at $N for | damage!";
   strwithoutdam << "A head on your whip lashes out of its own accord unleashing a " << damtypeStr << " blast at $N!";
   send_damage(strwithdam.str().c_str(), ch, obj, ch->fighting, dammsg, strwithoutdam.str().c_str(), TO_CHAR);

   return damage(ch, victim, dam, damtype, 0, 0);
}
