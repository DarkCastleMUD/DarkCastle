/*	Fight1.c written from the original by Morcallen 1/17/95
*	This contains all the fight starting mechanisms as well
*	as damage.
*/ 
/* $Id: fight.cpp,v 1.13 2002/07/30 21:35:22 pirahna Exp $ */

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* malloc */

#ifndef WIN32
#include <unistd.h>
#endif
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <fight.h>
#include <levels.h>
#include <race.h>
#include <player.h> // log
#include <utility.h> // log
#include <character.h>
#include <spells.h> // weapon_spells
#include <isr.h>
#include <mobile.h>
#include <room.h>
#include <handler.h>
#include <interp.h> // do_flee()
#include <fileinfo.h> // SAVE_DIR
#include <db.h> // fread_string()
#include <connect.h> // descriptor_data
#include <magic.h> // weapon spells
#include <terminal.h> // YELLOW, etc..
#include <act.h>
#include <clan.h>
#include <returnvals.h>
#include <assert.h>

extern int top_of_world;
extern CHAR_DATA *character_list;
extern struct descriptor_data *descriptor_list;
extern CWorld world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;

struct clan_data * get_clan(struct char_data *);

/* functions that nobody else should be calling */
int is_stunned(CHAR_DATA *ch);
int do_lightning_shield(CHAR_DATA *ch, CHAR_DATA *vict, int dam);
bool do_frostshield(CHAR_DATA *ch, CHAR_DATA *vict);
int do_fireshield(CHAR_DATA *ch, CHAR_DATA *vict, int dam);
void do_dam_msgs(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int attacktype, int weapon);
void inform_victim(CHAR_DATA *ch, CHAR_DATA *vict, int dam);
void update_stuns(CHAR_DATA *ch);
int is_fighting_mob(CHAR_DATA *ch);
int fighter(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int active_magic_user(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int active_necro(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int paladin(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int antipaladin(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int thief(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int barbarian(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int monk(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int bard_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int ranger_combat(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
int active_cleric(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *invoker);
void remove_memory(CHAR_DATA *ch, char type, CHAR_DATA *vict);
void clan_death (char_data *ch, char_data *victim);

void update_flags(CHAR_DATA *vict);
 
CHAR_DATA *combat_list = NULL, *combat_next_dude = NULL;

void perform_violence(void)
{
  CHAR_DATA *ch;
  int is_mob = 0;
  int retval;
  static struct affected_type *af, *next_af_dude;
  extern char *spell_wear_off_msg[];
  
  if(!combat_list)                  return;
  
  for(ch = combat_list; ch; ch = combat_next_dude) {
    // if combat_next_dude gets killed, it get's updated in "stop_fighting"
    // pretty kludgy way to do it, but it work
    combat_next_dude = ch->next_fighting;
    
    affect_total(ch);
    if(!ch->fighting) { 
      log("Error in perform_violence()!  Null ch->fighting!", IMMORTAL, LOG_BUG);
      return;
    }
    
    /* remove a tick of para lag every round of combat yer in */
    if(IS_AFFECTED(ch, AFF_PARALYSIS))
      for (af = ch->affected; af; af = next_af_dude) {
        next_af_dude = af->next;
        if (af->type == SPELL_PARALYZE)
          if (af->duration >= 1)
            af->duration--;
          if(af->duration == 0)
            if (*spell_wear_off_msg[af->type]) { 
              send_to_char(spell_wear_off_msg[af->type], ch);
              send_to_char("\n\r", ch);
              affect_remove(ch, af);
            }  
      }
      
    if(can_attack(ch)) {
      is_mob = IS_MOB(ch);
      if(is_mob) {
        if((mob_index[ch->mobdata->nr].combat_func)) {
          retval = ((*mob_index[ch->mobdata->nr].combat_func)(ch, NULL, 0, "", ch));
          if(SOMEONE_DIED(retval))
            continue;
        }
        else {
          retval = 0;
          switch(GET_CLASS(ch)) {
           case CLASS_WARRIOR:       retval = fighter(ch, NULL, 0, "", ch);           break;
           case CLASS_THIEF:         retval = thief(ch, NULL, 0, "", ch);             break;
           case CLASS_MONK:          retval = monk(ch, NULL, 0, "", ch);              break;
           case CLASS_BARD:          retval = bard_combat(ch, NULL, 0, "", ch);       break;
           case CLASS_RANGER:        retval = ranger_combat(ch, NULL, 0, "", ch);     break;
           case CLASS_MAGIC_USER:    retval = active_magic_user(ch, NULL, 0, "", ch); break;
           case CLASS_CLERIC:        retval = active_cleric(ch, NULL, 0, "", ch);     break;
           case CLASS_PALADIN:       retval = paladin(ch, NULL, 0, "", ch);           break;
           case CLASS_ANTI_PAL:      retval = antipaladin(ch, NULL, 0, "", ch);       break;
           case CLASS_BARBARIAN:     retval = barbarian(ch, NULL, 0, "", ch);         break;
           case CLASS_NECROMANCER:   retval = active_necro(ch, NULL, 0, "", ch);      break;
           default:                  break;
          }
          if(SOMEONE_DIED(retval))
            continue;
        }
      } // if is_mob

      retval = attack(ch, ch->fighting, TYPE_UNDEFINED);

      if(is_mob && IS_SET(retval, eCH_DIED)) // no point in going anymore
        continue;
      else update_flags(ch);

      if(ch->equipment[WIELD]) {
        if(obj_index[ch->equipment[WIELD]->item_number].combat_func) {
          retval = ((*obj_index[ch->equipment[WIELD]->item_number].combat_func)
                     (ch, ch->equipment[WIELD], 0, "", ch));
          if(SOMEONE_DIED(retval))
            continue;
        }
        if(ch->equipment[SECOND_WIELD]) {
          if(obj_index[ch->equipment[SECOND_WIELD]->item_number].combat_func) {
            retval = ((*obj_index[ch->equipment[SECOND_WIELD]->item_number].combat_func)
                     (ch, ch->equipment[SECOND_WIELD], 0, "", ch));
            if(SOMEONE_DIED(retval))
              continue;
          }
        }
      }
      // MOB Progs
      retval = mprog_hitprcnt_trigger( ch, ch->fighting );
      if(IS_SET(retval, eCH_DIED))
        continue;
      retval = mprog_fight_trigger( ch, ch->fighting );
      if(IS_SET(retval, eCH_DIED))
        continue;

    } // can_attack
    else if(is_stunned(ch))
      update_stuns(ch);
    else
      stop_fighting(ch);
      
    if(IS_NPC(ch))
      if(MOB_WAIT_STATE(ch))
        MOB_WAIT_STATE(ch) -= 1;
        
    // This takes care of flee and stuff
    if(ch && ch->fighting)
      if(ch->in_room != (ch->fighting)->in_room)
        stop_fighting(ch);
  } // for
}

bool gets_dual_wield_attack(char_data * ch)
{
  byte learned = has_skill(ch, SKILL_DUAL_WIELD);
  byte percent = number(1, 101);

  if(percent > learned)
    return FALSE;
  return TRUE;
}

// int attack(...) FUNCTION SHOULD BE CALLED *INSTEAD* OF HIT IN ALL CASES!
// standard retvals
int attack(CHAR_DATA *ch, CHAR_DATA *vict, int type, int weapon)
{
  int do_say(struct char_data *ch, char *argument, int cmd);
  int result = 0;  
  obj_data * wielded = 0;
  int handle_any_guard(char_data * ch);

  if (!ch || !vict) { 
    log("NULL Victim or Ch sent to attack!  This crashes us!", -1, LOG_BUG);
    return eINTERNAL_ERROR;
  }

  assert(GET_POS(ch) != POSITION_DEAD);
  assert(GET_POS(vict) != POSITION_DEAD);

  if (!can_attack(ch))                          return eFAILURE;
  if (!can_be_attacked(ch, vict))               return eFAILURE;

  // TODO - until I can make sure that area effects don't attack other mobs
  // when cast by mobs, I need to make sure mobs aren't killing each other  
  if (IS_NPC(ch) && IS_NPC(vict) && 
      !IS_AFFECTED(ch, AFF_CHARM) &&   !IS_AFFECTED(ch, AFF_FAMILIAR) &&
      !IS_AFFECTED(vict, AFF_CHARM) && !IS_AFFECTED(vict, AFF_FAMILIAR)) 
  {
    remove_memory(ch, 'h');
    remove_memory(ch, 't');
    remove_memory(vict, 'h');
    remove_memory(vict, 't');
    stop_fighting(ch);
    stop_fighting(vict);
    do_say(ch, "I'm sorry my fellow mob.  I have seen the error of my ways.", 0);
    do_say(vict, "It is okay brethren.  Let's go have a beer.", 0);
    return eFAILURE;
  }
  
  set_cantquit(ch, vict);   // This sets the flag if necessary
  set_fighting(ch, vict);
  
  wielded = ch->equipment[WIELD];

  if(type != SKILL_BACKSTAB)
    if(handle_any_guard(vict))
      vict = ch->fighting;

  assert(vict);

  /* if it's backstab send it to one_hit so it can be handled */
  if(type == SKILL_BACKSTAB)  {
    return one_hit(ch, vict, SKILL_BACKSTAB, weapon);
  }
  else if(GET_CLASS(ch) == CLASS_MONK && wielded == FALSE)
  {
    if(GET_LEVEL(ch) > MORTAL) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }

    if(GET_LEVEL(ch) > 47)
      if(number(0, 1)) {
        result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
      }

    if(GET_LEVEL(ch) > 39) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }
    else if(GET_LEVEL(ch) > 29)
      if(number(0, 1)) {
        result = one_hit(ch, vict, type, FIRST);
        if(SOMEONE_DIED(result))       return result;
      }

    if(GET_LEVEL(ch) > 19) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }

    if(IS_AFFECTED(ch, AFF_HASTE)) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }

    // lose an attack if using a shield
    if(GET_LEVEL(ch) > 9 && !ch->equipment[WEAR_SHIELD]) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }

    result = one_hit(ch, vict, type, FIRST);
    if(SOMEONE_DIED(result))       return result;

  } // End of the monk attacks
  else // It's a normal attack
  {
    result = one_hit(ch, vict, type, FIRST); // Everyone gets one hit 
    if(SOMEONE_DIED(result))       return result;

    // This is here so we only show this after the PC's first
    // attack rather than after every hit.
    if(GET_POS(vict) == POSITION_STUNNED)
      act("$n is $B$0stunned$R, but will probably recover.", vict, 0, 0, TO_ROOM, INVIS_NULL);
    
    if(second_attack(ch)) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }
    if(third_attack(ch)) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }
    if(fourth_attack(ch)) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }
    if(IS_AFFECTED(ch, AFF_HASTE)) {
      result = one_hit(ch, vict, type, FIRST);
      if(SOMEONE_DIED(result))       return result;
    }
    if(second_wield(ch) && gets_dual_wield_attack(ch)) {
      result = one_hit(ch, vict, type, SECOND);
      if(SOMEONE_DIED(result))       return result;
    }
    
    // Now we handle monk attacks
  }	

  return eSUCCESS;
} // of attack

void update_flags(CHAR_DATA *vict)
{
  if (IS_SET(vict->combat, COMBAT_BASH1)) {
    REMOVE_BIT(vict->combat, COMBAT_BASH1);
    SET_BIT(vict->combat, COMBAT_BASH2);
  }
  else if(IS_SET(vict->combat, COMBAT_BASH2))
    REMOVE_BIT(vict->combat, COMBAT_BASH2);
  
  if (IS_SET(vict->combat, COMBAT_RAGE1)) {
    REMOVE_BIT(vict->combat, COMBAT_RAGE1);
    SET_BIT(vict->combat, COMBAT_RAGE2);
  }
  else if(IS_SET(vict->combat, COMBAT_RAGE2)) {
    REMOVE_BIT(vict->combat, COMBAT_RAGE2);
    act("$n calms down a bit.", vict, 0, 0, TO_ROOM, 0);
    act("Your mind seems a bit clearer now.", vict, 0, 0, TO_CHAR, 0);
  }

  if (IS_SET(vict->combat, COMBAT_BLADESHIELD1)) {
    REMOVE_BIT(vict->combat, COMBAT_BLADESHIELD1);
    SET_BIT(vict->combat, COMBAT_BLADESHIELD2);
  }
  else if(IS_SET(vict->combat, COMBAT_BLADESHIELD2))
    REMOVE_BIT(vict->combat, COMBAT_BLADESHIELD2);    

  if (IS_SET(vict->combat, COMBAT_VITAL_STRIKE))
    REMOVE_BIT(vict->combat, COMBAT_VITAL_STRIKE);

  if(IS_SET(vict->combat, COMBAT_MONK_STANCE)) {
     // stance lasts 'modifier' rounds.  Remove bit once used up
     struct affected_type * pspell;
     pspell = affected_by_spell(vict, KI_STANCE+KI_OFFSET);
     pspell->modifier--;
     if(pspell->modifier < 0)
     {
        REMOVE_BIT(vict->combat, COMBAT_MONK_STANCE);
        send_to_char("Your stance ends.  You can absorb no more.\r\n", vict);
     }
     else send_to_char("Your stance weakens...\r\n", vict);
  }
}

void update_stuns(CHAR_DATA *ch)
{
  if (IS_SET(ch->combat, COMBAT_SHOCKED))
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED);
  
  if (IS_SET(ch->combat, COMBAT_STUNNED)) {
    REMOVE_BIT(ch->combat, COMBAT_STUNNED);
    SET_BIT(ch->combat, COMBAT_STUNNED2);
  }
  else if(IS_SET(ch->combat, COMBAT_STUNNED2)) {
    REMOVE_BIT(ch->combat, COMBAT_STUNNED2);
    if (GET_HIT(ch) > 0)
      if (GET_POS(ch) != POSITION_FIGHTING) {
        act("$n regains conciousness...", ch, 0, 0, TO_ROOM, 0);
        act("You regain conciousness...", ch, 0, 0, TO_CHAR, 0);
        GET_POS(ch) = POSITION_SITTING;
      }
  }
}

bool do_frostshield(CHAR_DATA *ch, CHAR_DATA *vict) {
  if(!ch || !vict) {
    log("Null ch or vict sent to do_frostshield", IMP, LOG_BUG);
    return(false);
  }
  if(!IS_AFFECTED(vict, AFF_FROSTSHIELD)) {
    return(false);
  }
  if(number(0, 100) < 5) {
    act("Bits of $Bfrost$R fly as $n's blow bounces off your $B$1icy$R shield.", ch, 0, vict, TO_VICT, 0);
    act("Bit of $Bfrost$R fly as $n's blow bounces off $N's $B$1icy$R shield.", ch, 0, vict, TO_ROOM, NOTVICT);
    act("Bits of $Bfrost$R fly as your blow bounces off $N's $B$1icy$R shield.", ch, 0, vict, TO_CHAR, 0);
    return(true);
  } else {
    return(false);
  }
}  

int do_lightning_shield(CHAR_DATA *ch, CHAR_DATA *vict, int dam) {
  extern struct race_shit race_info[];
  
  if(!ch || !vict) {
    log("Null ch or vict sent to do_lightning_shield", IMP, LOG_BUG);
    return eFAILURE|eINTERNAL_ERROR;
  }
  
  if(GET_POS(vict) == POSITION_DEAD)            return eFAILURE;
  if(GET_LEVEL(ch) >= IMMORTAL)                 return eFAILURE;
  if(!IS_AFFECTED(vict, AFF_LIGHTNINGSHIELD))   return eFAILURE;

  if (IS_SET(race_info[(int)GET_RACE(ch)].immune, ISR_ENERGY)) {
    dam = 0;
  } else {
    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
  }
      
  dam /= 5;
  GET_HIT(ch) -= dam;
  if(dam > 0) {
    act("Sparks from $N's $B$5lightning$R shield sting you.", ch, 0, vict, TO_CHAR, 0);
    act("Sparks from your $B$5lightning$R shield sting $n.", ch, 0, vict, TO_VICT, 0);
    act("Sparks from $N's $B$5lightning$R shield sting $n.", ch, 0, vict, TO_ROOM, NOTVICT);
  } else {
    act("You ignore $N's pathetic sparks.", ch, 0, vict, TO_CHAR, 0);
    act("$n ignores $N's pathetic sparks.", ch, 0, vict, TO_ROOM, NOTVICT);
    act("$n ignores your pathetic sparks.", ch, 0, vict, TO_VICT, 0);
  }
  update_pos(ch);
    
  if (GET_POS(ch) == POSITION_DEAD) {
      group_gain(vict, ch);
      fight_kill(vict, ch, TYPE_CHOOSE);
      return eSUCCESS|eCH_DIED;
  }

  return eSUCCESS;
}

// standard retvals
int do_fireshield(CHAR_DATA *ch, CHAR_DATA *vict, int dam)
{
  // ch is the person who just hit the victim
  // so ch takes the damage from this spell 
  extern struct race_shit race_info[];
  
  if (!ch || !vict || ch == vict) {
    log("Null ch or vict, or ch==vict sent to do_fireshield!", IMP, LOG_BUG);
    abort();
  }
  
  if(GET_POS(vict) == POSITION_DEAD)            return eFAILURE;
  if(!IS_NPC(ch) && GET_LEVEL(ch) >= IMMORTAL)  return eFAILURE;
  if(!IS_AFFECTED(vict, AFF_FIRESHIELD))        return eFAILURE;

  if (IS_SET(race_info[(int)GET_RACE(ch)].immune, ISR_FIRE))
    dam = 0;
  else {
    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
      
    dam = 50;
  }
    
  GET_HIT(ch) -= dam;
  do_dam_msgs(vict, ch, dam, SPELL_FIRESHIELD, WIELD);                         
  update_pos(ch);
    
  if (GET_POS(ch) == POSITION_DEAD) {
      group_gain(vict, ch);
      fight_kill(vict, ch, TYPE_CHOOSE);
      return eSUCCESS|eCH_DIED;
  }

  return eSUCCESS;
}

// standard retvals
int do_acidshield(CHAR_DATA *ch, CHAR_DATA *vict, int dam)
{
  // ch is the person who just hit the victim
  // so ch takes the damage from this spell 
  extern struct race_shit race_info[];
  
  if (!ch || !vict || ch == vict) {
    log("Null ch or vict, or ch==vict sent to do_fireshield!", IMP, LOG_BUG);
    abort();
  }
  
  if(GET_POS(vict) == POSITION_DEAD)            return eFAILURE;
  if(!IS_NPC(ch) && GET_LEVEL(ch) >= IMMORTAL)  return eFAILURE;
  if(!affected_by_spell(vict, SPELL_ACID_SHIELD)) return eFAILURE;

  if (IS_SET(race_info[(int)GET_RACE(ch)].immune, ISR_ACID))
    dam = 0;
  else {
    if (IS_AFFECTED(ch, AFF_EAS))
      dam /= 4;
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
      dam /= 2;
      
    dam /= 4;
  }
    
  GET_HIT(ch) -= dam;
  do_dam_msgs(vict, ch, dam, SPELL_ACID_SHIELD, WIELD);
  update_pos(ch);
    
  if (GET_POS(ch) == POSITION_DEAD) {
      group_gain(vict, ch);
      fight_kill(vict, ch, TYPE_CHOOSE);
      return eSUCCESS|eCH_DIED;
  }

  return eSUCCESS;
}

int get_weapon_damage_type(struct obj_data * wielded) {
    char log_buf[256];

    switch(wielded->obj_flags.value[3]) { 
    case 0:
    case 1:
    case 2:  return TYPE_WHIP;     break;
    case 3:  return TYPE_SLASH;    break;
    case 4:
    case 5:
    case 6:  return TYPE_CRUSH;    break;
    case 7:  return TYPE_BLUDGEON; break;
    case 8:  return TYPE_STING;    break;
    case 9:
    case 10:
    case 11: return TYPE_PIERCE;   break;
    default: 
      sprintf(log_buf, "WORLD: Unknown w_type for object #%d name: %s, fourth value flag is: %d.", 
          wielded->item_number, wielded->name, wielded->obj_flags.value[3]);
      log(log_buf, OVERSEER, LOG_BUG);
      break;
    } 
    return TYPE_HIT; // should never get here
}

int get_monk_bare_damage(char_data * ch) {
    int dam = 0;

    if(GET_LEVEL(ch) < 5)
      dam = dice(1, 3);
    else if(GET_LEVEL(ch) < 10)
      dam = dice(1, 4);
    else if(GET_LEVEL(ch) < 15)
      dam = dice(2, 3);
    else if(GET_LEVEL(ch) < 20)
      dam = dice(2, 4);
    else if(GET_LEVEL(ch) < 25)
      dam = dice(3, 3);
    else if(GET_LEVEL(ch) < 30)
      dam = dice(3, 4);
    else if(GET_LEVEL(ch) < 35)
      dam = dice(4, 3);
    else if(GET_LEVEL(ch) < 40)
      dam = dice(4, 4);
    else if(GET_LEVEL(ch) < 45)
      dam = dice(5, 3);
    else if(GET_LEVEL(ch) < 50)
      dam = dice(5, 4);
    else if(GET_LEVEL(ch) < 55)
      dam = dice(5, 5);
    else if(GET_LEVEL(ch) < IMMORTAL)
      dam = dice(10, 6);
    else if(GET_LEVEL(ch) < IMP)
      dam = dice(10, 10);
    else
      dam = dice(50, 5);

    return dam;
}

// standard "returnvals.h" returns
int one_hit(CHAR_DATA *ch, CHAR_DATA *vict, int type, int weapon)
{
  struct obj_data *wielded;	/* convenience */
  int w_type;			/* Holds type info for damage() */
  int weapon_type;
  int victim_ac, calc_thaco;	/* Holders for Calculation */
  int dam;			/* Self explan. */
  int diceroll;			/* ... */
  int retval = 0;
  
  extern int thaco[8][61];
  extern struct str_app_type str_app[];
  extern byte backstab_mult[];
  
  int do_say(struct char_data *ch, char *argument, int cmd);
  
  if(!vict || !ch) { 
    log("Null victim or char in one_hit!  This Crashes us!", -1, LOG_BUG);
    return eINTERNAL_ERROR;
  }
  
  if(ch == vict) {
    do_say(ch, "What the hell am I DOING?!?!\r\n", 9);
    return eFAILURE;
  }
  
  if(GET_POS(vict) == POSITION_DEAD)            return eSUCCESS|eVICT_DIED;

  // TODO - I'd like to remove these 3 cause they are checked in attack()
  /* This happens with multi-attacks */
  if(ch->in_room != vict->in_room)              return eFAILURE;
  if(!can_be_attacked(ch, vict))                return eFAILURE;
  if(!can_attack(ch))                           return eFAILURE;

  // Figure out the correct weapon 
  wielded = ch->equipment[weapon];
  
  // Second got called without a secondary wield 
  if(weapon == SECOND && wielded == FALSE)      return 0;
  
  set_cantquit(ch, vict); /* This sets the flag if necessary */

  w_type = TYPE_HIT;
  if(wielded && wielded->obj_flags.type_flag == ITEM_WEAPON) 
     w_type = get_weapon_damage_type(wielded);
  
  weapon_type = w_type;
  if(type == SKILL_BACKSTAB)
    w_type = SKILL_BACKSTAB;
  
  /* Calculate thac0 vs. armor clss.  Thac0 for mobs is in hitroll */
  if(!IS_NPC(ch))
    calc_thaco = thaco[(int)GET_CLASS(ch) - 1][(int)GET_LEVEL(ch)];
  else /* ch is a mob */
    calc_thaco = 20;
  
  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
  calc_thaco -= GET_HITROLL(ch);
  
  /* Calculate victim's ac */
  victim_ac = GET_AC(vict) / 10;
  victim_ac = MAX(-20, victim_ac);
  
  /* Roll the dice! */
  diceroll = number(1, 20);
  
  /* Can't miss a victim with these effects! */
  if(IS_AFFECTED(vict, AFF_PARALYSIS) || !AWAKE(vict) ||
    IS_SET(vict->combat, COMBAT_STUNNED) ||
    IS_SET(vict->combat, COMBAT_STUNNED2) ||
    IS_SET(vict->combat, COMBAT_SHOCKED))
    diceroll = 20;
  
  /* miss! */
  if(diceroll < 20 && AWAKE(vict) &&
    (diceroll == 1 || diceroll < calc_thaco - victim_ac)) { 
    return damage(ch, vict, 0, w_type, w_type, weapon);
  }
  
  if(wielded)  {
    dam = dice(wielded->obj_flags.value[1], wielded->obj_flags.value[2]);
    if(IS_NPC(ch)) {
      dam = dice(ch->mobdata->damnodice, ch->mobdata->damsizedice);
      dam += (dice(wielded->obj_flags.value[1], wielded->obj_flags.value[2])/2);
    }
  }
  else if(IS_NPC(ch))
    dam = dice(ch->mobdata->damnodice, ch->mobdata->damsizedice);
  else if(GET_CLASS(ch) == CLASS_MONK && wielded == FALSE)
    dam = get_monk_bare_damage(ch);
  else dam = number(0, GET_LEVEL(ch)/2);

  /* Damage bonuses */
  dam += str_app[STRENGTH_APPLY_INDEX(ch)].todam;
  dam += GET_DAMROLL(ch);
  
  // Bonus for hitting weak opponents
  if(GET_POS(vict) < POSITION_FIGHTING)
    dam *= 1 + (2 * (POSITION_FIGHTING - GET_POS(vict))) / 10;
  
  // BACKSTAB GOES IN HERE!
  if(type == SKILL_BACKSTAB) { 
    if(IS_SET(ch->combat, COMBAT_CIRCLE)) { 
      dam *= ((backstab_mult[(int)GET_LEVEL(ch)]) / 2);
      REMOVE_BIT(ch->combat, COMBAT_CIRCLE);
    }
    else if((GET_CLASS(ch) == CLASS_THIEF) ||
      (GET_CLASS(ch) == CLASS_ANTI_PAL) || IS_NPC(ch)) 
    {
      if(GET_CLASS(ch) == CLASS_ANTI_PAL)
        dam *= (backstab_mult[(int)GET_LEVEL(ch)]+1);
      else dam *= backstab_mult[(int)GET_LEVEL(ch)];
    }
  }
  
  if(dam < 1)
    dam = 1;
  
  // Now check for special code that occurs each hit
  retval = do_skewer(ch, vict, dam, weapon);

  // if we both died in the skewer, don't kill the victim
  if(IS_SET(retval, eCH_DIED)) {
    if(IS_SET(retval, eVICT_DIED)) {
      GET_HIT(vict) = 1;
      update_pos(vict);
    }
    REMOVE_BIT(retval, eVICT_DIED);
    return retval;
  }

  // HA!  skewered the bastard to death!
  if(IS_SET(retval, eVICT_DIED)) { 
    group_gain(ch, vict);
    fight_kill(ch, vict, TYPE_CHOOSE);
    return eSUCCESS|eVICT_DIED;
  }
  
  retval = damage(ch, vict, dam, weapon_type, w_type, weapon);

  if(!IS_SET(retval, eCH_DIED) && 
     !IS_SET(retval, eVICT_DIED) && 
      IS_SET(retval, eSUCCESS))
    retval = weapon_spells(ch, vict, weapon);

  // weapon spells is going to return failure if a spell didn't go off.  However,
  // we did actually hit the opponent, so set it to success and get out
  SET_BIT(retval, eSUCCESS);

  // if we got here, i hit him, and noone died
  return retval;  
} // one_hit 




//   Handles equipment damage...   was tired of modifying it in both
//   procedures..  besides, kinda redundant having it in both anyhows....
//   Eventually I will do the same for ISR code,and hopefully clean up
//   fight.c some more.. :)     -Godflesh...

// This code sucks!!
// I just went through this to fix the gl lantern scrap thing,
// and it had it all.  crash bugs, memory leaks, item corruptions,
// it was and still is a gross piece of shit, although i fixed the
// glaring problems.
// -Sadus 8/23/96

// It still sucks, but i'm working on it...-pir  02/16/01

// pos of -1 means inventory
void eq_destroyed(char_data * ch, obj_data * obj, int pos)
{
  if(pos != -1)
  {
    unequip_char(ch, pos);

    if(pos == WEAR_LIGHT) {
      world[ch->in_room].light--;
      ch->glow_factor--;
    }
    else if((pos == WIELD) && (ch->equipment[SECOND_WIELD])) {
      obj_data * temp;
      temp = unequip_char(ch, SECOND_WIELD);
      equip_char(ch, temp, WIELD);
    }
    act("$p carried by $n is destroyed.", ch, obj, 0, TO_ROOM, 0);
  }
  else act("$p worn by $n is destroyed.", ch, obj, 0, TO_ROOM, 0);

  act("Your $p has been destroyed.", ch, obj, 0, TO_CHAR, 0);
  act("$p falls to the ground in scraps.", ch, obj, 0, TO_CHAR, 0);
  act("$p falls to the ground in scraps.", ch, obj, 0, TO_ROOM, 0);
  make_scraps(ch, obj);
  extract_obj(obj);
}

void eq_damage(CHAR_DATA * ch, CHAR_DATA * victim,
    int dam, int weapon_type, int attacktype) 
{
  int count, eqdam, chance, value;
  struct obj_data * obj;
  int pos;
    
  if(IS_SET(world[ch->in_room].room_flags, ARENA)) // Don't damage eq in arena
    return;
  if(!IS_NPC(victim) && !IS_NPC(ch))        // Don't damage eq on pc->pc fights
    return;

  chance = GET_DEX(ch);
  if(dam < 40)                              // Under 40 damage decreases chance of damage
    chance += 40 - dam;                     // Helps out the lower level chars
    
  if(number(0, chance))                     // 1 in chance of eq damage
    return;

  //  let's scrap some worn eq
  if(number(0, 3) == 0) 
  {
    // TODO - eventually we need to determine where someone gets hit and just pass the location

    // determine which location takes the damage giving a higher chance to certain locations
    pos = number(0, WEAR_MAX+6);
    switch(pos) {
      case WEAR_MAX+1:
      case WEAR_MAX+2:
         pos = WEAR_BODY;
         break;
      case WEAR_MAX+3:
         pos = WEAR_LEGS;
         break;
      case WEAR_MAX+4:
         pos = WEAR_ARMS;
         break;
      case WEAR_MAX+5:
         pos = WEAR_HEAD;
         break;
      case WEAR_MAX+6:
         pos = WEAR_SHIELD;
         break;
      default:
         break; // pos = itself
    }
          
    if(!victim->equipment[pos])  // they lucked out and didn't get anything hit
       return;

    obj = victim->equipment[pos];
            
    // add a point
    eqdam = damage_eq_once(obj);

    // determine if time to scrap it              
    if(eqdam >= eq_max_damage(obj))
      eq_destroyed(victim, obj, pos);
    else {
      act("$p is damaged.", victim, obj, 0, TO_CHAR, 0);
      act("$p worn by $n is damaged.", victim, obj, 0, TO_ROOM, 0);
    }
  } // number(0, 3) == 0
  else if(victim->carrying && (number(0, 6) == 0)) 
  { 
    // let's scrap something in the inventory 
    for(count = 0, obj = victim->carrying; obj; obj = obj->next_content)
        count++;
    value = number(1, count); // choose a random inventory item
    // loop up to that item
    for(count = 0, obj = victim->carrying; obj && value < count; obj = obj->next_content)
        count++;

    assert(obj);
       
    // add a point
    eqdam = damage_eq_once(obj);

    // determine if time to scrap it              
    if(eqdam >= eq_max_damage(obj))
      eq_destroyed(victim, obj, -1);
    else {
      act("$p is damaged.", victim, obj, 0, TO_CHAR, 0);
      act("$p carried by $n is damaged.", victim, obj, 0, TO_ROOM, 0);
    }
  }  // end of inventory damage... 

  save_char_obj(victim);
}

void pir_stat_loss(char_data * victim)
{
  /* Pir's extra stat loss.  Bwahahah */
  if(number(1,100) <= GET_LEVEL(victim) && GET_LEVEL(victim) >= 50 && 
    !IS_NPC(victim)) 
  {
    switch(number(1,5)) 
    {
    case 1: if(GET_STR(victim) > 4) 
            {
              GET_STR(victim) -= 1;
              victim->raw_str -=1 ;
              send_to_char("*** You lose one strength point ***\r\n", victim);
              sprintf(log_buf, "%s lost a str too. ouch.", GET_NAME(victim));
            }
      break;
    case 2: if(GET_WIS(victim) > 4) 
            {
              GET_WIS(victim) -= 1;
              victim->raw_wis -=1 ;
              send_to_char("*** You lose one wisdom point ***\r\n", victim);
              sprintf(log_buf, "%s lost a wis too. ouch.", GET_NAME(victim));
            }
      break;
    case 3: GET_CON(victim) -= 1;
      victim->raw_con -=1 ;
      send_to_char("*** You lose another constitution point ***\r\n", victim);
      sprintf(log_buf, "%s lost a con too. ouch.", GET_NAME(victim));
      break;
    case 4: if(GET_INT(victim) > 4) 
            {
              GET_INT(victim) -= 1;
              victim->raw_intel -=1 ;
              send_to_char("*** You lose one intelligence point ***\r\n", victim);
              sprintf(log_buf, "%s lost a int too. ouch.", GET_NAME(victim));
            }
      break;
    case 5: if(GET_DEX(victim) > 4) 
            {
              GET_DEX(victim) -= 1;
              victim->raw_dex -=1 ;
              send_to_char("*** You lose one dexterity point ***\r\n", victim);
              sprintf(log_buf, "%s lost a dex too. ouch.", GET_NAME(victim));
            }
      break;
    } // of switch
    log(log_buf, SERAPH, LOG_MORTAL);
  } // of pir's extra stat loss
}

int damage_retval(CHAR_DATA * ch, CHAR_DATA * vict, int value)
{
  // we need to make sure in the case of a reflect or something
  // that we are returning the death of the CH if he died

  if(ch == vict && IS_SET(value, eVICT_DIED))
    return (value|eCH_DIED);

  return value;
}

// returns standard returnvals.h return codes
int damage(CHAR_DATA * ch, CHAR_DATA * victim,
           int dam, int weapon_type, int attacktype, int weapon)
{
  long weapon_bit;
  struct obj_data *wielded;
  int typeofdamage;
  int damage_type(int weapon_type);
  long int get_weapon_bit(int weapon_type);
  int32 hit_limit(CHAR_DATA * ch);
  int retval;
  int modifier = 0;
  int learned;
  
  if(!weapon)
    weapon = WIELD;
  typeofdamage = damage_type(weapon_type);

  if(GET_POS(victim) == POSITION_DEAD)           return eSUCCESS|eVICT_DIED;

  if(typeofdamage == DAMAGE_TYPE_MAGIC)  
  {
    if(IS_AFFECTED(victim, AFF_REFLECT)  && 
       number(1,101) < 6)
    {
      if(ch == victim) { // some idiot was shooting at himself
        act("Your spell reflects into the unknown.", ch, 0, 0, TO_CHAR, 0);
        act("$n's spell rebounds into the unknown.", ch, 0, 0, TO_ROOM, 0);
        return eSUCCESS;
      } else {
        act("$n's spell bounces back at him", ch, 0, victim, TO_VICT, 0);
        act("Oh SHIT! Your spell bounces off of $N and heads right back at you.", ch, 0, victim, TO_CHAR, 0);
        act("$n's spell reflects off of $N's magical aura", ch, 0, victim, TO_ROOM, NOTVICT);
        victim = ch;
      }
    }
    if(IS_SET(victim->combat, COMBAT_REPELANCE))
    {
       if(GET_LEVEL(ch) > 70)
         send_to_char("The power of the spell bursts through your mental barriers as if they weren't there!\r\n", victim);
       else if(!(number(0, 9)))
         send_to_char("Your mental shields cannot hold back the force of the spell!\r\n", victim);
       else {
        act("$n's spell is dissolved into nothingness by your will.", ch, 0, victim, TO_VICT, 0);
        act("$N supreme will dissolves your spell into formless mana.", ch, 0, victim, TO_CHAR, 0);
        act("$n's spell streaks at $N and suddenly ceases to be.", ch, 0, victim, TO_ROOM, NOTVICT);
        REMOVE_BIT(victim->combat, COMBAT_REPELANCE);
        return eSUCCESS;
       }
       REMOVE_BIT(victim->combat, COMBAT_REPELANCE);
    }
  }
  // Can't hurt god, but he likes to see the messages. 
  if (GET_LEVEL(victim) >= IMMORTAL && !IS_NPC(victim))
    dam = 0;
  
  if(victim != ch) {
    if(!can_attack(ch) || !can_be_attacked(ch, victim))
      return eFAILURE;
    set_cantquit(ch, victim);
  }
  
  /* An eye for an eye, a tooth for a tooth, a life for a life. */
  if (GET_POS(victim) > POSITION_STUNNED && ch != victim)
  {
    if(!victim->fighting)
      set_fighting(victim, ch);

    if((!IS_SET(victim->combat, COMBAT_STUNNED)) &&
      (!IS_SET(victim->combat, COMBAT_STUNNED2)) &&
      (!IS_SET(victim->combat, COMBAT_BASH1)) &&
      (!IS_SET(victim->combat, COMBAT_BASH2)) )
    {
      if (GET_POS(victim) > POSITION_STUNNED)
      {
        if (GET_POS(victim) < POSITION_FIGHTING) 
        {
          act("$n scrambles to $s feet!", victim, 0, 0, TO_ROOM, 0);
          act("You scramble to your feet!", victim, 0, 0, TO_CHAR, 0);
          GET_POS(victim) = POSITION_FIGHTING;
        }
      }
    }
  }
  else if(GET_POS(victim) == POSITION_SLEEPING) 
  {
    affect_from_char(victim, INTERNAL_SLEEPING);
    act("$n is shocked to a wakened state and scrambles to $s feet!", victim, 0, 0, TO_ROOM, 0);
    send_to_char("You are awakened from combat adrenaline springing you to your feet!", ch);
    GET_POS(victim) = POSITION_FIGHTING;
  }

  if (GET_POS(ch) > POSITION_STUNNED && ch != victim) {
    if (!ch->fighting)
      set_fighting(ch, victim);
  }

  if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
    act("$n slowly fades into existence.", ch, 0, 0, TO_ROOM, 0);
    if (affected_by_spell(ch, SPELL_INVISIBLE))
      affect_from_char(ch, SPELL_INVISIBLE);
    REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
  }

  // Frost Shield won't effect a backstab -pir
  if(attacktype != SKILL_BACKSTAB &&  GET_HIT(victim) > 0 &&
     typeofdamage == DAMAGE_TYPE_PHYSICAL)
    if(do_frostshield(ch, victim)) {
      return eSUCCESS;
    }

  if(typeofdamage == DAMAGE_TYPE_PHYSICAL) {
    if (IS_SET(ch->combat, COMBAT_BERSERK))
      dam = (int)(dam * 1.6);
    if (IS_SET(ch->combat, COMBAT_RAGE1) || IS_SET(ch->combat, COMBAT_RAGE2) && attacktype != SKILL_BACKSTAB)
      dam = (int)(dam * 1.3);
    if (IS_SET(ch->combat, COMBAT_HITALL))
      dam = (int)(dam * 2);
    if(( ((GET_HIT(ch) / GET_MAX_HIT(ch))*100) < 30 )   // less than 30% hps
         && (learned = has_skill(ch, SKILL_FRENZY))) {
      if(learned > number(1, 101)) {
        dam = (int)(dam * 1.2);
        SET_BIT(modifier, COMBAT_MOD_FRENZY);
      }
    }
    if(IS_SET(ch->combat, COMBAT_VITAL_STRIKE))
      dam = (int)(dam * 2);
  }
  if (IS_AFFECTED(victim, AFF_EAS))
    dam /= 4;

  // sanct is 3/4 damage now
  if (IS_AFFECTED(victim, AFF_SANCTUARY))
      dam -= (int) (dam/4);

  if(IS_SET(victim->combat, COMBAT_MONK_STANCE))  // half damage
      dam /= 2;

  if (attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING)
  {
    if(check_shieldblock(ch, victim))
      return eFAILURE;

    if(check_parry(ch, victim)) {
      if(typeofdamage == DAMAGE_TYPE_PHYSICAL)
          return damage_retval(ch, victim, check_riposte(ch, victim));
    }
    if (check_dodge(ch, victim))
      return eFAILURE;
  }

  if (attacktype == TYPE_PHYSICAL_MAGIC)
  {
    // Physical Magic can be dodged or blocked with a shield, but not parried
    if(check_shieldblock(ch, victim))
      return eFAILURE;
    if (check_dodge(ch, victim))
      return eFAILURE;
  }

  struct affected_type * pspell = NULL;
  if(GET_LEVEL(victim) < IMMORTAL && 
     (
      (pspell = affected_by_spell(victim, SPELL_STONE_SHIELD)) ||
      (pspell = affected_by_spell(victim, SPELL_GREATER_STONE_SHIELD))
     )
    )
  {
    dam -= 30;
    if(dam < 1)  dam = 1;
    pspell->duration--;
    if(0 == pspell->duration) {
      send_to_char("The ethereal stones protecting you shatter and fade into nothing.\r\n", victim);
      act("The ethereal stones surrounding $n shatter into nothingness.\r\n", victim, 0, 0, TO_ROOM, 0);
      affect_from_char(victim, SPELL_STONE_SHIELD);
      affect_from_char(victim, SPELL_GREATER_STONE_SHIELD);
    }
  }
  
  if (dam < 0)
    dam = 0;
  
  // Immune / Susceptibilities / Resistances 
  // Some code is in saves_spell  (for obvious reasons)
  weapon_bit = get_weapon_bit(weapon_type);
  
  if ((attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) ||
    (attacktype == SKILL_BACKSTAB)) 
  {
    if (IS_SET(victim->immune, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;
    else if (IS_SET(victim->resist, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;
    else if (IS_SET(victim->suscept, ISR_PHYSICAL))
      weapon_bit += ISR_PHYSICAL;
    
    wielded = ch->equipment[weapon];
    
    if(wielded) 
    {
      if ((IS_SET(victim->immune, ISR_MAGIC)) &&
        (IS_SET(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
        weapon_bit += ISR_MAGIC;
      if ((IS_SET(victim->immune, ISR_NON_MAGIC)) &&
        (!IS_SET(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
        weapon_bit += ISR_NON_MAGIC;
      if ((IS_SET(victim->suscept, ISR_MAGIC)) &&
        (IS_SET(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
        weapon_bit += ISR_MAGIC;
      if ((IS_SET(victim->suscept, ISR_NON_MAGIC)) &&
        (!IS_SET(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
        weapon_bit += ISR_NON_MAGIC;
      if ((IS_SET(victim->resist, ISR_MAGIC)) &&
        (IS_SET(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
        weapon_bit += ISR_MAGIC;
      if ((IS_SET(victim->resist, ISR_NON_MAGIC)) &&
        (!IS_SET(wielded->obj_flags.extra_flags, ITEM_MAGIC)) )
        weapon_bit += ISR_NON_MAGIC;
    }
  }
  
  if (IS_SET(victim->immune, weapon_bit) &&
    !IS_AFFECTED(victim, AFF_CHARM)) 
  {
    dam = 0;
    if (attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) {
      act("You ignore $n's puny weapon.", ch, 0, victim, TO_VICT, 0);
      act("$N ignores your puny weapon.", ch, 0, victim, TO_CHAR, 0);
      act("$N ignores $n's puny weapon.", ch, 0, victim, TO_ROOM, NOTVICT);
    }
  } 
  else if (IS_SET(victim->suscept, weapon_bit)) 
  {
    dam = (int)(dam * 1.3);
    if (attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING) {
      act("You shudder from the power of $n's weapon.", ch, 0, victim, TO_VICT, 0);
      act("$N shudders from the power of your weapon.", ch, 0, victim, TO_CHAR, 0);
      act("$N shudders from the power of $n's weapon.", ch, 0, victim, TO_ROOM, NOTVICT);
    } else {
      act("You shudder from the power of $n's spell.", ch, 0, victim, TO_VICT, 0);
      act("$N shudders from the power of your spell.", ch, 0, victim, TO_CHAR, 0);
      act("$N shudders from the power of $n's spell.", ch, 0, victim, TO_ROOM, NOTVICT);
    }
  } 
  else if (IS_SET(victim->resist, weapon_bit)) 
  {
    dam = (int)(dam * 0.7);
    if (attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING)  {
      act("You resist against the power of $n's weapon.", ch, 0, victim, TO_VICT, 0);
      act("$N resists against the power of your weapon.", ch, 0, victim, TO_CHAR, 0);
      act("$N resists against the power of $n's weapon.", ch, 0, victim, TO_ROOM, NOTVICT);
    } 
    else {
      act("You resist against the power of $n's spell.", ch, 0, victim, TO_VICT, 0);
      act("$N resists against the power of your spell.", ch, 0, victim, TO_CHAR, 0);
      act("$N resists against the power of $n's spell.", ch, 0, victim, TO_ROOM, NOTVICT);
    }
  }
  
  if (dam < 0)
    dam = 0;
  
  // Check for parry, mob disarm, and trip. Print a suitable damage message. 
  if (attacktype >= TYPE_HIT && attacktype < TYPE_SUFFERING)
  {
    if (ch->equipment[weapon] == NULL)
      dam_message(dam, ch, victim, TYPE_HIT, modifier);
    else dam_message(dam, ch, victim, attacktype, modifier);
    
    GET_HIT(victim) -= dam;
    update_pos(victim);
  } else {
    GET_HIT(victim) -= dam;
    update_pos(victim);
    do_dam_msgs(ch, victim, dam, attacktype, weapon);
  }
  
  /*  Now for eq damage...   */
  if(dam > 0 && typeofdamage == DAMAGE_TYPE_PHYSICAL)
      eq_damage(ch, victim, dam, weapon_type, attacktype);

  inform_victim(ch, victim, dam);

  if(typeofdamage == DAMAGE_TYPE_PHYSICAL && dam > 0)
  {
    retval = do_fireshield(ch, victim, dam);
    if(SOMEONE_DIED(retval))
      return damage_retval(ch, victim, retval);
    retval = do_acidshield(ch, victim, dam);
    if(SOMEONE_DIED(retval))
      return damage_retval(ch, victim, retval);
    retval = do_lightning_shield(ch, victim, dam);
    if(SOMEONE_DIED(retval))
      return damage_retval(ch, victim, retval);
  }

  // Sleep spells.
  if (!IS_SET(victim->combat, COMBAT_STUNNED) &&
    !IS_SET(victim->combat, COMBAT_STUNNED2) )
    if (!AWAKE(victim)) 
    {
      if (victim->fighting)
        stop_fighting(victim);
      
      if (IS_SET(victim->combat, COMBAT_BERSERK))
        REMOVE_BIT(victim->combat, COMBAT_BERSERK);
      if (IS_SET(victim->combat, COMBAT_RAGE1))
        REMOVE_BIT(victim->combat, COMBAT_RAGE1);
      if (IS_SET(victim->combat, COMBAT_RAGE2))
        REMOVE_BIT(victim->combat, COMBAT_RAGE2);
    }
      
  // Payoff for killing things. 
  if(GET_POS(victim) == POSITION_DEAD)
  {
    group_gain(ch, victim);
    fight_kill(ch, victim, TYPE_CHOOSE);
    return damage_retval(ch, victim, (eSUCCESS|eVICT_DIED));
  } 
  return eSUCCESS;
} 

// I removed this function and combined it with "damage".  There was
// no point in having two huge functions that did almost the same thing.
// damage() can tell if it's a magic/nonmagic attack anyway
// I'll remove this when I have time to go through and change all the
// spells in "spells.C" to use damage() instead
// -pir 7/26/00
int spell_damage(CHAR_DATA * ch, CHAR_DATA * victim,
           int dam, int weapon_type, int attacktype, int weapon)
{
  return damage(ch, victim, dam, weapon_type, attacktype, weapon);
}

int is_pkill(CHAR_DATA *ch, CHAR_DATA *vict)
{
  CHAR_DATA *tmp_ch;

  // TODO - change this so a mob following another mob isn't a pkill
  
  for(tmp_ch = ch; tmp_ch; tmp_ch = tmp_ch->master) 
  { 
    if(!IS_NPC(tmp_ch))
      if(IS_NPC(vict)) 
      { 
        if(vict->master) /* Attacking someone's charmie */
          if(!IS_NPC(vict->master))
            if(vict->master != ch) /* Can't pkill your own charmie */
              return TRUE;
            return FALSE; /* Standard mob kill */
      }
      else /* vict is a pc */
        return TRUE;
  }
  
  /* Still here?  It's an uncharmed mob fighting an uncharmed mob! */
  return FALSE;
}

void do_dam_msgs(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int attacktype, int weapon)
{
  extern struct message_list fight_messages[MAX_MESSAGES];
  struct message_type *messages;
  int i,j,  nr;
  for (i = 0; i < MAX_MESSAGES; i++)
  {
    if (fight_messages[i].a_type != attacktype)
      continue;
    
    nr = dice(1, fight_messages[i].number_of_attacks);
    j = 1;
    for (messages = fight_messages[i].msg; j < nr && messages; j++)
      messages = messages->next;
    
    if (!IS_NPC(victim) && GET_LEVEL(victim) >= IMMORTAL)
    {
      act(messages->god_msg.attacker_msg,
        ch, ch->equipment[weapon], victim, TO_CHAR, 0);
      act(messages->god_msg.victim_msg,
        ch, ch->equipment[weapon], victim, TO_VICT, 0);
      act(messages->god_msg.room_msg,
        ch, ch->equipment[weapon], victim, TO_ROOM, NOTVICT);
    }
    else
      if (dam == 0)
      {
        act(messages->miss_msg.attacker_msg,
          ch, ch->equipment[weapon], victim, TO_CHAR, 0);
        act(messages->miss_msg.victim_msg,
          ch, ch->equipment[weapon], victim, TO_VICT, 0);
        act(messages->miss_msg.room_msg,
          ch, ch->equipment[weapon], victim, TO_ROOM, NOTVICT);
      }
      else
        if (GET_POS(victim) == POSITION_DEAD)
        {
          act(messages->die_msg.attacker_msg,
            ch, ch->equipment[weapon], victim, TO_CHAR, 0);
          act(messages->die_msg.victim_msg,
            ch, ch->equipment[weapon], victim, TO_VICT, 0);
          act(messages->die_msg.room_msg,
            ch, ch->equipment[weapon], victim, TO_ROOM, NOTVICT);
        }
        else
        {
          act(messages->hit_msg.attacker_msg,
            ch, ch->equipment[weapon], victim, TO_CHAR, 0);
          act(messages->hit_msg.victim_msg,
            ch, ch->equipment[weapon], victim, TO_VICT, 0);
          act(messages->hit_msg.room_msg,
            ch, ch->equipment[weapon], victim, TO_ROOM, NOTVICT);
        }
  }
}

void set_cantquit(CHAR_DATA *ch, CHAR_DATA *vict)
{
  struct affected_type af, *paf;
  
  if(!ch) return;  /* This will happen if the char was in a fall room */
  
  if(ch == vict)
    return;
  
  /* can't get pkill flag in the arena! */
  if(IS_ARENA(ch->in_room))
    return;
  
  if(is_pkill(ch, vict) && !IS_SET(vict->affected_by, AFF_CANTQUIT)) { 
    af.type = FUCK_CANTQUIT;
    af.duration = 5;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_CANTQUIT;
    
    if(!IS_SET(ch->affected_by, AFF_CANTQUIT))
      affect_to_char(ch, &af);
    else {
      for(paf = ch->affected; paf; paf = paf->next) {
        if(paf->type == FUCK_CANTQUIT)
          paf->duration = 5;
      }
    }
  }
}

void fight_kill(CHAR_DATA *ch, CHAR_DATA *vict, int type)
{
  if (!vict) {
    log("Null vict sent to fight_kill()!", -1, LOG_BUG);
    return;
  }
  
  if (vict->fighting)
    stop_fighting(vict);
  if (ch)
    if (ch->fighting)
      stop_fighting(ch);
    
    switch(type)
    {
    case TYPE_CHOOSE:
      /* if it's a mob then it can't be pkilled */
      if(IS_NPC(vict))
        raw_kill(ch, vict);
      else if(IS_ARENA(vict->in_room))
        arena_kill(ch, vict);
      else if(is_pkill(ch, vict))
        do_pkill(ch, vict);
      else
        raw_kill(ch, vict);
      break;
    case TYPE_PKILL: do_pkill(ch, vict); break;
    case TYPE_RAW_KILL: raw_kill(ch, vict); break;
    case TYPE_ARENA_KILL: arena_kill(ch, vict); break;
    }
}

// check riposte never returns eSUCCESS because that would
// get returned from damage as a successful damage, which it's
// not.
int check_riposte(CHAR_DATA * ch, CHAR_DATA * victim)
{
  int percent;
  int chance;
  int retval;
  int newretval;
  
  if((IS_SET(victim->combat, COMBAT_STUNNED)) ||
    (ch->equipment[WIELD] == NULL && number(1, 101) >= 50) ||
    (IS_SET(victim->combat, COMBAT_STUNNED2)) ||
    (IS_SET(victim->combat, COMBAT_BASH1)) ||
    (IS_SET(victim->combat, COMBAT_BASH2)) ||
    (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return eFAILURE;
  
  chance = 0;

  // TODO - eventually, when mobs have skills, remove this  
  if(IS_NPC(victim)) {
    if(GET_CLASS(victim) == CLASS_WARRIOR && GET_LEVEL(victim) > 40)
      chance = 25;
    else return eFAILURE;
  }
  else if ((chance = has_skill(victim, SKILL_RIPOSTE)))
    chance += GET_DEX(ch);
  else return eFAILURE;
  
  if(IS_SET(victim->combat, COMBAT_BLADESHIELD1) || IS_SET(victim->combat, COMBAT_BLADESHIELD2)) {
    if(chance < 75)
      chance = 75;
    if(IS_SET(ch->combat, COMBAT_BLADESHIELD1) || IS_SET(ch->combat, COMBAT_BLADESHIELD2))
      chance = 1;
  }

  percent = (number(1, 101) - GET_LEVEL(victim)) + GET_LEVEL(ch);
  if (percent >= chance)
    return eFAILURE;
  
  
  act("$n turns $N's attack into one of $s own!", victim, NULL, ch, TO_ROOM, NOTVICT);
  act("$n turns your attack against you!", victim, NULL, ch, TO_VICT, 0);
  act("You turn $N's attack against $m.", victim, NULL, ch, TO_CHAR, 0);
  
  retval = one_hit(victim, ch, TYPE_UNDEFINED, FIRST);
  retval = SWAP_CH_VICT(retval);

  REMOVE_BIT(newretval, eSUCCESS);
  SET_BIT(newretval, eFAILURE);
  
  return newretval;  
}


bool check_shieldblock(CHAR_DATA * ch, CHAR_DATA * victim)
{
  int percent;
  int chance;
  
  if((IS_SET(victim->combat, COMBAT_STUNNED)) ||
    (victim->equipment[WEAR_SHIELD] == NULL) ||
    (IS_NPC(victim) && (!IS_SET(victim->mobdata->actflags, ACT_PARRY))) ||
    (IS_SET(victim->combat, COMBAT_STUNNED2)) ||
    (IS_SET(victim->combat, COMBAT_BASH1)) ||
    (IS_SET(victim->combat, COMBAT_BASH2)) ||
    (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return FALSE;
  
  chance = 0;

  // TODO - remove this when mobs have "skills"
  if (IS_NPC(victim))
  {
    switch(GET_CLASS(victim)) {
      case CLASS_MONK:
      case CLASS_ANTI_PAL:
      case CLASS_PALADIN:
      case CLASS_WARRIOR:  chance = GET_LEVEL(victim); break;
      case CLASS_RANGER:
      case CLASS_BARBARIAN:
      case CLASS_THIEF:    chance = GET_LEVEL(victim) - 10;
                           if(chance < 10) chance = 1; break;
      default:
         return FALSE;
    }
  }
  else if (!(chance = has_skill(victim, SKILL_SHIELDBLOCK)))
    return FALSE;

  chance /= 2;
  chance += (int)(GET_DEX(ch) / 2);

  if((GET_LEVEL(ch) - GET_LEVEL(victim)) < 0)
    chance += (int)((GET_LEVEL(ch) - GET_LEVEL(victim)));

  percent = number(1, 101);
  if (percent >= chance)
    return FALSE;
  
  act("$n blocks $N's attack with $s shield.", victim, NULL, ch, TO_ROOM, NOTVICT);
  act("$n blocks your attack with $s shield.", victim, NULL, ch, TO_VICT, 0);
  act("You block $N's attack with your shield.", victim, NULL, ch, TO_CHAR, 0);
  return TRUE;
}

bool check_parry(CHAR_DATA * ch, CHAR_DATA * victim)
{
  int percent;
  int chance;
  int specialization;
  
  if((IS_SET(victim->combat, COMBAT_STUNNED)) ||
    (victim->equipment[WIELD] == NULL) ||
    (ch->equipment[WIELD] == NULL && number(1, 101) >= 50) ||
    (IS_NPC(victim) && (!IS_SET(victim->mobdata->actflags, ACT_PARRY))) ||
    (IS_SET(victim->combat, COMBAT_STUNNED2)) ||
    (IS_SET(victim->combat, COMBAT_BASH1)) ||
    (IS_SET(victim->combat, COMBAT_BASH2)) ||
    (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return FALSE;
  
  chance = 0;
  
  // TODO - when mobs have skills, remove this, and the act_parry flag
  if (IS_NPC(victim) && (IS_SET(victim->mobdata->actflags, ACT_PARRY)))
    chance = MIN(30, 2 * GET_LEVEL(victim));
  else
    if ((chance = has_skill(victim, SKILL_PARRY)))

  if (chance == 0)
    return FALSE;

  specialization = chance / 100;
  chance = chance % 100;

  chance /= 2;
  chance += (3 * GET_DEX(ch)) / 4;
  chance += 10 * specialization;    
  
  if((GET_LEVEL(ch) - GET_LEVEL(victim)) < 0)
    chance += (int)((GET_LEVEL(ch) - GET_LEVEL(victim)));

  if(chance < 1)
    chance = 1;

  if(chance > 100 ||
     IS_SET(victim->combat, COMBAT_BLADESHIELD1) || 
     IS_SET(victim->combat, COMBAT_BLADESHIELD2))
    chance = 100;
  
  percent = number(1, 101);
  if (percent >= chance)
    return FALSE;

  act("$n parries $N's attack.", victim, NULL, ch, TO_ROOM, NOTVICT);
  act("$n parries your attack.", victim, NULL, ch, TO_VICT, 0);
  act("You parry $N's attack.", victim, NULL, ch, TO_CHAR, 0);
  return TRUE;
}

/*
* Check for dodge.
*/
bool check_dodge(CHAR_DATA * ch, CHAR_DATA * victim)
{
  int percent;
  int chance;
  int specialization;
  
  // TODO - eventually have this check for mobs that have dodge SKILL

  if((IS_SET(victim->combat, COMBAT_STUNNED)) ||
    (IS_NPC(victim) && (!IS_SET(victim->mobdata->actflags, ACT_DODGE))) ||
    (IS_SET(victim->combat, COMBAT_STUNNED2)) ||
    (IS_SET(victim->combat, COMBAT_BASH1)) ||
    (IS_SET(victim->combat, COMBAT_BASH2)) ||
    (IS_AFFECTED(victim, AFF_PARALYSIS)))
    return FALSE;
  chance = 0;
  
  if (IS_NPC(victim) && (IS_SET(victim->mobdata->actflags, ACT_DODGE)))
    chance = MIN(20, 2 * GET_LEVEL(victim));
  else
    chance = has_skill(victim, SKILL_DODGE);

  if (chance == 0)
    return FALSE;

  specialization = chance / 100;
  chance = chance % 100;

  chance /= 2;
  chance += (3 * GET_DEX(ch)) / 5;    
  chance += 10 * specialization;     // specialization = 10% better chance

  if((GET_LEVEL(ch) - GET_LEVEL(victim)) < 0)
    chance += (int)((GET_LEVEL(ch) - GET_LEVEL(victim)));
  
  if(chance < 1)
    chance = 1;

  if(chance > 100)
    chance = 100;
  
  percent = number(1, 101);
  if (percent >= chance)
    return FALSE;
  
  act("$n dodges $N's attack.", victim, NULL, ch, TO_ROOM, NOTVICT);
  act("$n dodges your attack.", victim, NULL, ch, TO_VICT, 0);
  act("You dodge $N's attack.", victim, NULL, ch, TO_CHAR, 0);
  return TRUE;
}

/*
* Load fighting messages into memory.
*/
void load_messages(void)
{
	 FILE *fl;
   int i, type;
   extern struct message_list fight_messages[MAX_MESSAGES];
   struct message_type *messages;
   char chk[100];
   
   if (!(fl = dc_fopen(MESS_FILE, "r")))
   {
     perror("read messages");
     exit(0);
   }
   
   for (i = 0; i < MAX_MESSAGES; i++)
   {
     fight_messages[i].a_type = 0;
     fight_messages[i].number_of_attacks = 0;
     fight_messages[i].msg = 0;
   }
   
   fscanf(fl, " %s \n", chk);
   
   while (*chk == 'M')
   {
     fscanf(fl, " %d\n", &type);
     for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
       (fight_messages[i].a_type); i++);
     if (i >= MAX_MESSAGES)
     {
       log("Too many combat messages.", ANGEL, LOG_BUG);
       exit(0);
     }
     
#ifdef LEAK_CHECK
     messages = (struct message_type *)
       calloc(1, sizeof(struct message_type));
#else
     messages = (struct message_type *)
       dc_alloc(1, sizeof(struct message_type));
#endif
     fight_messages[i].number_of_attacks++;
     fight_messages[i].a_type = type;
     messages->next = fight_messages[i].msg;
     fight_messages[i].msg = messages;
     
     messages->die_msg.attacker_msg = fread_string(fl, 0);
     messages->die_msg.victim_msg = fread_string(fl, 0);
     messages->die_msg.room_msg = fread_string(fl, 0);
     messages->miss_msg.attacker_msg = fread_string(fl, 0);
     messages->miss_msg.victim_msg = fread_string(fl, 0);
     messages->miss_msg.room_msg = fread_string(fl, 0);
     messages->hit_msg.attacker_msg = fread_string(fl, 0);
     messages->hit_msg.victim_msg = fread_string(fl, 0);
     messages->hit_msg.room_msg = fread_string(fl, 0);
     messages->god_msg.attacker_msg = fread_string(fl, 0);
     messages->god_msg.victim_msg = fread_string(fl, 0);
     messages->god_msg.room_msg = fread_string(fl, 0);
     fscanf(fl, " %s \n", chk);
   }
   
   dc_fclose(fl);
}

void free_messages_from_memory()
{
  extern struct message_list fight_messages[MAX_MESSAGES];
  struct message_type *next_message = NULL;
  int i;
  
  for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type); i++)
    while(fight_messages[i].msg)
    {
      next_message = fight_messages[i].msg->next;
      dc_free(fight_messages[i].msg->die_msg.attacker_msg);
      dc_free(fight_messages[i].msg->die_msg.victim_msg);
      dc_free(fight_messages[i].msg->die_msg.room_msg);
      dc_free(fight_messages[i].msg->miss_msg.attacker_msg);
      dc_free(fight_messages[i].msg->miss_msg.victim_msg);
      dc_free(fight_messages[i].msg->miss_msg.room_msg);
      dc_free(fight_messages[i].msg->hit_msg.attacker_msg);
      dc_free(fight_messages[i].msg->hit_msg.victim_msg);
      dc_free(fight_messages[i].msg->hit_msg.room_msg);
      dc_free(fight_messages[i].msg->god_msg.attacker_msg);
      dc_free(fight_messages[i].msg->god_msg.victim_msg);
      dc_free(fight_messages[i].msg->god_msg.room_msg);
      dc_free(fight_messages[i].msg);
        fight_messages[i].msg = next_message;
    }
}

/*
* Set position of a victim.
*/
void update_pos(CHAR_DATA * victim)
{
  if (GET_HIT(victim) > 0)
  {
    if((!IS_SET(victim->combat, COMBAT_STUNNED)) &&
      (!IS_SET(victim->combat, COMBAT_STUNNED2)) )
      if (GET_POS(victim) <= POSITION_STUNNED)
        GET_POS(victim) = POSITION_STANDING;
      return;
  }
  else GET_POS(victim) = POSITION_DEAD;
}



/*
* Start fights.
*/
void set_fighting(CHAR_DATA * ch, CHAR_DATA * vict)
{
  CHAR_DATA *k, *next_char;
  int count = 0;
  
  if(ch->fighting) /* If he's already fighting */
    return;
  
  if(!IS_NPC(ch) && IS_NPC(vict))
    if(!IS_SET(vict->mobdata->actflags, ACT_STUPID))
      add_memory(vict, GET_NAME(ch), 'h');
 
  if(IS_NPC(ch) && IS_NPC(vict) && IS_AFFECTED(ch, AFF_CHARM) && 
     ch->master && !IS_NPC(ch->master))
    if(!IS_SET(vict->mobdata->actflags, ACT_STUPID))
      add_memory(vict, GET_NAME(ch->master), 'h');
   
  for (k = combat_list; k; k = next_char) {
    next_char = k->next_fighting;
    if (k->fighting == vict)
      count++;
  }
    
  if (count >= 6) {
    send_to_char("You can't get close enough to fight.",ch);
    return;
  }
    
  ch->next_fighting = combat_list;
  combat_list = ch;
    
  if (IS_AFFECTED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);
    
  ch->fighting = vict;
    
  if ((!IS_SET(ch->combat, COMBAT_STUNNED)) &&
      (!IS_SET(ch->combat, COMBAT_STUNNED2)) &&
      (!IS_SET(ch->combat, COMBAT_BASH1)) &&
      (!IS_SET(ch->combat, COMBAT_BASH2)) )
    GET_POS(ch) = POSITION_FIGHTING;
    
  return;
}

// Stop fights.
void stop_fighting(CHAR_DATA * ch)
{
  CHAR_DATA *tmp;
  
  if (!ch)
  {
    log("Null ch in stop_fighting.  This would have crashed us.", IMP, LOG_BUG);
    return;
  }
  
  if (!ch->fighting)
    return;
  
  // This is in the command interpreter now, so berserk lasts
  // until you are totally done fighting.
  // -Sadus
  /*
  if (IS_SET(ch->combat, COMBAT_BERSERK)) {
  REMOVE_BIT(ch->combat, COMBAT_BERSERK);
  act("$n settles down.", ch, 0, 0, TO_ROOM, 0);
  act("You settle down.", ch, 0, 0, TO_CHAR, 0);
  GET_AC(ch) -= 80;
  }
  */
  
  if (IS_SET(ch->combat, COMBAT_RAGE1)) {
    REMOVE_BIT(ch->combat, COMBAT_RAGE1);
    act("$n calms down.", ch, 0, 0, TO_ROOM, 0);
    act("Your mind seems a bit clearer now.", ch, 0, 0, TO_CHAR, 0);
  }
  if (IS_SET(ch->combat, COMBAT_RAGE2)) {
    REMOVE_BIT(ch->combat, COMBAT_RAGE2);
    act("$n calms down.", ch, 0, 0, TO_ROOM, 0);
    act("Your mind seems a bit clearer now.", ch, 0, 0, TO_CHAR, 0);
  }
  
  // make sure people aren't stuck unable to do anything
  if(IS_SET(ch->combat, COMBAT_SHOCKED))
    REMOVE_BIT(ch->combat, COMBAT_SHOCKED);
  
  affect_total(ch);
  
  GET_POS(ch) = POSITION_STANDING;
  update_pos(ch);
  
  if (!ch->fighting)
    return;
  
  // Remove ch's lag if he wasn't using wimpy.
  if (!IS_NPC(ch) && ch->desc && !IS_SET(ch->pcdata->toggles, PLR_WIMPY))
    ch->desc->wait = 0;
  
  if (ch == combat_next_dude)
    combat_next_dude = ch->next_fighting;
  
  if (combat_list == ch)
    combat_list = ch->next_fighting;
  else {
    for (tmp = combat_list; tmp && (tmp->next_fighting != ch);
    tmp = tmp->next_fighting)
      ;
    if (!tmp) {
      log("Stop_fighting: char not found", ANGEL, LOG_BUG);
      // abort();
      return;
    }
    tmp->next_fighting = ch->next_fighting;
  }
  
  ch->next_fighting = 0;
  ch->fighting = 0;
  
  if (IS_SET(ch->combat, COMBAT_BASH1))
    REMOVE_BIT(ch->combat, COMBAT_BASH1);
  if (IS_SET(ch->combat, COMBAT_BASH2))
    REMOVE_BIT(ch->combat, COMBAT_BASH2);
  if (IS_SET(ch->combat, COMBAT_STUNNED))
    REMOVE_BIT(ch->combat, COMBAT_STUNNED);
  if (IS_SET(ch->combat, COMBAT_STUNNED2))
    REMOVE_BIT(ch->combat, COMBAT_STUNNED2);
  
  return;
}


void make_scraps(CHAR_DATA *ch, struct obj_data *obj)
{
  struct obj_data *corpse /*, *o*/;
  char buf[MAX_STRING_LENGTH];
  /*int i;*/
  
#ifdef LEAK_CHECK
  corpse = (struct obj_data *)calloc(1, sizeof(struct obj_data));
#else
  corpse = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
#endif
  clear_object(corpse);
  
  corpse->item_number = NOWHERE;
  corpse->in_room = NOWHERE;
  corpse->name = str_hsh("scraps");
  
  sprintf(buf, "A pile of scraps from %s is lying here.",
    obj->short_description);
  corpse->description = str_hsh(buf);
  
  
  sprintf(buf, "a pile of scraps.");
  corpse->short_description = str_hsh(buf);
  
  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = ITEM_TAKE;
  corpse->obj_flags.value[0] = 0;
  corpse->obj_flags.value[3] = 0;
  corpse->obj_flags.weight = obj->obj_flags.weight;
  corpse->obj_flags.eq_level = 0;

  corpse->obj_flags.more_flags = 0;

  corpse->obj_flags.timer = 0;
  
  object_list_new_new_owner(corpse, 0);
  obj_to_room(corpse, ch->in_room);
  
  return;
}


#define MAX_NPC_CORPSE_TIME 3
#define MAX_PC_CORPSE_TIME 15

void make_corpse(CHAR_DATA * ch)
{
  struct obj_data *corpse, *o, *tmp_o, *blah;
  struct obj_data *money, *next_obj;
  extern struct obj_data *object_list;
  char buf[MAX_STRING_LENGTH];
  int i;
  
#ifdef LEAK_CHECK
  corpse = (struct obj_data *)calloc(1, sizeof(struct obj_data));
#else
  corpse = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
#endif
  clear_object(corpse);
  
  corpse->item_number = NOWHERE;
  corpse->in_room = NOWHERE;
  
  // If pc is in the name, the consent system works
  // Thieves don't deserve consent! Loot time!
  // Morc
  
  if(IS_NPC(ch) || IS_SET(ch->pcdata->punish, PUNISH_THIEF))
    sprintf(buf, "corpse %s", GET_NAME(ch));
  else
    sprintf(buf, "corpse %s pc", GET_NAME(ch));
  corpse->name = str_hsh(buf);
  
  sprintf(buf, "the corpse of %s is lying here.",
    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);
  
  sprintf(buf, "the corpse of %s",
    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);
  
  for(i = 0; i < MAX_WEAR; i++)
    if(ch->equipment[i])
      obj_to_char(unequip_char(ch, i), ch);
    
  if(GET_GOLD(ch) > 0) {
    money = create_money(GET_GOLD(ch));
    GET_GOLD(ch) = 0;
    obj_to_obj(money, corpse);
  }
  
  corpse->obj_flags.type_flag = ITEM_CONTAINER;
  corpse->obj_flags.wear_flags = ITEM_TAKE;
  corpse->obj_flags.value[0] = 0;	/* You can't store stuff in a corpse */
  corpse->obj_flags.value[3] = 1;	/* corpse identifier */
  corpse->obj_flags.weight = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  corpse->obj_flags.eq_level = 0;
  
  if(IS_NPC(ch)) {
    corpse->obj_flags.timer        = MAX_NPC_CORPSE_TIME;
    corpse->obj_flags.more_flags = 0;
  }
  else {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }
  
  for(o = ch->carrying; o; o = next_obj) {
    next_obj = o->next_content;
    
    if(IS_SET(o->obj_flags.extra_flags, ITEM_SPECIAL) &&
      (GET_ITEM_TYPE(o) == ITEM_CONTAINER))
      for(tmp_o = o->contains; tmp_o; tmp_o = blah) {
        blah = tmp_o->next_content;
        if(!IS_SET(tmp_o->obj_flags.extra_flags, ITEM_SPECIAL)) {
          move_obj(tmp_o, corpse);
        }
      } // if and for
      
      if(!IS_SET(o->obj_flags.extra_flags, ITEM_SPECIAL))
        move_obj(o, corpse);
      
  } // for
  
  corpse->next = object_list;
  object_list = corpse;
  
  for(o = corpse->contains; o; o->in_obj = corpse, o = o->next_content)
    ;
  
  object_list_new_new_owner(corpse, 0);
  obj_to_room(corpse, ch->in_room);
  
  return;
}

void make_dust(CHAR_DATA * ch)
{
  struct obj_data *o, *tmp_o, *blah;
  struct obj_data *money, *next_obj;
  int i;
  
  for(i = 0; i < MAX_WEAR; i++)
    if(ch->equipment[i])
      obj_to_char(unequip_char(ch, i), ch);
    
  if(GET_GOLD(ch) > 0) {
    money = create_money(GET_GOLD(ch));
    GET_GOLD(ch) = 0;
    obj_to_room(money, ch->in_room);
  }
  
  for(o = ch->carrying; o; o = next_obj) {
    next_obj = o->next_content;
    
    if(IS_SET(o->obj_flags.extra_flags, ITEM_SPECIAL) &&
      (GET_ITEM_TYPE(o) == ITEM_CONTAINER))
      for(tmp_o = o->contains; tmp_o; tmp_o = blah) {
        blah = tmp_o->next_content;
        if(!IS_SET(tmp_o->obj_flags.extra_flags, ITEM_SPECIAL))
          move_obj(tmp_o, ch->in_room);
        
      } // if and for
      
      if(!IS_SET(o->obj_flags.extra_flags, ITEM_SPECIAL))
        move_obj(o, ch->in_room);
      
  } // for
  
  return;
}


// ch kills victim
void change_alignment(CHAR_DATA *ch, CHAR_DATA *victim)
{
  int x = (abs(GET_ALIGNMENT(victim)) + 1000) / 100;
  
  x += ((GET_LEVEL(victim) - GET_LEVEL(ch)) / 5);  
  
  if(GET_ALIGNMENT(victim) >= 0)
    x *= (-2);
  else x /= 2;
  
  GET_ALIGNMENT(ch) += x;
  
  GET_ALIGNMENT(ch) = MIN(1000, MAX((-1000), GET_ALIGNMENT(ch)));  
}


/* head of a corpse or withered husk  */

void make_husk(CHAR_DATA *ch) {
  struct obj_data *corpse;
  char buf[MAX_STRING_LENGTH];
  
#ifdef LEAK_CHECK
  corpse = (struct obj_data*)calloc(1, sizeof(struct obj_data));
#else
  corpse = (struct obj_data*)dc_alloc(1, sizeof(struct obj_data));
#endif
  clear_object(corpse);
  corpse->item_number = NOWHERE;
  corpse->in_room = NOWHERE;
  corpse->name = str_hsh("husk");
  sprintf(buf, "The withered husk of %s, its soul drained, flutters here.",
    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->description = str_hsh(buf);
  sprintf(buf, "Husk of %s",
    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
  corpse->short_description = str_hsh(buf);
  
  corpse->obj_flags.type_flag = ITEM_TRASH;
  corpse->obj_flags.wear_flags = 0;
  corpse->obj_flags.value[0] = 0;
  corpse->obj_flags.value[3] = 1;
  corpse->obj_flags.weight = 1000;
  corpse->obj_flags.eq_level = 0;
  if(IS_NPC(ch)) {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  } else {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }
  obj_to_room(corpse, ch->in_room);
  return;
}

void make_head(CHAR_DATA * ch)
{
  struct obj_data *corpse;
  char buf[MAX_STRING_LENGTH];
  
#ifdef LEAK_CHECK
  corpse = (struct obj_data *)calloc(1, sizeof(struct obj_data));
#else
  corpse = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
#endif
  clear_object(corpse);
  
  corpse->item_number = NOWHERE;
  corpse->in_room = NOWHERE;
  corpse->name = str_hsh("head");
  
	 sprintf(buf, "The head of %s is lying here.",
	    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
   corpse->description = str_hsh(buf);
   
   sprintf(buf, "Head of %s",
	    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
   corpse->short_description = str_hsh(buf);
   
   
   corpse->obj_flags.type_flag = ITEM_TRASH;
   corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_HOLD;
   corpse->obj_flags.value[0] = 0;	/* You can't store stuff in a corpse */
   corpse->obj_flags.value[3] = 1;	/* corpse identifyer */
   corpse->obj_flags.weight = 5;
   corpse->obj_flags.eq_level = 0;
   if (IS_NPC(ch))
   {
     corpse->obj_flags.more_flags = 0;
     corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
   }
   else
   {
     corpse->obj_flags.more_flags = 0;
     corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
   }
   
   obj_to_room(corpse, ch->in_room);
   
   return;
}

void make_arm(CHAR_DATA * ch)
{
  struct obj_data *corpse;
  char buf[MAX_STRING_LENGTH];
  
#ifdef LEAK_CHECK
  corpse = (struct obj_data *)calloc(1, sizeof(struct obj_data));
#else
  corpse = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
#endif
  clear_object(corpse);
  
  corpse->item_number = NOWHERE;
  corpse->in_room = NOWHERE;
  corpse->name = str_hsh("arm");
  
   sprintf(buf, "The arm of %s is lying here.",
	    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
   corpse->description = str_hsh(buf);
   
   sprintf(buf, "Arm of %s",
	    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
   corpse->short_description = str_hsh(buf);
   
   
   corpse->obj_flags.type_flag = ITEM_TRASH;
   corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_HOLD;
   corpse->obj_flags.value[0] = 0;	/* You can't store stuff in a corpse */
   corpse->obj_flags.value[3] = 1;	/* corpse identifyer */
   corpse->obj_flags.weight = 5;
   corpse->obj_flags.eq_level = 0;
   if (IS_NPC(ch))
   {
     corpse->obj_flags.more_flags = 0;
     corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
   }
   else
   {
     corpse->obj_flags.more_flags = 0;
     corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
   }
   
   obj_to_room(corpse, ch->in_room);
   
   return;
}

void make_leg(CHAR_DATA * ch)
{
  struct obj_data *corpse;
  char buf[MAX_STRING_LENGTH];
  
#ifdef LEAK_CHECK
  corpse = (struct obj_data *)calloc(1, sizeof(struct obj_data));
#else
  corpse = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
#endif
  clear_object(corpse);
  
  corpse->item_number = NOWHERE;
  corpse->in_room = NOWHERE;
  corpse->name = str_hsh("leg");
  
   sprintf(buf, "The leg of %s is lying here.",
	    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
   corpse->description = str_hsh(buf);
   
   sprintf(buf, "Leg of %s",
	    (IS_NPC(ch) ? ch->short_desc : GET_NAME(ch)));
   corpse->short_description = str_hsh(buf);
   
   
   corpse->obj_flags.type_flag = ITEM_TRASH;
   corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_HOLD;
   corpse->obj_flags.value[0] = 0;	/* You can't store stuff in a corpse */
   corpse->obj_flags.value[3] = 1;	/* corpse identifyer */
   corpse->obj_flags.weight = 5;
   corpse->obj_flags.eq_level = 0;
   if (IS_NPC(ch))
   {
     corpse->obj_flags.more_flags = 0;
     corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
   }
   else
   {
     corpse->obj_flags.more_flags = 0;
     corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
   }
   
   obj_to_room(corpse, ch->in_room);
   
   return;
}



void make_heart(CHAR_DATA * ch, CHAR_DATA * vict)
{
  struct obj_data *corpse;
  char buf[MAX_STRING_LENGTH];
  
#ifdef LEAK_CHECK
  corpse = (struct obj_data *)calloc(1, sizeof(struct obj_data));
#else
  corpse = (struct obj_data *)dc_alloc(1, sizeof(struct obj_data));
#endif
  
  clear_object(corpse);
  
  corpse->item_number = NOWHERE;
  corpse->in_room = NOWHERE;
  corpse->name = str_hsh("heart");
  
  sprintf(buf, "%s's heart is laying here.",
    (IS_NPC(vict) ? vict->short_desc : GET_NAME(vict)));
  corpse->description = str_hsh(buf);
  
  sprintf(buf, "the heart of %s",
    (IS_NPC(vict) ? vict->short_desc : GET_NAME(vict)));
  corpse->short_description = str_hsh(buf);
  
  corpse->obj_flags.type_flag = ITEM_FOOD;
  corpse->obj_flags.wear_flags = ITEM_TAKE + ITEM_HOLD;
  corpse->obj_flags.value[0] = 0;	/* You can't store stuff in a heart */
  corpse->obj_flags.value[3] = 1;	/* corpse identifyer */
  corpse->obj_flags.weight = 2;
  corpse->obj_flags.eq_level = 0;
  if (IS_NPC(ch))
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_NPC_CORPSE_TIME;
  }
  else
  {
    corpse->obj_flags.more_flags = 0;
    corpse->obj_flags.timer = MAX_PC_CORPSE_TIME;
  }
  equip_char(ch, corpse, HOLD);
  
  return;
}


void death_cry(CHAR_DATA * ch)
{
  int door, was_in;
  char *message;
  
  act("Your blood freezes as you hear $n's death cry.",
    ch, 0, 0, TO_ROOM, 0);
  
  if (IS_NPC(ch))
    message = "You hear something's death cry.";
  else
    message = "You hear someone's death cry.";
  
  was_in = ch->in_room;
  for (door = 0; door <= 5; door++)
  {
    if (CAN_GO(ch, door))
    {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      if (ch->in_room == was_in)
        continue;
      act(message, ch, 0, 0, TO_ROOM, 0);
      ch->in_room = was_in;
    }
  }
  ch->in_room = was_in;
}

// Return TRUE if killed vict.  False otherwise
int do_skewer(CHAR_DATA *ch, CHAR_DATA *vict, int dam, int weapon)
{
  int percent, damadd = 0;
  
  if((GET_CLASS(ch) != CLASS_WARRIOR) && GET_LEVEL(ch) < ARCHANGEL)  return 0;  
  if(GET_LEVEL(ch) < MAX_MORTAL)                                     return 0;
  if(!IS_NPC(vict) && GET_LEVEL(vict) >= IMMORTAL)                   return 0;	
  if(!ch->equipment[weapon])                                         return 0;

  percent = number(1, 101); // 101 is complete failure
  if(percent > has_skill(ch, SKILL_SKEWER))                          return 0;

  int type = ch->equipment[weapon]->obj_flags.value[3];
  if( ! (type == TYPE_SLASH || type == TYPE_STING || type == TYPE_PIERCE) )  return 0;
  
  if (number(0, 100) < 5) {
    act("$n jams his weapon into $N!", ch, 0, vict, TO_ROOM, NOTVICT);
    act("You jam your weapon in $N's heart!", ch, 0, vict, TO_CHAR, 0);
    act("$n's weapon is speared into you! Ouch!", ch, 0, vict, TO_VICT, 0);
    damadd = dam * 2; // doubles original damage
    if (GET_LEVEL(vict) > GET_LEVEL(ch))
      damadd /= GET_LEVEL(vict) - GET_LEVEL(ch);
    GET_HIT(vict) -= damadd;
    
    if(number(0, 4999) == 1) { /* tiny chance of instakill */
      GET_HIT(vict) = -1;
      send_to_char("You impale your weapon through your opponent's chest!\r\n", ch);
      act("$n's weapon blows through your chest sending your entrails flying for yards behind you.  Everything goes black...", ch, 0, vict, TO_VICT, 0);
      act("$n's weapon rips through $N's chest sending gore and entrails flying for yards!\r\n", ch, 0, vict, NOTVICT, 0);
      update_pos(vict);
      return 1;
    }
  }
  // if they're still here the skewer missed
  return 0;
}

void raw_kill(CHAR_DATA * ch, CHAR_DATA * victim)
{
  CHAR_DATA *i=0;
  char buf[MAX_STRING_LENGTH];
  char buf1[100], buf2[100];
  int is_thief = 0;
  int death_room = 0;
  
  if(!victim) {
    log("Error in raw_kill()!  Null ch or victim!", IMMORTAL, LOG_BUG);
    return;
  }
  
  if (IS_SET(world[victim->in_room].room_flags, ARENA)) {
    fight_kill(ch, victim, TYPE_ARENA_KILL);
    return;
  }
  
  if(ch && IS_NPC(victim) && !IS_NPC(ch) && GET_LEVEL(ch) >= IMMORTAL) { 
    sprintf(buf, "%s killed %s.", GET_NAME(ch), GET_NAME(victim));
    special_log(buf);
  }

  GET_POS(victim) = POSITION_STANDING;  
  mprog_death_trigger(victim, ch);
  GET_POS(victim) = POSITION_DEAD;  
  
  if(GET_RACE(victim) == RACE_UNDEAD)
    make_dust(victim);
  else make_corpse(victim);
  
  if(IS_NPC(victim)) { 
    extract_char(victim, TRUE);
    return;
  }
  
  
  victim->pcdata->group_kills = 0;
  victim->pcdata->grplvl      = 0;
  if(IS_SET(victim->pcdata->punish, PUNISH_SPAMMER))
    REMOVE_BIT(victim->pcdata->punish, PUNISH_SPAMMER);
  if(IS_SET(victim->pcdata->punish, PUNISH_THIEF)) {
    is_thief = 1;
    REMOVE_BIT(victim->pcdata->punish, PUNISH_THIEF);
  }

  GET_POS(victim) = POSITION_RESTING;
  death_room = victim->in_room;
  extract_char(victim, FALSE);
  
  while(victim->affected)
    affect_remove(victim, victim->affected);
  
  GET_HIT(victim)  = 1;
  if(GET_MOVE(victim) <= 0)
    GET_MOVE(victim) = 1;
  if(GET_MANA(victim) <= 0)
    GET_MANA(victim) = 1;
  
  if (GET_CLASS(victim) == CLASS_MONK)
    GET_AC(victim) -= (GET_LEVEL(victim) * 3);
  
  save_char_obj(victim);
  
  for (i = character_list; i; i = i->next) {
    remove_memory(i, 'h', victim);
    if (IS_NPC(i) && (i->mobdata->fears))
        if (!strcmp(i->mobdata->fears, GET_NAME(victim)))
          remove_memory(i, 'f');
    if (IS_NPC(i) && (i->hunting))
        if (!strcmp(i->hunting, GET_NAME(victim)))
          remove_memory(i, 't');
  }
  
  /* If we're still here we can thrash the victim */
  if(!IS_NPC(victim)) /* We don't need mob deaths logged */
  {
    if(ch)
      sprintf(buf, "%s killed by %s", GET_NAME(victim), GET_NAME(ch));
    else sprintf(buf, "%s killed by [null killer]", GET_NAME(victim));
    // notify the clan members - clan_death checks for null ch/vict
    clan_death (victim, ch);
    sprintf(log_buf, "%s at %d", buf, world[death_room].number);
    log(log_buf, ANGEL, LOG_MORTAL);

    // update stats
    GET_RDEATHS(victim) += 1;

    /* New death system... dying is a BITCH!  */
    // Stat loss if they are a thief and they were killed by:  mob/self/clannie
    // or they're not a thief, and got a bad roll
    if((is_thief && ((ch && (IS_NPC(ch) || ch == victim || (ch->clan && ch->clan == victim->clan))))) ||
       (!is_thief && (GET_LEVEL(victim)>20 && number(1,101) <= GET_LEVEL(victim))) )
    {
      GET_CON(victim) -= 1;
      victim->raw_con -= 1;
      send_to_char("*** You lose one constitution point ***\n\r", victim);
      if(!IS_NPC(victim)) 
      {
        sprintf(log_buf, "%s lost a con. ouch.", GET_NAME(victim));
        log(log_buf, SERAPH, LOG_MORTAL);
      }
      pir_stat_loss(victim);
      if(GET_CON(victim) <= 4)
      {
        sprintf(buf1, "%s/%c/%s", SAVE_DIR, victim->name[0], victim->name);
        send_to_char("Your Constitution has reached 4...you are permanently dead!\n\r", victim);
        send_to_char("\r\n"
          "         (buh bye, -pirahna)\r\n"
          "        O  ,-----------,\r\n"
          "       o  /             \\  /|\r\n"
          "       . /  0            \\/ |\r\n"
          "        |                   |\r\n"
          "         \\               /\\ |\r\n"
          "          \\             /  \\|\r\n"
          "           `-----------`\r\n", victim);
        do_quit(victim, "", 666);
        unlink(buf1);
        sprintf(buf2, "%s permanently dies.", buf1);
        log(buf2, ANGEL, LOG_MORTAL);
        return;
      }
    } // lose stats
    GET_EXP(victim) /= 2;
  } // !IS_NPC
}



void group_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
  char buf[256];
  long no_members, share;
  long totallevels;
  CHAR_DATA *k, *highest;
  struct follow_type *f;
  
  /* No exp for pkills (duh) */
  if(is_pkill(ch, victim))
    return;
  
    /* Monsters don't get kill xp's. Dying of mortal wounds doesn't give xp
    * to anyone!
  */
  if(ch == victim)
    return;
  
  if(!IS_NPC(victim))
    return;
  
  if(!IS_AFFECTED(ch, AFF_CHARM) && IS_NPC(ch))
    return;
  
  if((k = ch->master) == NULL)
    k = ch;
  
  if((highest = ch->master) == NULL)
    highest = ch;
  
  no_members = 0;
  totallevels = 0;
  
  if(IS_AFFECTED(k, AFF_GROUP) && k->in_room == ch->in_room) { 
    no_members += 1;
    totallevels += GET_LEVEL(k);
  }
  
  for(f = k->followers; f; f = f->next) { 
    if(IS_AFFECTED(f->follower, AFF_GROUP) &&  
      f->follower->in_room == ch->in_room) { 
      no_members += 1;
      totallevels += GET_LEVEL(f->follower);
      if(GET_LEVEL(f->follower) > GET_LEVEL(highest))
        highest = f->follower;
    }
  }
  
  if(no_members == 0) { 
    no_members = 1;
    totallevels = GET_LEVEL(ch);
  }

  share = GET_EXP(victim);

  switch(no_members) {
    case 1:  share = (int) (share * 0.8); break;  
    case 2:  break; // * 1.0 
    case 3:  share = (int) (share * 1.2); break;  
    default:  share = (int) (share * 1.4); break; 
  }

  /* Kludgy loop to get k in at end. */
  for(f = k->followers;; f = f->next) 
  { 
    CHAR_DATA *tmp_ch;
    long tmp_share;
    
    tmp_ch = (f == NULL) ? k : f->follower;
    
    if(tmp_ch->in_room != ch->in_room)
      goto LContinue;
    
    if((!IS_AFFECTED(tmp_ch, AFF_GROUP)) && (no_members > 1))
      goto LContinue;
    
    if(!IS_AFFECTED(tmp_ch, AFF_GROUP) && tmp_ch != ch)
      goto LContinue;
    
    if((tmp_ch != ch) && (!IS_AFFECTED(ch, AFF_GROUP)))
      goto LContinue;
    
    if(GET_LEVEL(tmp_ch) - GET_LEVEL(highest) <= -20) {
      act("You are too low for this group.  You gain no experience.",
        tmp_ch, 0, 0, TO_CHAR, 0);
      goto LContinue;
    }
    
    
    if(GET_LEVEL(tmp_ch) == 0 || share == 0)
      tmp_share = 0;
    else tmp_share = (GET_LEVEL(tmp_ch) * share / totallevels);

    // reduce xp if you are higher level than mob
    switch(GET_LEVEL(victim) - GET_LEVEL(ch)) {
      case -1: tmp_share = (int) (tmp_share * 0.9); break;
      case -2: tmp_share = (int) (tmp_share * 0.8); break;
      case -3: tmp_share = (int) (tmp_share * 0.7); break;
      case -4: tmp_share = (int) (tmp_share * 0.6); break;
      case -5: tmp_share = (int) (tmp_share * 0.5); break;
      case -6: tmp_share = (int) (tmp_share * 0.4); break;
      case -7: tmp_share = (int) (tmp_share * 0.3); break;
      case -8: tmp_share = (int) (tmp_share * 0.2); break;
      case -9: tmp_share = (int) (tmp_share * 0.1); break;
      default:  if(GET_LEVEL(victim) < GET_LEVEL(ch))
                  tmp_share = 0;
               break;
    }
 
    // reduce/add to xp depending on number of people in group
    switch(no_members) {
      case 1:
        tmp_share = MIN((GET_LEVEL(tmp_ch) * 6000), tmp_share);
        break;
      case 2:
        tmp_share = MIN((GET_LEVEL(tmp_ch) * 7000), tmp_share);
        break;
      case 3:
        tmp_share = MIN((GET_LEVEL(tmp_ch) * 8000), tmp_share);
        break;
      default: 
        tmp_share = MIN((GET_LEVEL(tmp_ch) * 9000), tmp_share);
        break;
    }
    
    if(tmp_share > share)
      tmp_share = share;
    
    /* pir 3/12/1999 to get rid of -exp bug for mobs with too much xp */
    if(tmp_share < 0)
      tmp_share = 0;
    
    if(IS_NPC(tmp_ch))
      tmp_share /= 2;
    
    sprintf(buf, "You receive %ld exps of %ld total.\n\r",
      tmp_share, share);
    send_to_char(buf, tmp_ch);
    gain_exp(tmp_ch, tmp_share);
    change_alignment(tmp_ch, victim);
    
LContinue:
    if(f == NULL)
      break;
  }
}




void dam_message(int dam, CHAR_DATA * ch, CHAR_DATA * victim,
                 int w_type, long modifier)
{
  static char *attack_table[] =
  {
    "hit", "pound", "pierce", "slash", "whip", "claw",
      "bite", "sting", "crush"
  };
  
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
  char *vs, *vp, *vx;
  char *attack;
  char punct;
  char modstring[200];
  
  vx = "";
  
  if(dam == 0) { 
    vs = "miss";
    vp = "misses";
  }
  else if(dam <= 2) { 
    vs = "tickle";
    vp = "tickles";
	 }
  else if(dam <= 5) { 
    vs = "graze";
    vp = "grazes";
	 }
  else if(dam <= 10) { 
    vs = "hit";
    vp = "hits";
  }
  else if(dam <= 15) { 
    vs = "hit";
    vp = "hits";
    vx = " hard";
  }
  else if(dam <= 20) { 
    vs = "hit";
    vp = "hits";
    vx = " very hard";
  }
  else if(dam <= 27) { 
    vs = "hit";
    vp = "hits";
    vx = " damn hard";
	 }
  else if(dam <= 35) { 
    vs = "massacre";
    vp = "massacres";
  }
  else if(dam <= 49) { 
    vs = "annihilate";
    vp = "annihilates";
  }
  else if(dam <= 55) { 
    vs = "obliterate";
    vp = "obliterates"; 
  }
  else if(dam <= 70) {
    vs = "cremate";
    vp = "cremates";
  }
  else if(dam <= 85) {
    vs = "beat the shit out of";
    vp = "beats the shit out of";
  } 
  else if(dam <= 100) {
    vs = "BEAT THE LIVING SHIT out of";
    vp = "BEATS THE LIVING SHIT out of";
  }
  else if(dam <= 150) {
    vs = "POUND THE FUCK out of";
    vp = "POUNDS THE FUCK out of";
  }
  else if(dam <= 200) {
    vs = "FUCKING DEMOLISH";
    vp = "FUCKING DEMOLISHES";
  }
  else { 
     vs = "TOTALLY FUCKING DISINTEGRATE";
     vp = "TOTALLY FUCKING DISINTEGRATES";
  }
   
   w_type -= TYPE_HIT;
   if (((unsigned) w_type) >= sizeof(attack_table))
   {
     log("Dam_message: bad w_type", ANGEL, LOG_BUG);
     w_type = 0;
   }
   punct = (dam <= 29) ? '.' : '!';

   if(modifier) {
     strcpy(modstring, "frenzied ");
   }
   else *modstring = '\0';

   if (w_type == 0)
   {
     sprintf(buf1, "$n's %spunch %s $N%s%c", modstring, vp, vx, punct);
     sprintf(buf2, "Your %spunch %s $N%s%c", modstring, vs, vx, punct);
     sprintf(buf3, "$n's %spunch %s you%s%c", modstring, vp, vx, punct);
   }
   else
   {
     attack = attack_table[w_type];
     sprintf(buf1, "$n's %s%s %s $N%s%c", modstring, attack, vp, vx, punct);
     sprintf(buf2, "Your %s%s %s $N%s%c", modstring, attack, vp, vx, punct);
     sprintf(buf3, "$n's %s%s %s you%s%c", modstring, attack, vp, vx, punct);
   }
   
   act(buf1, ch, NULL, victim, TO_ROOM, NOTVICT);
   act(buf2, ch, NULL, victim, TO_CHAR, 0);
   act(buf3, ch, NULL, victim, TO_VICT, 0);
}



/*
* Disarm a creature.
* Caller must check for successful attack.
*/
void disarm(CHAR_DATA * ch, CHAR_DATA * victim)
{
  struct obj_data *obj;
  
  if (victim->equipment[WIELD] == NULL)            return;
  if (ch->equipment[WIELD] == NULL)                return;
  
  act("$n disarms you and sends your weapon flying!", ch, NULL, victim, TO_VICT, 0);
  act("You disarm $N and send $S weapon flying!", ch, NULL, victim, TO_CHAR, 0);
  act("$n disarms $N and sends $S weapon flying!", ch, NULL, victim, TO_ROOM, NOTVICT);

// all disarms go to inventory right now -pir 
//  if (!IS_NPC(ch)) {
    obj = unequip_char(victim, WIELD);
    obj_to_char(obj, victim);
    if (victim->equipment[SECOND_WIELD]) {
      obj = unequip_char(victim, SECOND_WIELD);
      equip_char(victim, obj, WIELD);
    }
    return;
//  }

// we never get here cause of above code
  
  obj = unequip_char(victim, WIELD);
  /* If it's gl make it go to inventory. Morc. */
  if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL))
    obj_to_char(obj, victim);
  else
    obj_to_room(obj, victim->in_room);
  
  if (victim->equipment[SECOND_WIELD]) {
    obj = unequip_char(victim, SECOND_WIELD);
    equip_char(victim, obj, WIELD);
  }
}




/*
* Trip a creature.
* Caller must check for successful attack.
*/
void trip(CHAR_DATA * ch, CHAR_DATA * victim)
{
  if(!can_attack(ch) || !can_be_attacked(ch, victim))
    return;
  
  act("$n trips you and you go down!",
    ch, NULL, victim, TO_VICT, 0);
  act("You trip $N and $N goes down!",
    ch, NULL, victim, TO_CHAR, 0);
  act("$n trips $N and $N goes down!",
    ch, NULL, victim, TO_ROOM, NOTVICT);
  
	 WAIT_STATE(ch, PULSE_VIOLENCE * 2);
   if(damage(ch, victim, 1,TYPE_HIT, SKILL_TRIP, 0) == (-1))
     return;
   
   WAIT_STATE(victim, PULSE_VIOLENCE * 3);
   GET_POS(victim) = POSITION_SITTING;
   
   return;
}

// 'ch' can be null
// do_pkill should never be called directly, only through "fight_kill"
void do_pkill(CHAR_DATA *ch, CHAR_DATA *victim)
{
  char killer_message[MAX_STRING_LENGTH];
//  CHAR_DATA *i = 0;
  struct affected_type *af, *afpk;

  void move_player_home(CHAR_DATA *victim);

  if(!victim) {
    log("Null victim sent to do_pkill.", LOG_BUG, IMMORTAL);
    return;
  }
  
  if(ch) {
    set_cantquit(ch, victim);
  }
    
  // Kill charmed mobs outright
  if(IS_NPC(victim)) {
    fight_kill(ch, victim, TYPE_RAW_KILL);
    return;
  }

  if(IS_SET(victim->pcdata->punish, PUNISH_THIEF)) {
    GET_MOVE(victim) = 2;
    fight_kill(ch, victim, TYPE_RAW_KILL);
    return;
  }
  
  // Make sure barbs get their ac back
  if (IS_SET(victim->combat, COMBAT_BERSERK)) 
  {
    REMOVE_BIT(victim->combat, COMBAT_BERSERK);
    GET_AC(victim) -= 80;
  }
  
  for(af = victim->affected; af; af = afpk) {
    afpk = af->next;
    if(af->type != FUCK_CANTQUIT && 
       af->type != SKILL_LAY_HANDS &&
       af->type != SKILL_BLOOD_FURY)
      affect_remove(victim, af);
  }
  
  GET_HIT(victim)  = 1;
  GET_KI(victim)   = 1;
  GET_MANA(victim) = 1;
  if(IS_AFFECTED(victim, AFF_CANTQUIT))
    GET_MOVE(victim) = 4;
  
  move_player_home(victim);
  
  GET_POS(victim) = POSITION_RESTING;
  
  save_char_obj(victim);

  // have to be level 10 and linkalive to count as a pkill and not yourself
  if(!IS_MOB(ch) && GET_LEVEL(victim) > 9 && victim->desc && ch != victim)
  {
    GET_PKILLS(ch) += 1;
    GET_PKILLS_LOGIN(ch) += 1;

    GET_PDEATHS(victim) += 1;
    GET_PDEATHS_LOGIN(victim) += 1;

    GET_PKILLS(ch)             += 1;
    GET_PKILLS_LOGIN(ch)       += 1;
    GET_PKILLS_TOTAL(ch)       += GET_LEVEL(victim);
    GET_PKILLS_TOTAL_LOGIN(ch) += GET_LEVEL(victim);

    if(IS_AFFECTED(ch, AFF_GROUP)) {
        char_data * master   = ch->master ? ch->master : ch;
        if(!IS_MOB(master)) {
          master->pcdata->grplvl      += GET_LEVEL(victim);
          master->pcdata->group_kills += 1;
        }
      }
    
    sprintf(killer_message,"\n\r##%s was just DEFEATED in battle by %s!\n\r",
      GET_NAME(victim), GET_NAME(ch));
  } // if (ch && ch != victim)
  
  else
    sprintf(killer_message,"\n\r##%s just DIED!\n\r", GET_NAME(victim));
  
  send_info(killer_message);
}

// 'ch' can be null
void arena_kill(CHAR_DATA *ch, CHAR_DATA *victim)
{
  void remove_nosave(CHAR_DATA *vict);
  
  char killer_message[MAX_STRING_LENGTH];
//  CHAR_DATA *i;
  CHAR_DATA * tmp = NULL;
  struct clan_data * clan = NULL;
  struct clan_data * clan2 = NULL;
  extern int arena[4];
  int eliminated = 1;
  void move_player_home(CHAR_DATA *victim);
  
  if(!victim) {
    log("Null victim sent to do_pkill.", LOG_BUG, IMMORTAL);
    return;
  }

  // Kill mobs outright
  if (IS_NPC(victim)) {
    mprog_death_trigger(victim, ch);
    remove_nosave(victim);
    extract_char(victim, TRUE);
    return;
  }

  // We keep objects with the ITEM_NOSAVE flag
  // Needs to be called BEFORE move_char so that the items
  // end up in the right room
  remove_nosave(victim);
  
  move_player_home(victim);
  
  while(victim->affected)
    affect_remove(victim, victim->affected);
  
  if(ch && arena[2] == -2)
  {
    if(ch && ch->clan && GET_LEVEL(ch) < IMMORTAL)        clan  = get_clan(ch);
    if(victim->clan && GET_LEVEL(victim) < IMMORTAL)      clan2 = get_clan(victim);  
    sprintf(killer_message, "\n\r## %s [%s] just SLAUGHTERED %s [%s] in the arena!\n\r", 
         GET_NAME(ch), clan ? clan->name : "no clan", 
         GET_NAME(victim), clan2 ? clan2->name : "no clan");
    logf(111, LOG_CHAOS, "%s [%s] killed %s [%s]", GET_NAME(ch),
         clan ? clan->name : "no clan", GET_NAME(victim), 
         clan2 ? clan2->name : "no clan");
  }
  else if(ch)
    sprintf(killer_message, "\n\r## %s just SLAUGHTERED %s in the arena!\n\r", GET_SHORT(ch), GET_SHORT(victim));
  else
    sprintf(killer_message, "\n\r## %s just DIED in the arena!\n\r", GET_SHORT(victim));
  
  send_info(killer_message);
  
  // if it's a chaos, see if the clan was eliminated
  if(victim && arena[2] == -2 && clan2)
  {
    for(tmp = character_list; tmp; tmp = tmp->next) {
      if (IS_SET(world[tmp->in_room].room_flags, ARENA))
        if(victim->clan == tmp->clan && victim != tmp && GET_LEVEL(tmp) < IMMORTAL)
          eliminated = 0;
    }
    if(eliminated) {
      sprintf(killer_message, "## [%s] was just eliminated from the chaos!\n\r", clan2->name);
      send_info(killer_message);
    }    
  }
  
  send_to_char("You have been completely healed.\n\r", victim);
  GET_POS(victim) = POSITION_RESTING;
  GET_HIT(victim) = GET_MAX_HIT(victim);
  GET_MANA(victim) = GET_MAX_MANA(victim);
  GET_MOVE(victim) = GET_MAX_MOVE(victim);
  GET_KI(victim) = GET_MAX_KI(victim);
  save_char_obj(victim);
}

int is_stunned(CHAR_DATA *ch)
{
  if(IS_SET(ch->combat, COMBAT_STUNNED))
    return TRUE;
  if(IS_SET(ch->combat, COMBAT_STUNNED2))
    return TRUE;
  if(IS_SET(ch->combat, COMBAT_SHOCKED))
    return TRUE;
  return FALSE;
}

int can_attack(CHAR_DATA *ch)
{
  if((ch->in_room >= 0 && ch->in_room <= top_of_world) &&
    IS_SET(world[ch->in_room].room_flags, ARENA) && ArenaIsOpen()) {
    send_to_char("Wait until it closes\n\r", ch);
    return FALSE;
  }
  if(IS_AFFECTED(ch, AFF_PARALYSIS))
    return FALSE;
  if(is_stunned(ch))
    return FALSE;
  return TRUE;
}

int can_be_attacked(CHAR_DATA *ch, CHAR_DATA *vict)
{
  /* this will happen sometimes, no need to log it */
  if(!ch || !vict)
    return FALSE;
  
  if(vict->in_room < 0 || vict->in_room >= top_of_world || 
       ch->in_room < 0 || ch->in_room   >= top_of_world) 
    return FALSE;
  
  if(IS_NPC(vict))
    return TRUE;
  
  if(!IS_NPC(ch) && !IS_NPC(vict) && GET_LEVEL(ch) == 1) {
    send_to_char("You are too new in this realm to make enemies!\n\r", ch);
    return FALSE;
  }

  if(IS_AFFECTED(vict, AFF_CANTQUIT))
    return TRUE;
  
  if(!IS_NPC(ch) && GET_LEVEL(vict) == 1)  {
    send_to_char("You would slaughter a level one player?!\n\r", ch);
    return FALSE;
  }
  
  if(IS_SET(world[ch->in_room].room_flags, SAFE))
  {
    /* Allow the NPCs to continue fighting */
    if(IS_NPC(ch)) {
      if(ch->fighting == vict)
        return TRUE;
    }
    /* Allow players to continue fighting if they have a cantquit */
    if(IS_AFFECTED(ch, AFF_CANTQUIT)) // (tested earlier) || IS_AFFECTED(vict, AFF_CANTQUIT))
    {
      if(ch->fighting == vict)
        return TRUE;
    }
    /* Imps ignore safe flags  */
    if(!IS_NPC(ch) && (GET_LEVEL(ch) == IMP)) {
      send_to_char("There is no safe haven from an angry IMP!\n\r", vict);
      return TRUE;
    }
    
    send_to_char("No fighting permitted in a safe room.\n\r", ch);
    stop_fighting(ch);
    return FALSE;
  }
  return TRUE;
}

int weapon_spells(CHAR_DATA *ch, CHAR_DATA *vict, int weapon)
{
  int i, current_affect, chance, percent, retval;
  
  if(!ch || !vict) {
    log("Null ch or vict sent to weapon spells!", -1, LOG_BUG);
    return eFAILURE|eINTERNAL_ERROR;
  }
  
  if(!weapon)
    return eFAILURE;
  
  if(!can_attack(ch) || !can_be_attacked(ch, vict))
    return eFAILURE;
  
  if(ch->in_room != vict->in_room || GET_POS(vict) == POSITION_DEAD)
    return eFAILURE;
  
  if(!ch->equipment[weapon])
    return eFAILURE;
  
  for(i = 0; i < ch->equipment[weapon]->num_affects; i++)
  {
    /* It's possible the victim has fled or died */
    if(ch->in_room != vict->in_room) return eFAILURE;
    if(GET_POS(vict) == POSITION_DEAD) return eSUCCESS|eVICT_DIED;
    chance = number(0, 101);
    percent = ch->equipment[weapon]->affected[i].modifier;
    current_affect = ch->equipment[weapon]->affected[i].location;
    
    /* If they don't roll under chance, it doesn't work! */
    if(chance > percent) continue;

    switch(current_affect)
    {
    case WEP_MAGIC_MISSILE:
      retval = spell_magic_missile(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_BLIND:
      retval = spell_blindness(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_EARTHQUAKE:
      retval = spell_earthquake(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_CURSE:
      retval = spell_curse(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_COLOUR_SPRAY:
      retval = spell_colour_spray(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_DISPEL_EVIL:
      retval = spell_dispel_evil(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_ENERGY_DRAIN:
      retval = spell_energy_drain(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_FIREBALL:
      retval = spell_fireball(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_LIGHTNING_BOLT:
      retval = spell_lightning_bolt(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_HARM:
      retval = spell_harm(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_POISON:
      retval = spell_poison(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_SLEEP:
      retval = spell_sleep(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_FEAR:
      retval = spell_fear(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_DISPEL_MAGIC:
      retval = spell_dispel_magic(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_WEAKEN:
      retval = spell_weaken(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_CAUSE_LIGHT:
      retval = spell_cause_light(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_CAUSE_CRITICAL:
      retval = spell_cause_critical(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_PARALYZE:
      retval = spell_paralyze(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_ACID_BLAST:
      retval = spell_acid_blast(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_BEE_STING:
      retval = spell_bee_sting(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_CURE_LIGHT:
      retval = spell_cure_light(GET_LEVEL(ch), ch, ch, 0);
      break;
    case WEP_FLAMESTRIKE:
      retval = spell_flamestrike(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_HEAL_SPRAY:
      retval = spell_heal_spray(GET_LEVEL(ch), ch, ch, 0);
      break;
    case WEP_DROWN:
      retval = spell_drown(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_HOWL:
      retval = spell_howl(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_SOULDRAIN:
      retval = spell_souldrain(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_SPARKS:
      retval = spell_sparks(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_DISPEL_GOOD:
      retval = spell_dispel_good(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_TELEPORT:
      retval = spell_teleport(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_CHILL_TOUCH:
      retval = spell_chill_touch(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_POWER_HARM:
      retval = spell_power_harm(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_VAMPIRIC_TOUCH:
      retval = spell_vampiric_touch(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_LIFE_LEECH:
      retval = spell_life_leech(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_METEOR_SWARM:
      retval = spell_meteor_swarm(GET_LEVEL(ch), ch, vict, 0);
      break;
    case WEP_ENTANGLE:
      /* This is a hack since Morc did the spell wrong  - pir */
      retval = cast_entangle(GET_LEVEL(ch), ch, "", 0, vict, 0);
      break;
    case WEP_CREATE_FOOD:
      retval = cast_create_food(GET_LEVEL(ch), ch, "", 0, vict, 0);
      break;

    default:
      retval = eSUCCESS;
      // Don't want to log this since a non-spell affect is going to happen all
      // the time (like SAVE_VS_FIRE or HIT-N-DAM for example) -pir
      //logf(IMMORTAL, LOG_BUG, "Illegal affect %d in weapons spells item '%d'.", 
      //     current_affect, obj_index[ch->equipment[weapon]->item_number].virt);
      break;
    } /* switch statement */

    if(SOMEONE_DIED(retval))
      return retval;      
  } /* for loop */
  return eSUCCESS;    
}  /* spell effects */

int second_attack(CHAR_DATA *ch)
{
  if((IS_NPC(ch)) && (IS_SET(ch->mobdata->actflags, ACT_2ND_ATTACK)))
    return TRUE;
  if(number(1, 101) < has_skill(ch, SKILL_SECOND_ATTACK))
    return TRUE;
  return FALSE;
}

int third_attack(CHAR_DATA *ch)
{
  if((IS_NPC(ch)) && (IS_SET(ch->mobdata->actflags, ACT_3RD_ATTACK)))
    return TRUE;
  if(number(1, 101) < has_skill(ch, SKILL_THIRD_ATTACK))
    return TRUE;
  return FALSE;
}

int fourth_attack(CHAR_DATA *ch)
{
  if((IS_NPC(ch)) && (IS_SET(ch->mobdata->actflags, ACT_4TH_ATTACK)))
    return TRUE;
  return FALSE;
}

int second_wield(CHAR_DATA *ch)
{
  /* If the ch is capable of using the SECOND_WIELD */
  if((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_MONK))
    return FALSE;
  return TRUE;
}	

void inform_victim(CHAR_DATA *ch, CHAR_DATA *victim, int dam)
{
  int max_hit;
  
  switch (GET_POS(victim))
  {
  case POSITION_STUNNED:
    // This was moved into "attack" so that the message only goes off
    // once on the players first attack.
    //        act("$n is stunned, but will probably recover.",
    //        victim, 0, 0, TO_ROOM, INVIS_NULL);
    send_to_char("You are stunned, but will probably recover.\n\r",
      victim);
    break;
  case POSITION_DEAD:
    act("$n is DEAD!!", victim, 0, 0, TO_ROOM, INVIS_NULL);
    send_to_char("You have been KILLED!!\n\r\n\r", victim);
    break;
  default:
    max_hit = hit_limit(victim);
    if (dam > max_hit / 5)
      send_to_char("That really did HURT!\n\r", victim);
    // Wimp out?
    if ( GET_HIT(victim) < (max_hit / 5) ) {
      if (IS_SET(victim->combat, COMBAT_BERSERK) ||
	       IS_SET(victim->combat, COMBAT_RAGE1) ||
         IS_SET(victim->combat, COMBAT_RAGE2)) {
        send_to_char("You are too OUTRAGED to care about your "
          "wounds!\n\r", victim);
        return;
      }
      if (dam > 0)  
        send_to_char("You wish you would stop BLEEDING so much!\n\r",
        victim);
			   
      if (IS_NPC(victim)) {
        if (IS_SET(victim->mobdata->actflags, ACT_WIMPY))
        {
          remove_memory(victim, 't');
          remove_memory(victim, 'h', victim);
          if (victim->fighting)
            add_memory(victim, GET_NAME(victim->fighting), 'f');
          if ((!IS_AFFECTED(victim, AFF_PARALYSIS)) &&
            (!IS_SET(victim->combat, COMBAT_STUNNED)) &&
            (!IS_SET(victim->combat, COMBAT_STUNNED2)) &&
            (!IS_SET(victim->combat, COMBAT_BASH1)) &&
            (!IS_SET(victim->combat, COMBAT_BASH2)) && 
            (dam > 0))
            do_flee(victim, "", 0);
          return;
        } // end of if ACT_WIMPY
      } // end of if npc
      else {
        if (IS_SET(victim->pcdata->toggles, PLR_WIMPY)) {
            if ((!IS_AFFECTED(victim, AFF_PARALYSIS)) &&
              (!IS_SET(victim->combat, COMBAT_STUNNED)) &&
              (!IS_SET(victim->combat, COMBAT_STUNNED2)) &&
              (!IS_SET(victim->combat, COMBAT_BASH1)) &&
              (!IS_SET(victim->combat, COMBAT_BASH2)) &&
              (dam > 0))
              do_flee(victim, "", 0);
            return;
          }
       } // end else
    } // end max_hit / 5
    break;
  } // switch
} // inform_victim

  /*****
  | This function will return non-zero if the
  | ch is fighting a NON-pkill fight; that is,
  | if ch loses s/he will die a real death.
  | If the loser will be considered pkilled then
  | the function returns 0.
  | Morc 8/6/95
*/
int is_fighting_mob(CHAR_DATA *ch)
{
  CHAR_DATA *fighting = ch->fighting;
  if(!fighting) return 0;
  if(is_pkill(ch, fighting)) return 0;
  if(IS_NPC(fighting) && !IS_NPC(ch)) return 1;
  return 0;
}





int do_flee(struct char_data *ch, char *argument, int cmd)
{
  int i, attempt, retval;
  struct char_data * chTemp, * loop_ch; /*, * next_fighting; */
  
  if (is_stunned(ch))
    return eFAILURE;
  
  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  if(GET_CLASS(ch) == CLASS_BARD && IS_SINGING(ch))
     do_sing(ch, "stop", 9);

  if(IS_AFFECTED2(ch, AFF_NO_FLEE)) {
     if(affected_by_spell(ch, SPELL_IRON_ROOTS))
       send_to_char("The roots bracing your legs make it impossible to run!\r\n", ch);
     else send_to_char("Your legs are too tired for running away!\r\n", ch);
     return eFAILURE;
  }
  
  for(i = 0; i < 3; i++) {
    attempt = number(0, 5);  // Select a random direction
    // keep mobs from fleeing into a no_track room
    if(CAN_GO(ch, attempt))
      if(!IS_NPC(ch) || !IS_SET(world[EXIT(ch, attempt)->to_room].room_flags, NO_TRACK))
      {
          act("$n panics, and attempts to flee.", ch, 0, 0, TO_ROOM,
            INVIS_NULL);
          act("You panic, and attempt to flee.", ch, 0, 0, TO_CHAR, 0);
          
          // The escape has succeded
          if (!IS_SET((retval = do_simple_move(ch, attempt, FALSE)), eCH_DIED) &&
               IS_SET(retval, eSUCCESS)) 
          {
            // They got away.  Stop fighting for everyone not in room
            for (chTemp = combat_list; chTemp; chTemp = chTemp->next_fighting) 
            {
              
              if (chTemp->fighting == ch &&
                chTemp->in_room != ch->in_room) 
              {
                stop_fighting(chTemp);
              }
            } // for
            
            // If anyone in current room is fighting them, we're done
            // otherwise keep going
            for(loop_ch = world[ch->in_room].people; loop_ch; loop_ch = loop_ch->next_in_room)
              if(loop_ch->fighting == ch)
                return 1;
              
            stop_fighting(ch);
            return eSUCCESS;
          } // do_simple_move
          
          else {
            if (!IS_SET(retval, eCH_DIED)) 
              act("$n tries to flee, but is too exhausted!", ch, 0, 0, TO_ROOM, INVIS_NULL);
            return retval;
          }
      } // if CAN_GO
  } // for
  
  // No exits were found
  send_to_char("PANIC! You couldn't escape!\n\r", ch);
  return eFAILURE;
}


long int get_weapon_bit(int weapon_type)
{
  switch (weapon_type)
  {
  case TYPE_HIT:
    return(ISR_HIT);
  case TYPE_BLUDGEON:
    return(ISR_BLUDGEON);
  case TYPE_PIERCE:
    return(ISR_PIERCE);
  case TYPE_SLASH:
    return(ISR_SLASH);
  case TYPE_WHIP:
    return(ISR_WHIP);
  case TYPE_CLAW:
    return(ISR_CLAW);
  case TYPE_BITE:
    return(ISR_BITE);
  case TYPE_STING:
    return(ISR_STING);
  case TYPE_CRUSH:
    return(ISR_CRUSH);

  case TYPE_PHYSICAL_MAGIC:
  case TYPE_MAGIC:
    return(ISR_MAGIC);

  case TYPE_CHARM:
    return(ISR_CHARM);
  case TYPE_FIRE:
    return(ISR_FIRE);
  case TYPE_ENERGY:
    return(ISR_ENERGY);
  case TYPE_ACID:
    return(ISR_ACID);
  case TYPE_POISON:
    return(ISR_POISON);
  case TYPE_SLEEP:
    return(ISR_SLEEP);
  case TYPE_COLD:
    return(ISR_COLD);
  case TYPE_PARA:
    return(ISR_PARA);
  case TYPE_SONG:
    return(ISR_SONG);
  case TYPE_KI:
    return(ISR_KI);
  case TYPE_WATER:
    return(ISR_WATER);
  default:
    return(0);
  }  /* end switch */
  return(0);
}

void remove_nosave(CHAR_DATA *vict)
{
  struct obj_data *o, *next_obj, *blah, *tmp_o;
  
  if(!vict) {
    log("Null victim sent to remove_nosave!", OVERSEER, LOG_BUG);
    return;
  }
  
  for(o = vict->carrying; o; o = next_obj) {
    next_obj = o->next_content;
    
    // I suppose it would be possible to have a NOSAVE container --
    // in this case you lose everything inside the container.
    
    if(GET_ITEM_TYPE(o) == ITEM_CONTAINER &&
      !IS_SET(o->obj_flags.extra_flags, ITEM_NOSAVE)) {
      for(tmp_o = o->contains; tmp_o; tmp_o = blah) {
        blah = tmp_o->next_content;
        if(IS_SET(tmp_o->obj_flags.extra_flags, ITEM_NOSAVE))
          move_obj(tmp_o, vict->in_room);
      }
    }
    
    if(IS_SET(o->obj_flags.extra_flags, ITEM_NOSAVE))
      move_obj(o, vict->in_room);
    
  } // for
}  

int damage_type(int weapon_type)
{
  switch (weapon_type)
  {
  case TYPE_HIT:
  case TYPE_BLUDGEON:
  case TYPE_PIERCE:
  case TYPE_SLASH:
  case TYPE_WHIP:
  case TYPE_CLAW:
  case TYPE_BITE:
  case TYPE_STING:
  case TYPE_CRUSH:
    return(DAMAGE_TYPE_PHYSICAL);

  case TYPE_MAGIC:
  case TYPE_CHARM:
  case TYPE_FIRE:
  case TYPE_ENERGY:
  case TYPE_ACID:
  case TYPE_POISON:
  case TYPE_SLEEP:
  case TYPE_COLD:
  case TYPE_PARA:
  case TYPE_KI:
  case TYPE_PHYSICAL_MAGIC:
    return(DAMAGE_TYPE_MAGIC);

  case TYPE_SONG:
    return(DAMAGE_TYPE_SONG);
  default:
    return(0);
  }  /* end switch */
  return(0);
}
